#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include "core/VideoDecoder.h"

class VideoPlayer : public QWidget {
    Q_OBJECT

public:
    explicit VideoPlayer(QWidget* parent = nullptr);
    ~VideoPlayer();

    bool openVideo(const QString& filePath);
    void closeVideo();
    bool isVideoOpen() const { return videoOpen; }

    void play();
    void pause();
    void stop();
    void seekToFrame(int frameNumber);
    void seekToTime(double seconds);
    void nextFrame();
    void previousFrame();

    int getCurrentFrame() const { return currentFrameNumber; }
    double getCurrentTime() const;
    double getDuration() const;
    int getTotalFrames() const { return totalFrames; }
    bool isPlaying() const { return playing; }

    void setPlaybackSpeed(double speed);
    double getPlaybackSpeed() const { return playbackSpeed; }

    QPixmap getCurrentFramePixmap() const;

signals:
    void frameChanged(int frameNumber);
    void playbackStateChanged(bool playing);
    void positionChanged(double seconds);

private slots:
    void onPlaybackTimer();
    void onProgressSliderMoved(int value);
    void onPlayPauseClicked();
    void onStopClicked();
    void onPreviousFrameClicked();
    void onNextFrameClicked();
    void onSpeedChanged(int index);

private:
    void setupUI();
    void updateFrame();
    void updateControls();
    void updateTimeLabel();
    QString formatTime(double seconds);

    // Video components
    VideoDecoder* decoder;
    QLabel* videoDisplay;
    bool videoOpen;
    bool playing;
    int currentFrameNumber;
    int totalFrames;
    double frameRate;
    double playbackSpeed;

    // UI components
    QPushButton* playPauseButton;
    QPushButton* stopButton;
    QPushButton* previousFrameButton;
    QPushButton* nextFrameButton;
    QSlider* progressSlider;
    QLabel* timeLabel;
    QComboBox* speedComboBox;

    // Playback timer
    QTimer* playbackTimer;
    qint64 lastFrameTime;
};

#endif // VIDEOPLAYER_H
