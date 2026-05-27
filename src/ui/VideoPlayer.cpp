#include "VideoPlayer.h"
#include <QComboBox>
#include <QDateTime>
#include <QResizeEvent>
#include <QSizePolicy>

VideoPlayer::VideoPlayer(QWidget* parent)
    : QWidget(parent), decoder(nullptr), macroblockAnalyzer(nullptr),
      qualityHeatmapAnalyzer(nullptr), referenceDecoder(nullptr),
      videoOpen(false), playing(false),
      currentFrameNumber(0), totalFrames(0), frameRate(0.0), playbackSpeed(1.0),
      lastFrameTime(0) {
    decoder = new VideoDecoder();
    macroblockAnalyzer = new MacroblockAnalyzer();
    qualityHeatmapAnalyzer = new QualityHeatmapAnalyzer();
    referenceDecoder = new VideoDecoder();
    playbackTimer = new QTimer(this);
    connect(playbackTimer, &QTimer::timeout, this, &VideoPlayer::onPlaybackTimer);
    setupUI();
}

VideoPlayer::~VideoPlayer() {
    closeVideo();
    delete decoder;
    delete macroblockAnalyzer;
    delete qualityHeatmapAnalyzer;
    delete referenceDecoder;
}

void VideoPlayer::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Video display area with overlay
    QWidget* videoContainer = new QWidget(this);
    videoContainer->setMinimumSize(640, 360);

    // Use a layout for the video container to make videoDisplay fill it
    QVBoxLayout* containerLayout = new QVBoxLayout(videoContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    videoDisplay = new QLabel(videoContainer);
    videoDisplay->setAlignment(Qt::AlignCenter);
    videoDisplay->setStyleSheet("QLabel { background-color: #2b2b2b; color: #888; border: 1px solid #555; }");
    videoDisplay->setText("未打开视频文件");
    videoDisplay->setScaledContents(false);
    videoDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    containerLayout->addWidget(videoDisplay);

    // Create macroblock overlay on top of video display
    macroblockOverlay = new MacroblockOverlay(videoContainer);
    macroblockOverlay->raise();

    // Create quality heatmap overlay on top of macroblock overlay
    qualityHeatmapOverlay = new QualityHeatmapOverlay(videoContainer);
    qualityHeatmapOverlay->raise();

    mainLayout->addWidget(videoContainer, 1);

    // Control panel
    QWidget* controlPanel = new QWidget(this);
    QVBoxLayout* controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setContentsMargins(5, 5, 5, 5);

    // Progress slider
    progressSlider = new QSlider(Qt::Horizontal, this);
    progressSlider->setEnabled(false);
    progressSlider->setMinimum(0);
    progressSlider->setMaximum(0);
    connect(progressSlider, &QSlider::sliderMoved, this, &VideoPlayer::onProgressSliderMoved);
    controlLayout->addWidget(progressSlider);

    // Button row
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    previousFrameButton = new QPushButton("⏮ 上一帧", this);
    previousFrameButton->setEnabled(false);
    connect(previousFrameButton, &QPushButton::clicked, this, &VideoPlayer::onPreviousFrameClicked);
    buttonLayout->addWidget(previousFrameButton);

    playPauseButton = new QPushButton("▶ 播放", this);
    playPauseButton->setEnabled(false);
    connect(playPauseButton, &QPushButton::clicked, this, &VideoPlayer::onPlayPauseClicked);
    buttonLayout->addWidget(playPauseButton);

    stopButton = new QPushButton("⏹ 停止", this);
    stopButton->setEnabled(false);
    connect(stopButton, &QPushButton::clicked, this, &VideoPlayer::onStopClicked);
    buttonLayout->addWidget(stopButton);

    nextFrameButton = new QPushButton("下一帧 ⏭", this);
    nextFrameButton->setEnabled(false);
    connect(nextFrameButton, &QPushButton::clicked, this, &VideoPlayer::onNextFrameClicked);
    buttonLayout->addWidget(nextFrameButton);

    buttonLayout->addStretch();

    // Speed control
    QLabel* speedLabel = new QLabel("速度:", this);
    buttonLayout->addWidget(speedLabel);

    speedComboBox = new QComboBox(this);
    speedComboBox->addItem("0.25x", 0.25);
    speedComboBox->addItem("0.5x", 0.5);
    speedComboBox->addItem("1x", 1.0);
    speedComboBox->addItem("1.5x", 1.5);
    speedComboBox->addItem("2x", 2.0);
    speedComboBox->setCurrentIndex(2); // 1x
    speedComboBox->setEnabled(false);
    connect(speedComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VideoPlayer::onSpeedChanged);
    buttonLayout->addWidget(speedComboBox);

    // Time label
    timeLabel = new QLabel("00:00 / 00:00", this);
    buttonLayout->addWidget(timeLabel);

    controlLayout->addLayout(buttonLayout);
    mainLayout->addWidget(controlPanel);
}

bool VideoPlayer::openVideo(const QString& filePath) {
    closeVideo();

    if (!decoder->open(filePath)) {
        return false;
    }

    StreamInfo info = decoder->getStreamInfo();
    totalFrames = info.numFrames;
    frameRate = info.frameRate;
    currentFrameNumber = 0;

    progressSlider->setMaximum(totalFrames - 1);
    progressSlider->setEnabled(true);
    playPauseButton->setEnabled(true);
    stopButton->setEnabled(true);
    previousFrameButton->setEnabled(true);
    nextFrameButton->setEnabled(true);
    speedComboBox->setEnabled(true);

    videoOpen = true;

    // Load first frame
    FrameInfo frameInfo;
    if (decoder->readNextFrame(frameInfo)) {
        QImage image = decoder->getCurrentFrameImage();
        if (!image.isNull()) {
            videoDisplay->setPixmap(QPixmap::fromImage(image).scaled(
                videoDisplay->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    updateTimeLabel();
    return true;
}

void VideoPlayer::closeVideo() {
    stop();
    decoder->close();
    videoOpen = false;
    currentFrameNumber = 0;
    totalFrames = 0;
    frameRate = 0.0;

    videoDisplay->clear();
    videoDisplay->setText("未打开视频文件");
    progressSlider->setValue(0);
    progressSlider->setMaximum(0);
    progressSlider->setEnabled(false);
    playPauseButton->setEnabled(false);
    stopButton->setEnabled(false);
    previousFrameButton->setEnabled(false);
    nextFrameButton->setEnabled(false);
    speedComboBox->setEnabled(false);
    timeLabel->setText("00:00 / 00:00");
}

void VideoPlayer::play() {
    if (!videoOpen || playing) {
        return;
    }

    playing = true;
    lastFrameTime = QDateTime::currentMSecsSinceEpoch();
    int interval = static_cast<int>(1000.0 / (frameRate * playbackSpeed));
    playbackTimer->start(interval);
    playPauseButton->setText("⏸ 暂停");
    emit playbackStateChanged(true);
}

void VideoPlayer::pause() {
    if (!playing) {
        return;
    }

    playing = false;
    playbackTimer->stop();
    playPauseButton->setText("▶ 播放");
    emit playbackStateChanged(false);
}

void VideoPlayer::stop() {
    pause();
    seekToFrame(0);
}

void VideoPlayer::seekToFrame(int frameNumber) {
    if (!videoOpen || frameNumber < 0 || frameNumber >= totalFrames) {
        return;
    }

    // Close and reopen to seek
    QString currentPath = decoder->getFilePath();
    decoder->close();
    decoder->open(currentPath);

    // Read frames until target
    FrameInfo frameInfo;
    for (int i = 0; i <= frameNumber; i++) {
        if (!decoder->readNextFrame(frameInfo)) {
            break;
        }
    }

    currentFrameNumber = frameNumber;
    updateFrame();
    updateControls();
    emit frameChanged(currentFrameNumber);
    emit positionChanged(getCurrentTime());
}

void VideoPlayer::seekToTime(double seconds) {
    if (!videoOpen || frameRate <= 0) {
        return;
    }

    int targetFrame = static_cast<int>(seconds * frameRate);
    seekToFrame(targetFrame);
}

void VideoPlayer::nextFrame() {
    if (!videoOpen) {
        return;
    }

    if (currentFrameNumber < totalFrames - 1) {
        FrameInfo frameInfo;
        if (decoder->readNextFrame(frameInfo)) {
            currentFrameNumber++;
            updateFrame();
            updateControls();
            emit frameChanged(currentFrameNumber);
            emit positionChanged(getCurrentTime());
        }
    }
}

void VideoPlayer::previousFrame() {
    if (!videoOpen || currentFrameNumber <= 0) {
        return;
    }

    seekToFrame(currentFrameNumber - 1);
}

void VideoPlayer::setPlaybackSpeed(double speed) {
    playbackSpeed = speed;
    if (playing) {
        int interval = static_cast<int>(1000.0 / (frameRate * playbackSpeed));
        playbackTimer->setInterval(interval);
    }
}

QPixmap VideoPlayer::getCurrentFramePixmap() const {
    return videoDisplay->pixmap();
}

double VideoPlayer::getCurrentTime() const {
    if (!videoOpen || frameRate <= 0) {
        return 0.0;
    }
    return currentFrameNumber / frameRate;
}

double VideoPlayer::getDuration() const {
    if (!videoOpen || frameRate <= 0) {
        return 0.0;
    }
    return totalFrames / frameRate;
}

void VideoPlayer::onPlaybackTimer() {
    if (!playing || !videoOpen) {
        return;
    }

    if (currentFrameNumber >= totalFrames - 1) {
        pause();
        return;
    }

    nextFrame();
}

void VideoPlayer::onProgressSliderMoved(int value) {
    seekToFrame(value);
}

void VideoPlayer::onPlayPauseClicked() {
    if (playing) {
        pause();
    } else {
        play();
    }
}

void VideoPlayer::onStopClicked() {
    stop();
}

void VideoPlayer::onPreviousFrameClicked() {
    previousFrame();
}

void VideoPlayer::onNextFrameClicked() {
    nextFrame();
}

void VideoPlayer::onSpeedChanged(int index) {
    double speed = speedComboBox->itemData(index).toDouble();
    setPlaybackSpeed(speed);
}

void VideoPlayer::updateFrame() {
    QImage image = decoder->getCurrentFrameImage();
    if (!image.isNull()) {
        videoDisplay->setPixmap(QPixmap::fromImage(image).scaled(
            videoDisplay->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

        // Update macroblock overlay position and size
        if (macroblockOverlay) {
            macroblockOverlay->setGeometry(videoDisplay->geometry());

            // Update macroblock data if any overlay is enabled
            if (macroblockOverlay->isShowingBoundaries() ||
                macroblockOverlay->isShowingMotionVectors() ||
                macroblockOverlay->isShowingQPHeatmap()) {
                AVFrame* frame = decoder->getCurrentFrame();
                if (frame) {
                    QVector<MacroblockInfo> mbs = macroblockAnalyzer->extractMacroblocks(frame);
                    macroblockOverlay->setMacroblocks(mbs);
                }
            }
        }

        // Update quality heatmap overlay
        if (qualityHeatmapOverlay && qualityHeatmapOverlay->getHeatmapMode() != QualityHeatmapOverlay::None) {
            qualityHeatmapOverlay->setGeometry(videoDisplay->geometry());
            updateQualityHeatmap();
        }
    }
}

void VideoPlayer::updateControls() {
    progressSlider->blockSignals(true);
    progressSlider->setValue(currentFrameNumber);
    progressSlider->blockSignals(false);
    updateTimeLabel();
}

void VideoPlayer::updateTimeLabel() {
    QString current = formatTime(getCurrentTime());
    QString duration = formatTime(getDuration());
    timeLabel->setText(QString("%1 / %2").arg(current).arg(duration));
}

QString VideoPlayer::formatTime(double seconds) {
    int totalSecs = static_cast<int>(seconds);
    int hours = totalSecs / 3600;
    int minutes = (totalSecs % 3600) / 60;
    int secs = totalSecs % 60;

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

void VideoPlayer::setShowMacroblockBoundaries(bool show) {
    if (macroblockOverlay) {
        macroblockOverlay->setShowBoundaries(show);
        if (show && decoder->isOpen()) {
            // Extract and update macroblocks for current frame
            AVFrame* frame = decoder->getCurrentFrame();
            if (frame) {
                QVector<MacroblockInfo> mbs = macroblockAnalyzer->extractMacroblocks(frame);
                macroblockOverlay->setMacroblocks(mbs);
            }
        }
    }
}

void VideoPlayer::setShowMotionVectors(bool show) {
    if (macroblockOverlay) {
        macroblockOverlay->setShowMotionVectors(show);
        if (show && decoder->isOpen()) {
            // Extract and update macroblocks for current frame
            AVFrame* frame = decoder->getCurrentFrame();
            if (frame) {
                QVector<MacroblockInfo> mbs = macroblockAnalyzer->extractMacroblocks(frame);
                macroblockOverlay->setMacroblocks(mbs);
            }
        }
    }
}

void VideoPlayer::setShowQPHeatmap(bool show) {
    if (macroblockOverlay) {
        macroblockOverlay->setShowQPHeatmap(show);
        if (show && decoder->isOpen()) {
            // Extract and update macroblocks for current frame
            AVFrame* frame = decoder->getCurrentFrame();
            if (frame) {
                QVector<MacroblockInfo> mbs = macroblockAnalyzer->extractMacroblocks(frame);
                macroblockOverlay->setMacroblocks(mbs);
            }
        }
    }
}

void VideoPlayer::setShowSizes(bool show) {
    if (macroblockOverlay) {
        macroblockOverlay->setShowSizes(show);
        if (show && decoder->isOpen()) {
            // Extract and update macroblocks for current frame
            AVFrame* frame = decoder->getCurrentFrame();
            if (frame) {
                QVector<MacroblockInfo> mbs = macroblockAnalyzer->extractMacroblocks(frame);
                macroblockOverlay->setMacroblocks(mbs);
            }
        }
    }
}

void VideoPlayer::setShowExtendedParams(bool show) {
    if (macroblockOverlay) {
        macroblockOverlay->setShowExtendedParams(show);
        if (show && decoder->isOpen()) {
            // Extract and update macroblocks for current frame
            AVFrame* frame = decoder->getCurrentFrame();
            if (frame) {
                QVector<MacroblockInfo> mbs = macroblockAnalyzer->extractMacroblocks(frame);
                macroblockOverlay->setMacroblocks(mbs);
            }
        }
    }
}

void VideoPlayer::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // Update overlay geometry to match video display
    if (macroblockOverlay && videoDisplay) {
        macroblockOverlay->setGeometry(videoDisplay->geometry());
    }
    if (qualityHeatmapOverlay && videoDisplay) {
        qualityHeatmapOverlay->setGeometry(videoDisplay->geometry());
    }
}

void VideoPlayer::setShowQualityHeatmap(bool show, QualityHeatmapOverlay::HeatmapMode mode) {
    if (!qualityHeatmapOverlay) {
        return;
    }

    if (show) {
        qualityHeatmapOverlay->setHeatmapMode(mode);
        qualityHeatmapOverlay->show();
        updateQualityHeatmap();
    } else {
        qualityHeatmapOverlay->setHeatmapMode(QualityHeatmapOverlay::None);
        qualityHeatmapOverlay->hide();
    }
}

void VideoPlayer::setReferenceVideo(const QString& filePath) {
    if (referenceDecoder->isOpen()) {
        referenceDecoder->close();
    }

    if (!filePath.isEmpty()) {
        referenceDecoder->open(filePath);
    }
}

void VideoPlayer::updateQualityHeatmap() {
    if (!qualityHeatmapOverlay || !decoder->isOpen()) {
        return;
    }

    QualityHeatmapOverlay::HeatmapMode mode = qualityHeatmapOverlay->getHeatmapMode();
    if (mode == QualityHeatmapOverlay::None) {
        return;
    }

    // For PSNR/SSIM modes, need reference video
    if ((mode == QualityHeatmapOverlay::PSNR || mode == QualityHeatmapOverlay::SSIM) &&
        !referenceDecoder->isOpen()) {
        return;
    }

    AVFrame* testFrame = decoder->getCurrentFrame();
    if (!testFrame) {
        return;
    }

    if (mode == QualityHeatmapOverlay::PSNR || mode == QualityHeatmapOverlay::SSIM) {
        // Seek reference decoder to same frame
        referenceDecoder->close();
        referenceDecoder->open(referenceDecoder->getFilePath());
        FrameInfo frameInfo;
        for (int i = 0; i <= currentFrameNumber; i++) {
            if (!referenceDecoder->readNextFrame(frameInfo)) {
                break;
            }
        }

        AVFrame* refFrame = referenceDecoder->getCurrentFrame();
        if (!refFrame) {
            return;
        }

        if (mode == QualityHeatmapOverlay::PSNR) {
            QVector<double> psnrData = qualityHeatmapAnalyzer->calculatePSNRPerMacroblock(refFrame, testFrame);
            qualityHeatmapOverlay->setHeatmapData(psnrData,
                                                   qualityHeatmapAnalyzer->getBlockRows(),
                                                   qualityHeatmapAnalyzer->getBlockCols());
        } else if (mode == QualityHeatmapOverlay::SSIM) {
            QVector<double> ssimData = qualityHeatmapAnalyzer->calculateSSIMPerMacroblock(refFrame, testFrame);
            qualityHeatmapOverlay->setHeatmapData(ssimData,
                                                   qualityHeatmapAnalyzer->getBlockRows(),
                                                   qualityHeatmapAnalyzer->getBlockCols());
        }
    } else if (mode == QualityHeatmapOverlay::Temperature) {
        // For Temperature mode, use previous frame as reference
        if (currentFrameNumber > 0) {
            decoder->close();
            decoder->open(decoder->getFilePath());
            FrameInfo frameInfo;
            for (int i = 0; i < currentFrameNumber; i++) {
                if (!decoder->readNextFrame(frameInfo)) {
                    break;
                }
            }
            AVFrame* prevFrame = decoder->getCurrentFrame();
            decoder->readNextFrame(frameInfo);
            AVFrame* currFrame = decoder->getCurrentFrame();

            if (prevFrame && currFrame) {
                QImage heatmap = qualityHeatmapAnalyzer->generateTemperatureMap(prevFrame, currFrame);
                qualityHeatmapOverlay->setHeatmapImage(heatmap);
            }
        }
    } else if (mode == QualityHeatmapOverlay::Subtraction) {
        // For Subtraction mode, use previous frame as reference
        if (currentFrameNumber > 0) {
            decoder->close();
            decoder->open(decoder->getFilePath());
            FrameInfo frameInfo;
            for (int i = 0; i < currentFrameNumber; i++) {
                if (!decoder->readNextFrame(frameInfo)) {
                    break;
                }
            }
            AVFrame* prevFrame = decoder->getCurrentFrame();
            decoder->readNextFrame(frameInfo);
            AVFrame* currFrame = decoder->getCurrentFrame();

            if (prevFrame && currFrame) {
                QImage heatmap = qualityHeatmapAnalyzer->generateSubtractionMap(prevFrame, currFrame);
                qualityHeatmapOverlay->setHeatmapImage(heatmap);
            }
        }
    }
}
