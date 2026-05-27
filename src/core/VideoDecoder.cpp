#include "VideoDecoder.h"
#include <QDebug>

VideoDecoder::VideoDecoder()
    : formatCtx(nullptr), codecCtx(nullptr), frame(nullptr),
      packet(nullptr), swsCtx(nullptr), videoStreamIndex(-1), frameCounter(0),
      currentFilePath("") {
}

VideoDecoder::~VideoDecoder() {
    close();
}

bool VideoDecoder::open(const QString& filePath) {
    close();

    currentFilePath = filePath;

    formatCtx = avformat_alloc_context();
    if (avformat_open_input(&formatCtx, filePath.toUtf8().constData(), nullptr, nullptr) != 0) {
        qWarning() << "无法打开文件:" << filePath;
        return false;
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        qWarning() << "无法获取流信息";
        close();
        return false;
    }

    videoStreamIndex = -1;
    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        qWarning() << "未找到视频流";
        close();
        return false;
    }

    AVCodecParameters* codecParams = formatCtx->streams[videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        qWarning() << "不支持的编码器";
        close();
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecCtx, codecParams) < 0) {
        qWarning() << "无法复制编码器参数";
        close();
        return false;
    }

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        qWarning() << "无法打开编码器";
        close();
        return false;
    }

    frame = av_frame_alloc();
    packet = av_packet_alloc();
    frameCounter = 0;

    qDebug() << "成功打开视频:" << filePath;
    return true;
}

bool VideoDecoder::readNextFrame(FrameInfo& frameInfo) {
    if (!isOpen()) {
        return false;
    }

    while (av_read_frame(formatCtx, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            int ret = avcodec_send_packet(codecCtx, packet);
            if (ret < 0) {
                av_packet_unref(packet);
                continue;
            }

            ret = avcodec_receive_frame(codecCtx, frame);
            if (ret == 0) {
                frameInfo.frameNumber = frameCounter++;
                frameInfo.pts = frame->pts;
                frameInfo.dts = packet->dts;
                frameInfo.size = packet->size;
                frameInfo.isKeyFrame = (frame->flags & AV_FRAME_FLAG_KEY);

                AVStream* stream = formatCtx->streams[videoStreamIndex];
                frameInfo.timestamp = frame->pts * av_q2d(stream->time_base);

                switch (frame->pict_type) {
                    case AV_PICTURE_TYPE_I: frameInfo.frameType = 'I'; break;
                    case AV_PICTURE_TYPE_P: frameInfo.frameType = 'P'; break;
                    case AV_PICTURE_TYPE_B: frameInfo.frameType = 'B'; break;
                    default: frameInfo.frameType = '?'; break;
                }

                av_packet_unref(packet);
                return true;
            }
        }
        av_packet_unref(packet);
    }

    return false;
}

StreamInfo VideoDecoder::getStreamInfo() const {
    StreamInfo info;

    if (!isOpen()) {
        return info;
    }

    AVStream* stream = formatCtx->streams[videoStreamIndex];
    AVCodecParameters* codecParams = stream->codecpar;

    info.codecName = QString(avcodec_get_name(codecParams->codec_id));
    const AVCodecDescriptor* desc = avcodec_descriptor_get(codecParams->codec_id);
    if (desc) {
        info.codecLongName = QString(desc->long_name);
    }

    info.width = codecParams->width;
    info.height = codecParams->height;
    info.bitrate = codecParams->bit_rate;

    AVRational frameRate = av_guess_frame_rate(formatCtx, stream, nullptr);
    if (frameRate.num && frameRate.den) {
        info.frameRate = av_q2d(frameRate);
    }

    info.pixelFormat = QString(av_get_pix_fmt_name((AVPixelFormat)codecParams->format));
    info.duration = formatCtx->duration / AV_TIME_BASE;
    info.containerFormat = QString(formatCtx->iformat->long_name);

    if (stream->nb_frames > 0) {
        info.numFrames = stream->nb_frames;
    } else if (info.duration > 0 && info.frameRate > 0) {
        info.numFrames = static_cast<int64_t>(info.duration * info.frameRate);
    }

    return info;
}

QImage VideoDecoder::getCurrentFrameImage() {
    if (!isOpen() || !frame->data[0]) {
        return QImage();
    }

    int width = codecCtx->width;
    int height = codecCtx->height;

    swsCtx = sws_getCachedContext(swsCtx,
        width, height, codecCtx->pix_fmt,
        width, height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsCtx) {
        return QImage();
    }

    QImage image(width, height, QImage::Format_RGB888);
    uint8_t* dest[1] = { image.bits() };
    int destLinesize[1] = { static_cast<int>(image.bytesPerLine()) };

    sws_scale(swsCtx, frame->data, frame->linesize, 0, height, dest, destLinesize);

    return image;
}

void VideoDecoder::seek(int64_t timestamp) {
    if (!isOpen()) {
        return;
    }

    av_seek_frame(formatCtx, videoStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codecCtx);
}

void VideoDecoder::close() {
    freeResources();
}

void VideoDecoder::freeResources() {
    if (frame) {
        av_frame_free(&frame);
    }
    if (packet) {
        av_packet_free(&packet);
    }
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
    }
    if (formatCtx) {
        avformat_close_input(&formatCtx);
    }
    if (swsCtx) {
        sws_freeContext(swsCtx);
        swsCtx = nullptr;
    }
    videoStreamIndex = -1;
    frameCounter = 0;
}
