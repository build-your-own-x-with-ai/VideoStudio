#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <memory>

namespace VideoStudio {

class VideoDecoder;
class VideoOutput;
class BarChart;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void openFile();
    void play();
    void pause();
    void stepForward();
    void stepBackward();
    void onFrameClicked(int frameNumber);
    void onPlaybackTimer();

private:
    void createActions();
    void createMenus();
    void createToolbar();
    void createWidgets();
    void updateUI();

    std::unique_ptr<VideoDecoder> m_decoder;
    VideoOutput* m_videoOutput;
    BarChart* m_barChart;
    QTimer* m_playbackTimer;
    bool m_isPlaying;
};

} // namespace VideoStudio

#endif // MAINWINDOW_H
