#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QString>
#include <QObject>
#include <memory>
#include "framedata.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace VideoStudio {

class VideoDecoder : public QObject {
    Q_OBJECT

public:
    explicit VideoDecoder(QObject* parent = nullptr);
    ~VideoDecoder();

    bool openFile(const QString& filePath);
    void close();
    bool isOpen() const { return m_formatContext != nullptr; }

    AVFrame* decodeNextFrame();
    bool seekToFrame(int frameNumber);
    bool seekToTime(double seconds);

    // Getters
    AVCodecContext* getCodecContext() const { return m_codecContext; }
    AVFormatContext* getFormatContext() const { return m_formatContext; }
    const FrameIndex& getFrameIndex() const { return m_frameIndex; }

    int getWidth() const;
    int getHeight() const;
    int getFrameCount() const { return m_frameIndex.frameCount(); }
    double getDuration() const;
    double getFrameRate() const;
    QString getCodecName() const;
    QString getPixelFormat() const;

    int getCurrentFrameNumber() const { return m_currentFrameNumber; }

signals:
    void indexingProgress(int current, int total);
    void indexingComplete();
    void error(const QString& message);

private:
    bool buildFrameIndex();
    void freeResources();

    AVFormatContext* m_formatContext;
    AVCodecContext* m_codecContext;
    AVFrame* m_frame;
    AVPacket* m_packet;
    int m_videoStreamIndex;

    FrameIndex m_frameIndex;
    int m_currentFrameNumber;
    bool m_indexBuilt;
};

} // namespace VideoStudio

#endif // VIDEODECODER_H
