#include "mainwindow.h"
#include "core/videodecoder.h"
#include "widgets/videooutput.h"
#include "widgets/barchart.h"

#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QStatusBar>
#include <QLabel>

namespace VideoStudio {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_decoder(std::make_unique<VideoDecoder>())
    , m_videoOutput(nullptr)
    , m_barChart(nullptr)
    , m_playbackTimer(new QTimer(this))
    , m_isPlaying(false)
{
    setWindowTitle("VideoStudio");
    resize(1280, 720);

    createWidgets();
    createActions();
    createMenus();
    createToolbar();

    connect(m_playbackTimer, &QTimer::timeout, this, &MainWindow::onPlaybackTimer);

    statusBar()->showMessage("Ready");
}

MainWindow::~MainWindow() {
}

void MainWindow::createWidgets() {
    // Create central widget with layout
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    // Create bar chart
    m_barChart = new BarChart(this);
    connect(m_barChart, &BarChart::frameClicked, this, &MainWindow::onFrameClicked);

    // Create video output
    m_videoOutput = new VideoOutput(this);

    // Add widgets to layout
    layout->addWidget(m_barChart, 0);
    layout->addWidget(m_videoOutput, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::createActions() {
    // Actions will be created here
}

void MainWindow::createMenus() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* openAction = fileMenu->addAction(tr("&Open..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

    fileMenu->addSeparator();

    QAction* quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    // Navigation menu
    QMenu* navMenu = menuBar()->addMenu(tr("&Navigation"));

    QAction* playAction = navMenu->addAction(tr("&Play"));
    playAction->setShortcut(Qt::Key_Space);
    connect(playAction, &QAction::triggered, this, &MainWindow::play);

    QAction* pauseAction = navMenu->addAction(tr("P&ause"));
    connect(pauseAction, &QAction::triggered, this, &MainWindow::pause);

    navMenu->addSeparator();

    QAction* stepForwardAction = navMenu->addAction(tr("Step &Forward"));
    stepForwardAction->setShortcut(Qt::ALT | Qt::Key_Right);
    connect(stepForwardAction, &QAction::triggered, this, &MainWindow::stepForward);

    QAction* stepBackwardAction = navMenu->addAction(tr("Step &Backward"));
    stepBackwardAction->setShortcut(Qt::ALT | Qt::Key_Left);
    connect(stepBackwardAction, &QAction::triggered, this, &MainWindow::stepBackward);

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction* aboutAction = helpMenu->addAction(tr("&About"));
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, tr("About VideoStudio"),
            tr("VideoStudio v1.0\n\n"
               "Professional video stream analysis tool\n"
               "Built with Qt and FFmpeg"));
    });
}

void MainWindow::createToolbar() {
    QToolBar* toolbar = addToolBar(tr("Main Toolbar"));
    toolbar->setMovable(false);

    QAction* openAction = toolbar->addAction(tr("Open"));
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

    toolbar->addSeparator();

    QAction* playAction = toolbar->addAction(tr("Play"));
    connect(playAction, &QAction::triggered, this, &MainWindow::play);

    QAction* pauseAction = toolbar->addAction(tr("Pause"));
    connect(pauseAction, &QAction::triggered, this, &MainWindow::pause);

    toolbar->addSeparator();

    QAction* stepBackAction = toolbar->addAction(tr("Step Back"));
    connect(stepBackAction, &QAction::triggered, this, &MainWindow::stepBackward);

    QAction* stepFwdAction = toolbar->addAction(tr("Step Forward"));
    connect(stepFwdAction, &QAction::triggered, this, &MainWindow::stepForward);
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Video File"), QString(),
        tr("Video Files (*.mp4 *.mkv *.avi *.mov *.h264 *.h265 *.hevc *.264 *.265);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    if (m_decoder->openFile(fileName)) {
        statusBar()->showMessage(tr("Opened: %1").arg(fileName));

        // Update bar chart with frame index
        m_barChart->setFrameIndex(&m_decoder->getFrameIndex());

        // Display first frame
        AVFrame* frame = m_decoder->decodeNextFrame();
        if (frame) {
            m_videoOutput->displayFrame(frame);
            m_barChart->setCurrentFrame(0);
        }

        // Show video info in status bar
        QString info = QString("%1 | %2x%3 | %4 fps | %5 frames")
            .arg(m_decoder->getCodecName())
            .arg(m_decoder->getWidth())
            .arg(m_decoder->getHeight())
            .arg(m_decoder->getFrameRate(), 0, 'f', 2)
            .arg(m_decoder->getFrameCount());
        statusBar()->showMessage(info);
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to open video file"));
    }
}

void MainWindow::play() {
    if (!m_decoder->isOpen()) {
        return;
    }

    m_isPlaying = true;
    double frameRate = m_decoder->getFrameRate();
    int interval = frameRate > 0 ? static_cast<int>(1000.0 / frameRate) : 40;
    m_playbackTimer->start(interval);
    statusBar()->showMessage(tr("Playing..."));
}

void MainWindow::pause() {
    m_isPlaying = false;
    m_playbackTimer->stop();
    statusBar()->showMessage(tr("Paused"));
}

void MainWindow::stepForward() {
    if (!m_decoder->isOpen()) {
        return;
    }

    AVFrame* frame = m_decoder->decodeNextFrame();
    if (frame) {
        m_videoOutput->displayFrame(frame);
        int currentFrame = m_decoder->getCurrentFrameNumber();
        m_barChart->setCurrentFrame(currentFrame);
        statusBar()->showMessage(tr("Frame %1 / %2")
            .arg(currentFrame)
            .arg(m_decoder->getFrameCount()));
    } else {
        // Reached end of video
        pause();
        statusBar()->showMessage(tr("End of video"));
    }
}

void MainWindow::stepBackward() {
    if (!m_decoder->isOpen()) {
        return;
    }

    int currentFrame = m_decoder->getCurrentFrameNumber();
    if (currentFrame > 0) {
        if (m_decoder->seekToFrame(currentFrame - 1)) {
            AVFrame* frame = m_decoder->decodeNextFrame();
            if (frame) {
                m_videoOutput->displayFrame(frame);
                m_barChart->setCurrentFrame(currentFrame - 1);
                statusBar()->showMessage(tr("Frame %1 / %2")
                    .arg(currentFrame - 1)
                    .arg(m_decoder->getFrameCount()));
            }
        }
    }
}

void MainWindow::onFrameClicked(int frameNumber) {
    if (!m_decoder->isOpen()) {
        return;
    }

    pause();

    if (m_decoder->seekToFrame(frameNumber)) {
        AVFrame* frame = m_decoder->decodeNextFrame();
        if (frame) {
            m_videoOutput->displayFrame(frame);
            m_barChart->setCurrentFrame(frameNumber);
            statusBar()->showMessage(tr("Frame %1 / %2")
                .arg(frameNumber)
                .arg(m_decoder->getFrameCount()));
        }
    }
}

void MainWindow::onPlaybackTimer() {
    stepForward();
}

void MainWindow::updateUI() {
    // Update UI state based on decoder state
}

} // namespace VideoStudio
