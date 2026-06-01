#ifndef YUVVIEWERDIALOG_H
#define YUVVIEWERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <memory>

namespace VideoStudio {

class YUVReader;
class VideoOutput;

class YUVViewerDialog : public QDialog {
    Q_OBJECT

public:
    explicit YUVViewerDialog(QWidget* parent = nullptr);
    ~YUVViewerDialog();

private slots:
    void browseFile();
    void openYUVFile();
    void onFormatChanged(int index);
    void onWidthChanged(int width);
    void onHeightChanged(int height);
    void playPause();
    void stepForward();
    void stepBackward();
    void seekToFrame(int frameNumber);
    void onPlaybackTimer();
    void exportCurrentFrame();

private:
    void setupUI();
    void updateFrameDisplay();
    void updateFrameCountLabel();
    void validateAndUpdateFrameCount();

    // UI Components
    QLineEdit* m_filePathEdit;
    QPushButton* m_browseButton;
    QSpinBox* m_widthSpin;
    QSpinBox* m_heightSpin;
    QComboBox* m_formatCombo;
    QLabel* m_frameCountLabel;

    VideoOutput* m_videoOutput;

    QPushButton* m_playButton;
    QPushButton* m_stepBackButton;
    QPushButton* m_stepForwardButton;
    QSlider* m_frameSlider;
    QSpinBox* m_frameNumberSpin;
    QLabel* m_frameInfoLabel;

    QPushButton* m_exportButton;
    QPushButton* m_closeButton;

    // Data
    std::unique_ptr<YUVReader> m_reader;
    QTimer* m_playbackTimer;
    bool m_isPlaying;
    int m_currentFrame;
};

} // namespace VideoStudio

#endif // YUVVIEWERDIALOG_H
