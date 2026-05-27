#include "MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QScrollArea>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), decoder(nullptr), currentFilePath("") {
    decoder = new VideoDecoder();
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    updateUI();

    setWindowTitle("VideoStudio - 专业视频编解码分析工具");
    resize(1200, 800);
}

MainWindow::~MainWindow() {
    delete decoder;
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    QWidget* leftPanel = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);

    videoPreview = new QLabel("未打开视频文件", this);
    videoPreview->setAlignment(Qt::AlignCenter);
    videoPreview->setMinimumSize(640, 360);
    videoPreview->setStyleSheet("QLabel { background-color: #2b2b2b; color: #888; border: 1px solid #555; }");
    videoPreview->setScaledContents(false);

    leftLayout->addWidget(videoPreview, 1);

    splitter->addWidget(leftPanel);

    tabWidget = new QTabWidget(this);
    streamInfoPanel = new StreamInfoPanel(this);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(streamInfoPanel);
    scrollArea->setWidgetResizable(true);

    tabWidget->addTab(scrollArea, "概览");
    tabWidget->setMinimumWidth(350);

    splitter->addWidget(tabWidget);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
    setCentralWidget(centralWidget);
}

void MainWindow::setupMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);

    QMenu* fileMenu = menuBar->addMenu("文件");
    openAction = fileMenu->addAction("打开视频...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

    closeAction = fileMenu->addAction("关闭视频");
    closeAction->setEnabled(false);
    connect(closeAction, &QAction::triggered, this, &MainWindow::closeFile);

    fileMenu->addSeparator();

    exitAction = fileMenu->addAction("退出");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    QMenu* helpMenu = menuBar->addMenu("帮助");
    aboutAction = helpMenu->addAction("关于");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);

    setMenuBar(menuBar);
}

void MainWindow::setupToolBar() {
    QToolBar* toolBar = addToolBar("主工具栏");
    toolBar->setMovable(false);

    toolBar->addAction(openAction);
    toolBar->addAction(closeAction);
}

void MainWindow::setupStatusBar() {
    statusBar = new QStatusBar(this);
    statusLabel = new QLabel("就绪", this);
    statusBar->addWidget(statusLabel);
    setStatusBar(statusBar);
}

void MainWindow::openFile() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "打开视频文件",
        "",
        "视频文件 (*.mp4 *.mkv *.avi *.mov *.flv *.wmv *.webm);;所有文件 (*.*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    if (decoder->open(filePath)) {
        currentFilePath = filePath;
        StreamInfo info = decoder->getStreamInfo();
        streamInfoPanel->setStreamInfo(info);

        FrameInfo frameInfo;
        if (decoder->readNextFrame(frameInfo)) {
            QImage image = decoder->getCurrentFrameImage();
            if (!image.isNull()) {
                videoPreview->setPixmap(QPixmap::fromImage(image).scaled(
                    videoPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }

        statusLabel->setText(QString("已打开: %1").arg(filePath));
        setWindowTitle(QString("VideoStudio - %1").arg(QFileInfo(filePath).fileName()));
        updateUI();
    } else {
        QMessageBox::critical(this, "错误", "无法打开视频文件");
    }
}

void MainWindow::closeFile() {
    decoder->close();
    currentFilePath.clear();
    streamInfoPanel->clear();
    videoPreview->clear();
    videoPreview->setText("未打开视频文件");
    statusLabel->setText("就绪");
    setWindowTitle("VideoStudio - 专业视频编解码分析工具");
    updateUI();
}

void MainWindow::about() {
    QMessageBox::about(this, "关于 VideoStudio",
        "<h3>VideoStudio 1.0</h3>"
        "<p>专业视频编解码分析工具</p>"
        "<p>基于 Qt 和 FFmpeg 开发</p>"
        "<p>用于深度分析视频流的编解码参数、比特率、帧类型、GOP 结构等专业指标</p>");
}

void MainWindow::updateUI() {
    bool hasVideo = decoder->isOpen();
    closeAction->setEnabled(hasVideo);
}
