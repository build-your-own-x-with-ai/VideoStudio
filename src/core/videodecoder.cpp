#include "core/videodecoder.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>

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
    emit logMessage(QString("VideoDecoder::openFile called with: %1").arg(filePath));
    close();
    m_fileName = filePath;

    // Set format options for better TS file support
    AVDictionary* formatOpts = nullptr;
    av_dict_set(&formatOpts, "analyzeduration", "10000000", 0);  // 10 seconds
    av_dict_set(&formatOpts, "probesize", "10000000", 0);        // 10 MB

    emit logMessage("Opening input file with increased probe parameters...");
    // Open input file
    int ret = avformat_open_input(&m_formatContext, filePath.toUtf8().constData(), nullptr, &formatOpts);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        emit logMessage(QString("avformat_open_input failed: %1").arg(errbuf));
        av_dict_free(&formatOpts);
        emit error("Could not open file: " + filePath);
        return false;
    }
    av_dict_free(&formatOpts);
    emit logMessage("File opened successfully");

    // Retrieve stream information with increased limits
    m_formatContext->probesize = 10000000;      // 10 MB
    m_formatContext->max_analyze_duration = 10000000;  // 10 seconds in AV_TIME_BASE units

    emit logMessage(QString("Finding stream info with probesize: %1 max_analyze_duration: %2")
        .arg(m_formatContext->probesize).arg(m_formatContext->max_analyze_duration));
    ret = avformat_find_stream_info(m_formatContext, nullptr);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        emit logMessage(QString("avformat_find_stream_info failed: %1").arg(errbuf));
        emit error("Could not find stream information");
        close();
        return false;
    }
    emit logMessage(QString("Stream info found, nb_streams: %1").arg(m_formatContext->nb_streams));

    // Find video stream
    m_videoStreamIndex = -1;
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        emit logMessage(QString("Stream %1 type: %2 (VIDEO=%3)")
            .arg(i).arg(m_formatContext->streams[i]->codecpar->codec_type).arg(AVMEDIA_TYPE_VIDEO));
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = i;
            emit logMessage(QString("Found video stream at index: %1").arg(i));
            break;
        }
    }

    if (m_videoStreamIndex == -1) {
        emit logMessage("No video stream found!");
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

    // Enable motion vector export (critical for block statistics)
    m_codecContext->flags2 |= AV_CODEC_FLAG2_EXPORT_MVS;

    // Set codec options to export motion vectors
    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "flags2", "+export_mvs", 0);

    // Open codec
    if (avcodec_open2(m_codecContext, codec, &opts) < 0) {
        av_dict_free(&opts);
        emit error("Could not open codec");
        close();
        return false;
    }
    av_dict_free(&opts);

    emit logMessage(QString("Opened video file: %1").arg(filePath));
    emit logMessage(QString("Codec: %1").arg(codec->name));
    emit logMessage(QString("Resolution: %1 x %2").arg(m_codecContext->width).arg(m_codecContext->height));
    emit logMessage(QString("Frame rate: %1").arg(getFrameRate()));

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
        qDebug() << "decodeNextFrame: no codec context";
        return nullptr;
    }

    while (av_read_frame(m_formatContext, m_packet) >= 0) {
        if (m_packet->stream_index == m_videoStreamIndex) {
            // Send packet to decoder
            int ret = avcodec_send_packet(m_codecContext, m_packet);
            if (ret < 0) {
                qDebug() << "decodeNextFrame: avcodec_send_packet failed, ret=" << ret;
                av_packet_unref(m_packet);
                continue;
            }

            // Receive decoded frame
            ret = avcodec_receive_frame(m_codecContext, m_frame);
            if (ret == 0) {
                m_currentFrameNumber++;
                av_packet_unref(m_packet);
                qDebug() << "decodeNextFrame: successfully decoded frame" << m_currentFrameNumber;
                return m_frame;
            } else if (ret != AVERROR(EAGAIN)) {
                qDebug() << "decodeNextFrame: avcodec_receive_frame failed, ret=" << ret;
            }
        }
        av_packet_unref(m_packet);
    }

    qDebug() << "decodeNextFrame: no more packets, flushing decoder";
    // Flush decoder
    avcodec_send_packet(m_codecContext, nullptr);
    if (avcodec_receive_frame(m_codecContext, m_frame) == 0) {
        m_currentFrameNumber++;
        qDebug() << "decodeNextFrame: decoded frame from flush" << m_currentFrameNumber;
        return m_frame;
    }

    qDebug() << "decodeNextFrame: no more frames available";
    return nullptr;
}

bool VideoDecoder::seekToFrame(int frameNumber) {
    qDebug() << "seekToFrame called with frameNumber:" << frameNumber;
    if (!m_codecContext || frameNumber < 0 || frameNumber >= m_frameIndex.frameCount()) {
        qDebug() << "seekToFrame: invalid parameters - codecContext:" << (m_codecContext != nullptr)
                 << "frameNumber:" << frameNumber << "frameCount:" << m_frameIndex.frameCount();
        return false;
    }

    // Special case: seeking to frame 0, just seek to beginning of file
    if (frameNumber == 0) {
        qDebug() << "seekToFrame: seeking to beginning of file for frame 0";
        int ret = av_seek_frame(m_formatContext, m_videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_BYTE);
        if (ret < 0) {
            char errbuf[128];
            av_strerror(ret, errbuf, sizeof(errbuf));
            qDebug() << "seekToFrame: av_seek_frame to byte 0 failed:" << errbuf;
            // Try seeking to first frame's PTS as fallback
            const FrameInfo* frameInfo = m_frameIndex.getFrame(0);
            if (frameInfo) {
                ret = av_seek_frame(m_formatContext, m_videoStreamIndex, frameInfo->pts, AVSEEK_FLAG_BACKWARD);
                if (ret < 0) {
                    av_strerror(ret, errbuf, sizeof(errbuf));
                    qDebug() << "seekToFrame: av_seek_frame to PTS also failed:" << errbuf;
                    return false;
                }
            }
        }
        avcodec_flush_buffers(m_codecContext);
        m_currentFrameNumber = 0;
        qDebug() << "seekToFrame: successfully positioned at frame 0";
        return true;
    }

    const FrameInfo* frameInfo = m_frameIndex.getFrame(frameNumber);
    if (!frameInfo) {
        qDebug() << "seekToFrame: could not get frame info for frame" << frameNumber;
        return false;
    }

    // Find the nearest keyframe before target frame
    int keyframeIndex = frameNumber;
    while (keyframeIndex > 0) {
        const FrameInfo* kfInfo = m_frameIndex.getFrame(keyframeIndex);
        if (kfInfo && kfInfo->isKeyFrame) {
            break;
        }
        keyframeIndex--;
    }

    qDebug() << "seekToFrame: nearest keyframe is at" << keyframeIndex << "for target" << frameNumber;

    // Seek to keyframe
    const FrameInfo* keyframeInfo = m_frameIndex.getFrame(keyframeIndex);
    if (!keyframeInfo) {
        qDebug() << "seekToFrame: could not get keyframe info";
        return false;
    }

    int64_t timestamp = keyframeInfo->pts;
    qDebug() << "seekToFrame: seeking to keyframe PTS" << timestamp;

    // Use AVSEEK_FLAG_BACKWARD to ensure we land at or before the keyframe
    int ret = av_seek_frame(m_formatContext, m_videoStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "seekToFrame: av_seek_frame failed:" << errbuf;
        return false;
    }

    avcodec_flush_buffers(m_codecContext);
    qDebug() << "seekToFrame: flushed codec buffers";

    // Decode from keyframe to target frame
    int framesToDecode = frameNumber - keyframeIndex;
    qDebug() << "seekToFrame: need to decode" << framesToDecode << "frames from keyframe to target";

    AVFrame* decodedFrame = nullptr;
    for (int i = 0; i <= framesToDecode; i++) {
        decodedFrame = decodeNextFrame();
        if (!decodedFrame) {
            qDebug() << "seekToFrame: decodeNextFrame failed at frame" << i << "of" << framesToDecode;
            return false;
        }
    }

    m_currentFrameNumber = frameNumber;
    qDebug() << "seekToFrame: successfully reached frame" << frameNumber;
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
    AVFrame* frame = av_frame_alloc();

    // Estimate total frames from duration and frame rate
    int estimatedTotalFrames = 0;
    if (stream->duration != AV_NOPTS_VALUE && stream->avg_frame_rate.num > 0) {
        double duration = stream->duration * av_q2d(stream->time_base);
        double fps = av_q2d(stream->avg_frame_rate);
        estimatedTotalFrames = static_cast<int>(duration * fps);
    } else if (m_formatContext->duration != AV_NOPTS_VALUE && stream->avg_frame_rate.num > 0) {
        double duration = m_formatContext->duration / static_cast<double>(AV_TIME_BASE);
        double fps = av_q2d(stream->avg_frame_rate);
        estimatedTotalFrames = static_cast<int>(duration * fps);
    }

    int frameNumber = 0;
    int64_t lastDts = 0;

    while (av_read_frame(m_formatContext, packet) >= 0) {
        if (packet->stream_index == m_videoStreamIndex) {
            FrameInfo frameInfo;
            frameInfo.frameNumber = frameNumber;
            frameInfo.pts = packet->pts;
            frameInfo.dts = packet->dts;
            frameInfo.offset = packet->pos;
            frameInfo.size = packet->size;
            // isKeyFrame will be set after decoding based on frame type

            // Calculate timestamp (use PTS for display time)
            if (packet->pts != AV_NOPTS_VALUE) {
                frameInfo.timestamp = packet->pts * av_q2d(stream->time_base);
            }

            // Calculate bitrate (use DTS for decode order, which is monotonic)
            if (packet->dts != AV_NOPTS_VALUE && lastDts != 0) {
                double duration = (packet->dts - lastDts) * av_q2d(stream->time_base);
                if (duration > 0) {
                    frameInfo.bitrate = (packet->size * 8.0) / duration;
                }
            }
            lastDts = packet->dts;

            // Decode frame to get frame type
            if (avcodec_send_packet(m_codecContext, packet) >= 0) {
                if (avcodec_receive_frame(m_codecContext, frame) == 0) {
                    frameInfo.frameType = frame->pict_type;
                    frameInfo.qp = frame->quality;

                    // I-frames should always be key frames
                    // Use frame type as the authoritative source
                    if (frame->pict_type == AV_PICTURE_TYPE_I) {
                        frameInfo.isKeyFrame = true;
                    } else {
                        // For non-I frames, trust the packet flag
                        frameInfo.isKeyFrame = (packet->flags & AV_PKT_FLAG_KEY) != 0;
                    }

                    // Only add frame if we successfully decoded it
                    m_frameIndex.addFrame(frameInfo);
                    frameNumber++;

                    // Emit progress every 10 frames for smoother updates
                    if (frameNumber % 10 == 0) {
                        emit indexingProgress(frameNumber, estimatedTotalFrames);
                    }
                }
            }
        }
        av_packet_unref(packet);
    }

    // Flush decoder to get remaining frames
    avcodec_send_packet(m_codecContext, nullptr);
    while (avcodec_receive_frame(m_codecContext, frame) == 0) {
        // These are delayed frames - we already counted them in the index
    }

    av_frame_free(&frame);
    av_packet_free(&packet);

    // Seek back to beginning and reset decoder state
    av_seek_frame(m_formatContext, m_videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(m_codecContext);

    // Reset frame counter so next decode starts from frame 0
    m_currentFrameNumber = 0;

    m_indexBuilt = true;
    emit indexingComplete();

    emit logMessage(QString("Frame index built: %1 frames").arg(frameNumber));
    emit logMessage(QString("I-frames: %1").arg(m_frameIndex.getIFrameCount()));
    emit logMessage(QString("P-frames: %1").arg(m_frameIndex.getPFrameCount()));
    emit logMessage(QString("B-frames: %1").arg(m_frameIndex.getBFrameCount()));

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

int64_t VideoDecoder::getBitrate() const {
    if (!m_formatContext || m_videoStreamIndex < 0) {
        return 0;
    }

    // Try to get bitrate from codec context
    if (m_codecContext && m_codecContext->bit_rate > 0) {
        return m_codecContext->bit_rate;
    }

    // Try to get bitrate from stream
    AVStream* stream = m_formatContext->streams[m_videoStreamIndex];
    if (stream->codecpar && stream->codecpar->bit_rate > 0) {
        return stream->codecpar->bit_rate;
    }

    // Estimate from file size and duration
    if (m_formatContext->bit_rate > 0) {
        return m_formatContext->bit_rate;
    }

    return 0;
}

int64_t VideoDecoder::getFrameSize(int frameNumber) const {
    if (!m_formatContext || m_videoStreamIndex < 0 ||
        frameNumber < 0 || frameNumber >= m_frameIndex.frameCount()) {
        return 0;
    }

    // Get frame info from index
    const FrameInfo* frameInfo = m_frameIndex.getFrame(frameNumber);
    if (frameInfo) {
        return frameInfo->size;
    }

    return 0;
}

bool VideoDecoder::exportFrameAsYUV(int frameNumber, const QString& filePath) {
    if (!isOpen() || frameNumber < 0 || frameNumber >= getFrameCount()) {
        emit logMessage(QString("Export failed: invalid frame number %1").arg(frameNumber));
        return false;
    }

    // Save current position
    int originalFrame = m_currentFrameNumber;

    // For frame 0, just seek to beginning and decode
    if (frameNumber == 0) {
        if (!seekToFrame(0)) {
            emit logMessage(QString("Export failed: could not seek to frame 0"));
            return false;
        }
        AVFrame* frame = decodeNextFrame();
        if (!frame) {
            emit logMessage(QString("Export failed: could not decode frame 0"));
            return false;
        }

        // Write YUV data
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit logMessage(QString("Export failed: could not open file %1").arg(filePath));
            seekToFrame(originalFrame);
            return false;
        }

        // Y plane
        for (int y = 0; y < frame->height; y++) {
            file.write(reinterpret_cast<const char*>(frame->data[0] + y * frame->linesize[0]), frame->width);
        }
        // U plane
        for (int y = 0; y < frame->height / 2; y++) {
            file.write(reinterpret_cast<const char*>(frame->data[1] + y * frame->linesize[1]), frame->width / 2);
        }
        // V plane
        for (int y = 0; y < frame->height / 2; y++) {
            file.write(reinterpret_cast<const char*>(frame->data[2] + y * frame->linesize[2]), frame->width / 2);
        }
        file.close();

        emit logMessage(QString("Exported frame 0 to %1 (%2x%3 YUV420P)")
            .arg(filePath).arg(frame->width).arg(frame->height));

        // Restore original position
        if (originalFrame != 0) {
            seekToFrame(originalFrame);
        }
        return true;
    }

    // For other frames, seek to frame-1 and decode twice
    // This ensures we get the correct frame
    int seekTarget = (frameNumber > 0) ? frameNumber - 1 : 0;
    if (!seekToFrame(seekTarget)) {
        emit logMessage(QString("Export failed: could not seek to frame %1").arg(seekTarget));
        return false;
    }

    // Decode frames until we reach the target
    AVFrame* frame = nullptr;
    for (int i = seekTarget; i <= frameNumber; i++) {
        frame = decodeNextFrame();
        if (!frame) {
            emit logMessage(QString("Export failed: could not decode frame %1").arg(i));
            if (originalFrame != frameNumber) {
                seekToFrame(originalFrame);
            }
            return false;
        }
    }

    // Now we have the target frame
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit logMessage(QString("Export failed: could not open file %1").arg(filePath));
        if (originalFrame != frameNumber) {
            seekToFrame(originalFrame);
        }
        return false;
    }

    // Write YUV data (assuming YUV420P format)
    // Y plane
    for (int y = 0; y < frame->height; y++) {
        file.write(reinterpret_cast<const char*>(frame->data[0] + y * frame->linesize[0]), frame->width);
    }

    // U plane
    for (int y = 0; y < frame->height / 2; y++) {
        file.write(reinterpret_cast<const char*>(frame->data[1] + y * frame->linesize[1]), frame->width / 2);
    }

    // V plane
    for (int y = 0; y < frame->height / 2; y++) {
        file.write(reinterpret_cast<const char*>(frame->data[2] + y * frame->linesize[2]), frame->width / 2);
    }

    file.close();

    emit logMessage(QString("Exported frame %1 to %2 (%3x%4 YUV420P)")
        .arg(frameNumber)
        .arg(filePath)
        .arg(frame->width)
        .arg(frame->height));

    // Restore original position
    if (originalFrame != frameNumber) {
        seekToFrame(originalFrame);
    }

    return true;
}

bool VideoDecoder::exportFrameRangeAsYUV(int startFrame, int endFrame, const QString& outputDir) {
    if (!isOpen() || startFrame < 0 || endFrame >= getFrameCount() || startFrame > endFrame) {
        emit logMessage(QString("Export failed: invalid frame range %1-%2").arg(startFrame).arg(endFrame));
        return false;
    }

    // Save current position
    int originalFrame = m_currentFrameNumber;

    // Seek to start frame
    if (!seekToFrame(startFrame)) {
        emit logMessage(QString("Export failed: could not seek to frame %1").arg(startFrame));
        return false;
    }

    int exportedCount = 0;
    for (int frameNum = startFrame; frameNum <= endFrame; frameNum++) {
        QString fileName = QString("frame_%1.yuv").arg(frameNum, 6, 10, QChar('0'));
        QString filePath = outputDir + "/" + fileName;

        // Decode next frame
        AVFrame* frame = decodeNextFrame();
        if (!frame) {
            emit logMessage(QString("Failed to decode frame %1").arg(frameNum));
            break;
        }

        // Open output file
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit logMessage(QString("Failed to open file %1").arg(filePath));
            continue;
        }

        // Write YUV data (assuming YUV420P format)
        // Y plane
        for (int y = 0; y < frame->height; y++) {
            file.write(reinterpret_cast<const char*>(frame->data[0] + y * frame->linesize[0]), frame->width);
        }

        // U plane
        for (int y = 0; y < frame->height / 2; y++) {
            file.write(reinterpret_cast<const char*>(frame->data[1] + y * frame->linesize[1]), frame->width / 2);
        }

        // V plane
        for (int y = 0; y < frame->height / 2; y++) {
            file.write(reinterpret_cast<const char*>(frame->data[2] + y * frame->linesize[2]), frame->width / 2);
        }

        file.close();
        exportedCount++;

        // Log progress every 10 frames
        if (exportedCount % 10 == 0 || frameNum == endFrame) {
            emit logMessage(QString("Exported %1 / %2 frames...")
                .arg(exportedCount)
                .arg(endFrame - startFrame + 1));
        }
    }

    emit logMessage(QString("Exported %1 of %2 frames to %3")
        .arg(exportedCount)
        .arg(endFrame - startFrame + 1)
        .arg(outputDir));

    // Restore original position
    if (originalFrame != endFrame + 1) {
        seekToFrame(originalFrame);
    }

    return exportedCount > 0;
}

} // namespace VideoStudio
