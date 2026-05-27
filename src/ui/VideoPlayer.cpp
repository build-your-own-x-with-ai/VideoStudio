#include "VideoPlayer.h"
#include <QComboBox>
#include <QDateTime>

VideoPlayer::VideoPlayer(QWidget* parent)
    : QWidget(parent), decoder(nullptr), videoOpen(false), playing(false),
      currentFrameNumber(0), totalFrames(0), frameRate(0.0), playbackSpeed(1.0),
      lastFrameTime(0) {
    decoder = new VideoDecoder();
    playbackTimer = new QTimer(this);
    connect(playbackTimer, &QTimer::timeout, this, &VideoPlayer::onPlaybackTimer);
    setupUI();
}

VideoPlayer::~VideoPlayer() {
    closeVideo();
    delete decoder;
}

void VideoPlayer::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Video display area
    videoDisplay = new QLabel(this);
    videoDisplay->setAlignment(Qt::AlignCenter);
    videoDisplay->setMinimumSize(640, 360);
    videoDisplay->setStyleSheet("QLabel { background-color: #2b2b2b; color: #888; border: 1px solid #555; }");
    videoDisplay->setText("未打开视频文件");
    videoDisplay->setScaledContents(false);
    mainLayout->addWidget(videoDisplay, 1);

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
