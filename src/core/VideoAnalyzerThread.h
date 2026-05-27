#ifndef VIDEOANALYZERTHREAD_H
#define VIDEOANALYZERTHREAD_H

#include <QThread>
#include <QImage>
#include "VideoDecoder.h"
#include "MetricsCollector.h"

class VideoAnalyzerThread : public QThread {
    Q_OBJECT

public:
    explicit VideoAnalyzerThread(const QString& filePath, QObject* parent = nullptr);
    ~VideoAnalyzerThread();

    void stop();
    const QVector<FrameInfo>& getFrames() const { return frames; }
    const QImage& getFirstFrameImage() const { return firstFrameImage; }

signals:
    void progressUpdated(int current, int total);
    void analysisComplete();
    void analysisFailed(const QString& error);

protected:
    void run() override;

private:
    QString filePath;
    QVector<FrameInfo> frames;
    QImage firstFrameImage;
    bool shouldStop;
};

#endif // VIDEOANALYZERTHREAD_H
