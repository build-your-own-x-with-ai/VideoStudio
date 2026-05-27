#include "StreamInfoPanel.h"
#include <QGroupBox>
#include <QVBoxLayout>

StreamInfoPanel::StreamInfoPanel(QWidget* parent)
    : QWidget(parent) {
    setupUI();
}

void StreamInfoPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QGroupBox* videoGroup = new QGroupBox("视频信息", this);
    QGridLayout* gridLayout = new QGridLayout(videoGroup);

    int row = 0;
    gridLayout->addWidget(new QLabel("编码器:"), row, 0);
    codecLabel = new QLabel("-");
    codecLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gridLayout->addWidget(codecLabel, row++, 1);

    gridLayout->addWidget(new QLabel("分辨率:"), row, 0);
    resolutionLabel = new QLabel("-");
    resolutionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gridLayout->addWidget(resolutionLabel, row++, 1);

    gridLayout->addWidget(new QLabel("帧率:"), row, 0);
    frameRateLabel = new QLabel("-");
    frameRateLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gridLayout->addWidget(frameRateLabel, row++, 1);

    gridLayout->addWidget(new QLabel("比特率:"), row, 0);
    bitrateLabel = new QLabel("-");
    bitrateLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gridLayout->addWidget(bitrateLabel, row++, 1);

    gridLayout->addWidget(new QLabel("时长:"), row, 0);
    durationLabel = new QLabel("-");
    durationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gridLayout->addWidget(durationLabel, row++, 1);

    gridLayout->addWidget(new QLabel("总帧数:"), row, 0);
    numFramesLabel = new QLabel("-");
    numFramesLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gridLayout->addWidget(numFramesLabel, row++, 1);

    gridLayout->addWidget(new QLabel("像素格式:"), row, 0);
    pixelFormatLabel = new QLabel("-");
    pixelFormatLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gridLayout->addWidget(pixelFormatLabel, row++, 1);

    gridLayout->addWidget(new QLabel("容器格式:"), row, 0);
    containerLabel = new QLabel("-");
    containerLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gridLayout->addWidget(containerLabel, row++, 1);

    gridLayout->setColumnStretch(1, 1);

    mainLayout->addWidget(videoGroup);
    mainLayout->addStretch();
}

void StreamInfoPanel::setStreamInfo(const StreamInfo& info) {
    currentStreamInfo = info;

    codecLabel->setText(QString("%1 (%2)")
        .arg(info.codecName)
        .arg(info.codecLongName));

    resolutionLabel->setText(QString("%1 x %2")
        .arg(info.width)
        .arg(info.height));

    frameRateLabel->setText(QString("%1 fps")
        .arg(info.frameRate, 0, 'f', 2));

    bitrateLabel->setText(formatBitrate(info.bitrate));
    durationLabel->setText(formatDuration(info.duration));

    numFramesLabel->setText(QString::number(info.numFrames));
    pixelFormatLabel->setText(info.pixelFormat);
    containerLabel->setText(info.containerFormat);
}

void StreamInfoPanel::clear() {
    currentStreamInfo = StreamInfo();
    codecLabel->setText("-");
    resolutionLabel->setText("-");
    frameRateLabel->setText("-");
    bitrateLabel->setText("-");
    durationLabel->setText("-");
    numFramesLabel->setText("-");
    pixelFormatLabel->setText("-");
    containerLabel->setText("-");
}

QString StreamInfoPanel::formatBitrate(int64_t bitrate) {
    if (bitrate <= 0) {
        return "未知";
    }

    double kbps = bitrate / 1000.0;
    if (kbps < 1000) {
        return QString("%1 kbps").arg(kbps, 0, 'f', 0);
    }

    double mbps = kbps / 1000.0;
    return QString("%1 Mbps").arg(mbps, 0, 'f', 2);
}

QString StreamInfoPanel::formatDuration(int64_t seconds) {
    if (seconds <= 0) {
        return "未知";
    }

    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;

    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes)
            .arg(secs, 2, 10, QChar('0'));
    }
}
