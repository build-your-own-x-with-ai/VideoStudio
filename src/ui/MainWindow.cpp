#include "MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QScrollArea>
#include <QProgressDialog>
#include <QTimer>
#include <QDir>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), decoder(nullptr), metricsCollector(nullptr),
      analyzerThread(nullptr), bitrateAnalyzer(nullptr), gopAnalyzer(nullptr),
      progressDialog(nullptr), currentFilePath("") {
    decoder = new VideoDecoder();
    metricsCollector = new MetricsCollector(this);
    bitrateAnalyzer = new BitrateAnalyzer(this);
    gopAnalyzer = new GOPAnalyzer(this);
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    updateUI();

    setWindowTitle("VideoStudio - 专业视频编解码分析工具");
    resize(1200, 800);
}

MainWindow::~MainWindow() {
    if (analyzerThread) {
        analyzerThread->stop();
        analyzerThread->wait();
        delete analyzerThread;
    }
    delete decoder;
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    QWidget* leftPanel = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);

    videoPlayer = new VideoPlayer(this);
    connect(videoPlayer, &VideoPlayer::frameChanged, this, [this](int frameNumber) {
        statusLabel->setText(QString("帧 #%1 / %2").arg(frameNumber).arg(videoPlayer->getTotalFrames()));
    });
    leftLayout->addWidget(videoPlayer, 1);

    splitter->addWidget(leftPanel);

    tabWidget = new QTabWidget(this);
    streamInfoPanel = new StreamInfoPanel(this);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(streamInfoPanel);
    scrollArea->setWidgetResizable(true);

    tabWidget->addTab(scrollArea, "概览");

    frameListView = new FrameListView(this);
    frameListView->setMetricsCollector(metricsCollector);
    connect(frameListView, &FrameListView::frameSelected, this, &MainWindow::onFrameSelected);

    tabWidget->addTab(frameListView, "帧分析");

    bitrateChart = new BitrateChart(this);
    tabWidget->addTab(bitrateChart, "比特率分析");

    gopViewer = new GOPViewer(this);
    tabWidget->addTab(gopViewer, "GOP 分析");

    frameSizeChart = new FrameSizeChart(this);
    tabWidget->addTab(frameSizeChart, "帧大小分布");

    tabWidget->setMinimumWidth(350);

    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 1 && metricsCollector->getFrameCount() > 0) {
            frameListView->updateFrameList();
        } else if (index == 2 && metricsCollector->getFrameCount() > 0) {
            bitrateAnalyzer->analyze(metricsCollector->getAllFrames(), 1.0);
            bitrateChart->setBitrateData(bitrateAnalyzer->getBitratePoints(),
                                         bitrateAnalyzer->getStats());
        } else if (index == 3 && metricsCollector->getFrameCount() > 0) {
            gopAnalyzer->analyze(metricsCollector->getAllFrames());
            gopViewer->setGOPData(gopAnalyzer->getGOPs(),
                                  gopAnalyzer->getStats());
        } else if (index == 4 && metricsCollector->getFrameCount() > 0) {
            FrameSizeDistribution dist = metricsCollector->calculateFrameSizeDistribution();
            frameSizeChart->setDistribution(dist);
            frameSizeChart->setFrameData(metricsCollector->getAllFrames());
        }
    });

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

    QMenu* exportMenu = menuBar->addMenu("导出");
    exportHTMLAction = exportMenu->addAction("导出 HTML 报告...");
    exportHTMLAction->setEnabled(false);
    connect(exportHTMLAction, &QAction::triggered, this, &MainWindow::exportHTMLReport);

    exportFrameListAction = exportMenu->addAction("导出帧列表 (CSV)...");
    exportFrameListAction->setEnabled(false);
    connect(exportFrameListAction, &QAction::triggered, this, &MainWindow::exportFrameListCSV);

    exportBitrateAction = exportMenu->addAction("导出比特率数据 (CSV)...");
    exportBitrateAction->setEnabled(false);
    connect(exportBitrateAction, &QAction::triggered, this, &MainWindow::exportBitrateCSV);

    exportGOPAction = exportMenu->addAction("导出 GOP 数据 (CSV)...");
    exportGOPAction->setEnabled(false);
    connect(exportGOPAction, &QAction::triggered, this, &MainWindow::exportGOPCSV);

    exportMenu->addSeparator();

    saveScreenshotAction = exportMenu->addAction("保存截图...");
    saveScreenshotAction->setEnabled(false);
    connect(saveScreenshotAction, &QAction::triggered, this, &MainWindow::saveScreenshot);

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
        decoder->close();

        // Open video in player
        videoPlayer->openVideo(filePath);

        metricsCollector->clear();

        progressDialog = new QProgressDialog("正在分析视频帧...", "取消", 0, 100, this);
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setMinimumDuration(500);

        if (analyzerThread) {
            analyzerThread->stop();
            analyzerThread->wait();
            delete analyzerThread;
        }

        analyzerThread = new VideoAnalyzerThread(filePath, this);
        connect(analyzerThread, &VideoAnalyzerThread::progressUpdated,
                this, &MainWindow::onAnalysisProgress, Qt::QueuedConnection);
        connect(analyzerThread, &VideoAnalyzerThread::analysisComplete,
                this, &MainWindow::onAnalysisComplete, Qt::QueuedConnection);
        connect(analyzerThread, &VideoAnalyzerThread::analysisFailed,
                this, &MainWindow::onAnalysisFailed, Qt::QueuedConnection);
        connect(progressDialog, &QProgressDialog::canceled, [this]() {
            if (analyzerThread) {
                analyzerThread->stop();
            }
        });

        analyzerThread->start();

        statusLabel->setText(QString("正在分析: %1").arg(filePath));
        setWindowTitle(QString("VideoStudio - %1").arg(QFileInfo(filePath).fileName()));
        updateUI();
    } else {
        QMessageBox::critical(this, "错误", "无法打开视频文件");
    }
}

void MainWindow::closeFile() {
    decoder->close();
    metricsCollector->clear();
    currentFilePath.clear();
    streamInfoPanel->clear();
    frameListView->clear();
    videoPlayer->closeVideo();
    bitrateChart->clear();
    gopViewer->clear();
    frameSizeChart->clear();
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

    bool hasAnalyzedData = metricsCollector->getFrameCount() > 0;
    exportHTMLAction->setEnabled(hasAnalyzedData);
    exportFrameListAction->setEnabled(hasAnalyzedData);
    exportBitrateAction->setEnabled(hasAnalyzedData);
    exportGOPAction->setEnabled(hasAnalyzedData);
    saveScreenshotAction->setEnabled(videoPlayer->isVideoOpen());
}

void MainWindow::onFrameSelected(int frameIndex) {
    if (frameIndex < 0 || frameIndex >= metricsCollector->getFrameCount()) {
        return;
    }

    const FrameInfo& frame = metricsCollector->getFrame(frameIndex);
    statusLabel->setText(QString("帧 #%1 | 类型: %2 | 大小: %3 字节 | 时间: %4 秒")
        .arg(frame.frameNumber)
        .arg(frame.frameType)
        .arg(frame.size)
        .arg(frame.timestamp, 0, 'f', 3));

    // Seek video player to selected frame
    if (videoPlayer->isVideoOpen()) {
        videoPlayer->seekToFrame(frameIndex);
    }
}

void MainWindow::onAnalysisProgress(int current, int total) {
    if (progressDialog) {
        progressDialog->setMaximum(total);
        progressDialog->setValue(current);
    }
}

void MainWindow::onAnalysisComplete() {
    // 使用 QTimer 延迟处理，确保信号处理完毕
    QTimer::singleShot(0, this, [this]() {
        if (progressDialog) {
            progressDialog->close();
            delete progressDialog;
            progressDialog = nullptr;
        }

        if (analyzerThread) {
            analyzerThread->wait();
            const QVector<FrameInfo>& frames = analyzerThread->getFrames();
            metricsCollector->addFrames(frames);
            delete analyzerThread;
            analyzerThread = nullptr;
        }

        statusLabel->setText(QString("已打开: %1 (共 %2 帧)")
            .arg(currentFilePath)
            .arg(metricsCollector->getFrameCount()));
        updateUI();
    });
}

void MainWindow::onAnalysisFailed(const QString& error) {
    if (progressDialog) {
        progressDialog->close();
        delete progressDialog;
        progressDialog = nullptr;
    }

    QMessageBox::critical(this, "错误", QString("分析失败: %1").arg(error));
    closeFile();
}

void MainWindow::exportHTMLReport() {
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "导出 HTML 报告",
        QDir::homePath() + "/video_report.html",
        "HTML 文件 (*.html)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    bitrateAnalyzer->analyze(metricsCollector->getAllFrames(), 1.0);
    gopAnalyzer->analyze(metricsCollector->getAllFrames());

    bool success = Exporter::exportHTMLReport(
        filePath,
        currentFilePath,
        streamInfoPanel->getStreamInfo(),
        metricsCollector,
        bitrateAnalyzer->getStats(),
        gopAnalyzer->getStats()
    );

    if (success) {
        QMessageBox::information(this, "成功", "HTML 报告已导出");
    } else {
        QMessageBox::critical(this, "错误", "导出 HTML 报告失败");
    }
}

void MainWindow::exportFrameListCSV() {
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "导出帧列表",
        QDir::homePath() + "/frame_list.csv",
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    bool success = Exporter::exportFrameListToCSV(
        filePath,
        metricsCollector->getAllFrames()
    );

    if (success) {
        QMessageBox::information(this, "成功", "帧列表已导出");
    } else {
        QMessageBox::critical(this, "错误", "导出帧列表失败");
    }
}

void MainWindow::exportBitrateCSV() {
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "导出比特率数据",
        QDir::homePath() + "/bitrate_data.csv",
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    bitrateAnalyzer->analyze(metricsCollector->getAllFrames(), 1.0);

    bool success = Exporter::exportBitrateToCSV(
        filePath,
        bitrateAnalyzer->getBitratePoints()
    );

    if (success) {
        QMessageBox::information(this, "成功", "比特率数据已导出");
    } else {
        QMessageBox::critical(this, "错误", "导出比特率数据失败");
    }
}

void MainWindow::exportGOPCSV() {
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "导出 GOP 数据",
        QDir::homePath() + "/gop_data.csv",
        "CSV 文件 (*.csv)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    gopAnalyzer->analyze(metricsCollector->getAllFrames());

    bool success = Exporter::exportGOPToCSV(
        filePath,
        gopAnalyzer->getGOPs()
    );

    if (success) {
        QMessageBox::information(this, "成功", "GOP 数据已导出");
    } else {
        QMessageBox::critical(this, "错误", "导出 GOP 数据失败");
    }
}

void MainWindow::saveScreenshot() {
    QPixmap pixmap = videoPlayer->getCurrentFramePixmap();
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "警告", "没有可保存的图像");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "保存截图",
        QDir::homePath() + "/screenshot.png",
        "PNG 图像 (*.png);;JPEG 图像 (*.jpg)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    bool success = pixmap.save(filePath);

    if (success) {
        QMessageBox::information(this, "成功", "截图已保存");
    } else {
        QMessageBox::critical(this, "错误", "保存截图失败");
    }
}
