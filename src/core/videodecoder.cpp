#include "core/videodecoder.h"
#include <QDebug>

namespace VideoStudio {

VideoDecoder::VideoDecoder(QObject* parent)
    : QObject(parent)
    , m_formatContext(nullptr)
    , m_codecContext(nullptr)
    , m_frame(nullptr)
    , m_packet(nullptr)
    , m_videoStreamIndex(-1)
    , m_currentFrameNumber(0)
    , m_indexBuilt(false)
{
    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
}

VideoDecoder::~VideoDecoder() {
    close();
    if (m_frame) {
        av_frame_free(&m_frame);
    }
    if (m_packet) {
        av_packet_free(&m_packet);
    }
}

bool VideoDecoder::openFile(const QString& filePath) {
    close();

    // Open input file
    if (avformat_open_input(&m_formatContext, filePath.toUtf8().constData(), nullptr, nullptr) < 0) {
        emit error("Could not open file: " + filePath);
        return false;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        emit error("Could not find stream information");
        close();
        return false;
    }

    // Find video stream
    m_videoStreamIndex = -1;
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = i;
            break;
        }
    }

    if (m_videoStreamIndex == -1) {
        emit error("Could not find video stream");
        close();
        return false;
    }

    // Get codec parameters
    AVCodecParameters* codecParams = m_formatContext->streams[m_videoStreamIndex]->codecpar;

    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        emit error("Unsupported codec");
        close();
        return false;
    }

    // Allocate codec context
    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        emit error("Could not allocate codec context");
        close();
        return false;
    }

    // Copy codec parameters to context
    if (avcodec_parameters_to_context(m_codecContext, codecParams) < 0) {
        emit error("Could not copy codec parameters");
        close();
        return false;
    }

    // Open codec
    if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
        emit error("Could not open codec");
        close();
        return false;
    }

    qDebug() << "Opened video file:" << filePath;
    qDebug() << "Codec:" << codec->name;
    qDebug() << "Resolution:" << m_codecContext->width << "x" << m_codecContext->height;
    qDebug() << "Frame rate:" << getFrameRate();

    // Build frame index
    if (!buildFrameIndex()) {
        emit error("Failed to build frame index");
        close();
        return false;
    }

    m_currentFrameNumber = 0;
    return true;
}

void VideoDecoder::close() {
    freeResources();
    m_frameIndex.clear();
    m_currentFrameNumber = 0;
    m_indexBuilt = false;
}

AVFrame* VideoDecoder::decodeNextFrame() {
    if (!m_codecContext) {
        return nullptr;
    }

    while (av_read_frame(m_formatContext, m_packet) >= 0) {
        if (m_packet->stream_index == m_videoStreamIndex) {
            // Send packet to decoder
            int ret = avcodec_send_packet(m_codecContext, m_packet);
            if (ret < 0) {
                av_packet_unref(m_packet);
                continue;
            }

            // Receive decoded frame
            ret = avcodec_receive_frame(m_codecContext, m_frame);
            if (ret == 0) {
                m_currentFrameNumber++;
                av_packet_unref(m_packet);
                return m_frame;
            }
        }
        av_packet_unref(m_packet);
    }

    // Flush decoder
    avcodec_send_packet(m_codecContext, nullptr);
    if (avcodec_receive_frame(m_codecContext, m_frame) == 0) {
        m_currentFrameNumber++;
        return m_frame;
    }

    return nullptr;
}

bool VideoDecoder::seekToFrame(int frameNumber) {
    if (!m_codecContext || frameNumber < 0 || frameNumber >= m_frameIndex.frameCount()) {
        return false;
    }

    const FrameInfo* frameInfo = m_frameIndex.getFrame(frameNumber);
    if (!frameInfo) {
        return false;
    }

    // Seek to timestamp
    int64_t timestamp = frameInfo->pts;
    if (av_seek_frame(m_formatContext, m_videoStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD) < 0) {
        return false;
    }

    avcodec_flush_buffers(m_codecContext);

    // Decode until we reach the target frame
    int currentFrame = 0;
    while (currentFrame < frameNumber) {
        if (!decodeNextFrame()) {
            return false;
        }
        currentFrame++;
    }

    m_currentFrameNumber = frameNumber;
    return true;
}

bool VideoDecoder::seekToTime(double seconds) {
    if (!m_codecContext) {
        return false;
    }

    AVStream* stream = m_formatContext->streams[m_videoStreamIndex];
    int64_t timestamp = static_cast<int64_t>(seconds * AV_TIME_BASE);
    timestamp = av_rescale_q(timestamp, AV_TIME_BASE_Q, stream->time_base);

    if (av_seek_frame(m_formatContext, m_videoStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD) < 0) {
        return false;
    }

    avcodec_flush_buffers(m_codecContext);
    return true;
}

bool VideoDecoder::buildFrameIndex() {
    if (!m_formatContext || m_videoStreamIndex < 0) {
        return false;
    }

    m_frameIndex.clear();

    AVStream* stream = m_formatContext->streams[m_videoStreamIndex];
    AVPacket* packet = av_packet_alloc();

    int frameNumber = 0;
    int64_t lastPts = 0;

    while (av_read_frame(m_formatContext, packet) >= 0) {
        if (packet->stream_index == m_videoStreamIndex) {
            FrameInfo frameInfo;
            frameInfo.frameNumber = frameNumber++;
            frameInfo.pts = packet->pts;
            frameInfo.dts = packet->dts;
            frameInfo.offset = packet->pos;
            frameInfo.size = packet->size;
            frameInfo.isKeyFrame = (packet->flags & AV_PKT_FLAG_KEY) != 0;

            // Calculate timestamp
            if (packet->pts != AV_NOPTS_VALUE) {
                frameInfo.timestamp = packet->pts * av_q2d(stream->time_base);
            }

            // Calculate bitrate
            if (packet->pts != AV_NOPTS_VALUE && lastPts != 0) {
                double duration = (packet->pts - lastPts) * av_q2d(stream->time_base);
                if (duration > 0) {
                    frameInfo.bitrate = (packet->size * 8.0) / duration;
                }
            }
            lastPts = packet->pts;

            m_frameIndex.addFrame(frameInfo);

            if (frameNumber % 100 == 0) {
                emit indexingProgress(frameNumber, 0);
            }
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);

    // Seek back to beginning
    av_seek_frame(m_formatContext, m_videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(m_codecContext);

    m_indexBuilt = true;
    emit indexingComplete();

    qDebug() << "Frame index built:" << frameNumber << "frames";
    qDebug() << "I-frames:" << m_frameIndex.getIFrameCount();
    qDebug() << "P-frames:" << m_frameIndex.getPFrameCount();
    qDebug() << "B-frames:" << m_frameIndex.getBFrameCount();

    return true;
}

void VideoDecoder::freeResources() {
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }
    m_videoStreamIndex = -1;
}

int VideoDecoder::getWidth() const {
    return m_codecContext ? m_codecContext->width : 0;
}

int VideoDecoder::getHeight() const {
    return m_codecContext ? m_codecContext->height : 0;
}

double VideoDecoder::getDuration() const {
    if (!m_formatContext) {
        return 0.0;
    }
    return m_formatContext->duration / static_cast<double>(AV_TIME_BASE);
}

double VideoDecoder::getFrameRate() const {
    if (!m_formatContext || m_videoStreamIndex < 0) {
        return 0.0;
    }
    AVStream* stream = m_formatContext->streams[m_videoStreamIndex];
    return av_q2d(stream->r_frame_rate);
}

QString VideoDecoder::getCodecName() const {
    if (!m_codecContext) {
        return QString();
    }
    const AVCodec* codec = avcodec_find_decoder(m_codecContext->codec_id);
    return codec ? QString(codec->long_name) : QString();
}

QString VideoDecoder::getPixelFormat() const {
    if (!m_codecContext) {
        return QString();
    }
    return QString(av_get_pix_fmt_name(m_codecContext->pix_fmt));
}

} // namespace VideoStudio
