#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QString>
#include <QImage>
#include "FrameInfo.h"
#include "StreamInfo.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();

    bool open(const QString& filePath);
    bool readNextFrame(FrameInfo& frameInfo);
    StreamInfo getStreamInfo() const;
    QImage getCurrentFrameImage();
    void seek(int64_t timestamp);
    void close();

    bool isOpen() const { return formatCtx != nullptr; }

private:
    AVFormatContext* formatCtx;
    AVCodecContext* codecCtx;
    AVFrame* frame;
    AVPacket* packet;
    SwsContext* swsCtx;
    int videoStreamIndex;
    int64_t frameCounter;

    void freeResources();
};

#endif // VIDEODECODER_H
