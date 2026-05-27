#ifndef STREAMINFOPANEL_H
#define STREAMINFOPANEL_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include "core/StreamInfo.h"

class StreamInfoPanel : public QWidget {
    Q_OBJECT

public:
    explicit StreamInfoPanel(QWidget* parent = nullptr);
    void setStreamInfo(const StreamInfo& info);
    StreamInfo getStreamInfo() const { return currentStreamInfo; }
    void clear();

private:
    QLabel* codecLabel;
    QLabel* resolutionLabel;
    QLabel* frameRateLabel;
    QLabel* bitrateLabel;
    QLabel* durationLabel;
    QLabel* pixelFormatLabel;
    QLabel* containerLabel;
    QLabel* numFramesLabel;
    StreamInfo currentStreamInfo;

    void setupUI();
    QString formatBitrate(int64_t bitrate);
    QString formatDuration(int64_t seconds);
};

#endif // STREAMINFOPANEL_H
