#include "mainwindow.h"
#include "core/videodecoder.h"
#include "core/tsparser.h"
#include "core/mp4parser.h"
#include "core/mkvparser.h"
#include "core/aviparser.h"
#include "core/flvparser.h"
#include "core/nalunitparser.h"
#include "widgets/videooutput.h"
#include "widgets/barchart.h"
#include "widgets/areachart.h"
#include "dialogs/qualitymetricsdialog.h"
#include "dialogs/referencecomparisondialog.h"
#include "dialogs/aboutdialog.h"
#include "dialogs/csvexportdialog.h"
#include "dialogs/yuvexportdialog.h"
#include "dialogs/yuvviewerdialog.h"
#include "dialogs/duplicateframedetectiondialog.h"
#include "widgets/thumbnailbar.h"
#include "widgets/gopviewer.h"
#include "widgets/packetview.h"
#include "widgets/nalunitview.h"
#include "widgets/logviewer.h"
#include "panels/streampanel.h"
#include "panels/overlaypanel.h"
#include "panels/explorerpanel.h"
#include "panels/propertypanel.h"
#include "panels/hexviewerpanel.h"
#include "panels/messagespanel.h"
#include "panels/tr101290panel.h"
#include "panels/timedynamicspanel.h"
#include "panels/bitratepanel.h"
#include "panels/bufferpanel.h"
#include "panels/graphicspanel.h"
#include "panels/commentspanel.h"
#include "panels/epgpanel.h"
#include "panels/blockstatspanel.h"
#include "dialogs/savestreaminfodialog.h"

#include <QApplication>
#include <QCoreApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStatusBar>
#include <QLabel>
#include <QDockWidget>
#include <QTabWidget>
#include <QProgressBar>
#include <QProgressDialog>
#include <QPropertyAnimation>
#include <QTimer>
#include <QSettings>
#include <QFileInfo>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDir>

namespace VideoStudio {

// Static instance for message handler
MainWindow* MainWindow::s_instance = nullptr;

// Static constant for max recent files
const int MainWindow::MaxRecentFiles;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_decoder(std::make_unique<VideoDecoder>())
    , m_tsParser(nullptr)
    , m_videoOutput(nullptr)
    , m_barChart(nullptr)
    , m_areaChart(nullptr)
    , m_thumbnailBar(nullptr)
    , m_streamPanel(nullptr)
    , m_overlayPanel(nullptr)
    , m_gopViewer(nullptr)
    , m_explorerPanel(nullptr)
    , m_propertyPanel(nullptr)
    , m_hexViewerPanel(nullptr)
    , m_packetView(nullptr)
    , m_messagesPanel(nullptr)
    , m_tr101290Panel(nullptr)
    , m_timeDynamicsPanel(nullptr)
    , m_bitratePanel(nullptr)
    , m_bufferPanel(nullptr)
    , m_graphicsPanel(nullptr)
    , m_commentsPanel(nullptr)
    , m_epgPanel(nullptr)
    , m_barChartDock(nullptr)
    , m_areaChartDock(nullptr)
    , m_thumbnailDock(nullptr)
    , m_streamPanelDock(nullptr)
    , m_overlayPanelDock(nullptr)
    , m_gopViewerDock(nullptr)
    , m_explorerPanelDock(nullptr)
    , m_propertyPanelDock(nullptr)
    , m_hexViewerPanelDock(nullptr)
    , m_messagesPanelDock(nullptr)
    , m_tr101290PanelDock(nullptr)
    , m_timeDynamicsPanelDock(nullptr)
    , m_bitratePanelDock(nullptr)
    , m_graphicsPanelDock(nullptr)
    , m_commentsPanelDock(nullptr)
    , m_epgPanelDock(nullptr)
    , m_playbackTimer(new QTimer(this))
    , m_isPlaying(false)
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_progressBar(nullptr)
    , m_statusLabel(nullptr)
    , m_overlayMotionVectorsAction(nullptr)
    , m_overlayPartitionsAction(nullptr)
    , m_overlayFrameTypesAction(nullptr)
    , m_overlayQPHeatmapAction(nullptr)
    , m_filePathLabel(nullptr)
{
    setWindowTitle("VideoStudio - Professional Video Analysis Tool - https://github.com/build-your-own-x-with-ai - 作者：AI开发日志");
    resize(1400, 900);

    // Set organization info for window title
    QCoreApplication::setOrganizationName("VideoStudio");
    QCoreApplication::setOrganizationDomain("https://github.com/build-your-own-x-with-ai/VideoStudio");
    QCoreApplication::setApplicationName("VideoStudio");

    // Set static instance for message handler
    s_instance = this;

    // Setup audio playback
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.5);  // 50% volume by default

    createWidgets();
    createDockWidgets();
    createActions();
    createMenus();
    createToolbar();

    // Setup status bar with progress bar
    m_statusLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusLabel, 1);

    m_progressBar = new QProgressBar();
    m_progressBar->setMaximumWidth(200);
    m_progressBar->setTextVisible(true);
    m_progressBar->hide();  // Hidden by default
    statusBar()->addPermanentWidget(m_progressBar);

    connect(m_playbackTimer, &QTimer::timeout, this, &MainWindow::onPlaybackTimer);

    // Install message handler to capture qDebug/qWarning output
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        // Use static instance instead of activeWindow
        if (MainWindow::s_instance) {
            // Forward to log viewer (use QMetaObject::invokeMethod for thread safety)
            QMetaObject::invokeMethod(MainWindow::s_instance, "appendLog", Qt::QueuedConnection,
                Q_ARG(QString, msg), Q_ARG(QtMsgType, type));
        }

        // Also output to console
        QByteArray localMsg = msg.toLocal8Bit();
        switch (type) {
            case QtDebugMsg:
                fprintf(stderr, "Debug: %s\n", localMsg.constData());
                break;
            case QtInfoMsg:
                fprintf(stderr, "Info: %s\n", localMsg.constData());
                break;
            case QtWarningMsg:
                fprintf(stderr, "Warning: %s\n", localMsg.constData());
                break;
            case QtCriticalMsg:
                fprintf(stderr, "Critical: %s\n", localMsg.constData());
                break;
            case QtFatalMsg:
                fprintf(stderr, "Fatal: %s\n", localMsg.constData());
                abort();
        }
    });

    // Enable drag and drop
    setAcceptDrops(true);
}

MainWindow::~MainWindow() {
    // Clear static instance
    s_instance = nullptr;
}

void MainWindow::createWidgets() {
    // Create central tab widget
    m_centralTabs = new QTabWidget(this);
    setCentralWidget(m_centralTabs);

    // Create video output with scroll area
    m_videoOutput = new VideoOutput(this);
    m_videoScrollArea = new QScrollArea(this);
    m_videoScrollArea->setWidget(m_videoOutput);
    m_videoScrollArea->setWidgetResizable(false);
    m_videoScrollArea->setAlignment(Qt::AlignCenter);
    m_centralTabs->addTab(m_videoScrollArea, "Video");

    // Create packet view (will be shown only for TS files)
    m_packetView = new PacketView(this);
    // Don't add it yet - will be added when TS file is opened

    // Create NAL unit view (will be shown only for MP4/MKV files)
    m_nalUnitView = new NALUnitView(this);
    // Don't add it yet - will be added when MP4/MKV file is opened

    // Create other widgets (will be added to dock widgets)
    m_barChart = new BarChart(this);
    connect(m_barChart, &BarChart::frameClicked, this, &MainWindow::onFrameClicked);

    m_areaChart = new AreaChart(this);

    m_thumbnailBar = new ThumbnailBar(this);
    connect(m_thumbnailBar, &ThumbnailBar::frameClicked, this, &MainWindow::onFrameClicked);

    // Wrap thumbnail bar in scroll area for horizontal scrolling
    m_thumbnailScrollArea = new QScrollArea(this);
    m_thumbnailScrollArea->setWidget(m_thumbnailBar);
    m_thumbnailScrollArea->setWidgetResizable(false);  // Allow widget to set its own size
    m_thumbnailScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_thumbnailScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_thumbnailScrollArea->setMinimumHeight(m_thumbnailBar->minimumHeight());
    m_thumbnailScrollArea->setMaximumHeight(m_thumbnailBar->maximumHeight());

    m_streamPanel = new StreamPanel(this);

    m_overlayPanel = new OverlayPanel(this);
    m_overlayPanel->setVideoOutput(m_videoOutput);

    m_logViewer = new LogViewer(this);

    // Connect overlay panel signals to synchronize menu actions
    connect(m_overlayPanel, &OverlayPanel::motionVectorsToggled, this, [this](bool checked) {
        m_overlayMotionVectorsAction->setChecked(checked);
    });
    connect(m_overlayPanel, &OverlayPanel::partitionsToggled, this, [this](bool checked) {
        m_overlayPartitionsAction->setChecked(checked);
    });
    connect(m_overlayPanel, &OverlayPanel::frameTypesToggled, this, [this](bool checked) {
        m_overlayFrameTypesAction->setChecked(checked);
    });
    connect(m_overlayPanel, &OverlayPanel::qpHeatmapToggled, this, [this](bool checked) {
        m_overlayQPHeatmapAction->setChecked(checked);
    });

    m_gopViewer = new GOPViewer(this);
    connect(m_gopViewer, &GOPViewer::frameClicked, this, &MainWindow::onFrameClicked);

    m_explorerPanel = new ExplorerPanel(this);

    m_propertyPanel = new PropertyPanel(this);

    m_hexViewerPanel = new HexViewerPanel(this);

    m_messagesPanel = new MessagesPanel(this);

    m_tr101290Panel = new TR101290Panel(this);

    m_timeDynamicsPanel = new TimeDynamicsPanel(this);

    m_bitratePanel = new BitratePanel(this);

    m_bufferPanel = new BufferPanel(this);

    m_graphicsPanel = new GraphicsPanel(this);

    m_commentsPanel = new CommentsPanel(this);

    m_epgPanel = new EPGPanel(this);

    m_blockStatsPanel = new BlockStatsPanel(this);
}

void MainWindow::createDockWidgets() {
    // Thumbnail bar dock (top - primary navigation, fixed position)
    // Must be added first and span full width
    m_thumbnailDock = new QDockWidget(tr("Thumbnails"), this);
    m_thumbnailDock->setWidget(m_thumbnailScrollArea);  // Use scroll area instead of thumbnail bar
    m_thumbnailDock->setAllowedAreas(Qt::TopDockWidgetArea);  // Only allow top area
    m_thumbnailDock->setFeatures(QDockWidget::NoDockWidgetFeatures);  // Disable floating, closing, moving
    addDockWidget(Qt::TopDockWidgetArea, m_thumbnailDock);

    // Bar chart dock (top, in second row below thumbnails)
    m_barChartDock = new QDockWidget(tr("Frame Size Distribution"), this);
    m_barChartDock->setWidget(m_barChart);
    m_barChartDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::TopDockWidgetArea, m_barChartDock);

    // Split docks so thumbnails are in separate row
    splitDockWidget(m_thumbnailDock, m_barChartDock, Qt::Vertical);

    // Area chart dock (top, tabbed with bar chart)
    m_areaChartDock = new QDockWidget(tr("Bitstream Distribution"), this);
    m_areaChartDock->setWidget(m_areaChart);
    m_areaChartDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::TopDockWidgetArea, m_areaChartDock);
    tabifyDockWidget(m_barChartDock, m_areaChartDock);

    // Stream panel dock (right)
    m_streamPanelDock = new QDockWidget(tr("Stream Info"), this);
    m_streamPanelDock->setWidget(m_streamPanel);
    m_streamPanelDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_streamPanelDock);

    // Overlay panel dock (right, below stream panel)
    m_overlayPanelDock = new QDockWidget(tr("Overlays"), this);
    m_overlayPanelDock->setWidget(m_overlayPanel);
    m_overlayPanelDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_overlayPanelDock);

    // GOP viewer dock (top, tabbed with bar chart, NOT with thumbnails)
    m_gopViewerDock = new QDockWidget(tr("GOP Structure"), this);
    QScrollArea* gopScrollArea = new QScrollArea(this);
    gopScrollArea->setWidget(m_gopViewer);
    gopScrollArea->setWidgetResizable(false);
    gopScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    gopScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gopViewerDock->setWidget(gopScrollArea);
    m_gopViewerDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::TopDockWidgetArea, m_gopViewerDock);
    tabifyDockWidget(m_barChartDock, m_gopViewerDock);  // Tab with bar chart, not thumbnails

    // Explorer panel dock (left)
    m_explorerPanelDock = new QDockWidget(tr("Explorer"), this);
    m_explorerPanelDock->setWidget(m_explorerPanel);
    m_explorerPanelDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_explorerPanelDock);
    m_explorerPanelDock->hide(); // Hidden by default

    // Property panel dock (right, below overlay panel)
    m_propertyPanelDock = new QDockWidget(tr("Properties"), this);
    m_propertyPanelDock->setWidget(m_propertyPanel);
    m_propertyPanelDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_propertyPanelDock);
    m_propertyPanelDock->hide(); // Hidden by default

    // Hex Viewer panel dock (bottom)
    m_hexViewerPanelDock = new QDockWidget(tr("Hex Viewer"), this);
    m_hexViewerPanelDock->setWidget(m_hexViewerPanel);
    m_hexViewerPanelDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_hexViewerPanelDock);
    m_hexViewerPanelDock->hide(); // Hidden by default

    // Messages panel dock (bottom, tabbed with hex viewer)
    m_messagesPanelDock = new QDockWidget(tr("Messages"), this);
    m_messagesPanelDock->setWidget(m_messagesPanel);
    m_messagesPanelDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_messagesPanelDock);
    tabifyDockWidget(m_hexViewerPanelDock, m_messagesPanelDock);
    m_messagesPanelDock->hide(); // Hidden by default

    // TR 101-290 panel dock (bottom, tabbed with hex viewer)
    m_tr101290PanelDock = new QDockWidget(tr("TR 101-290"), this);
    m_tr101290PanelDock->setWidget(m_tr101290Panel);
    m_tr101290PanelDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_tr101290PanelDock);
    tabifyDockWidget(m_hexViewerPanelDock, m_tr101290PanelDock);
    m_tr101290PanelDock->hide(); // Hidden by default

    // Time Dynamics panel dock (bottom, tabbed with hex viewer)
    m_timeDynamicsPanelDock = new QDockWidget(tr("Time Dynamics"), this);
    m_timeDynamicsPanelDock->setWidget(m_timeDynamicsPanel);
    m_timeDynamicsPanelDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_timeDynamicsPanelDock);
    tabifyDockWidget(m_hexViewerPanelDock, m_timeDynamicsPanelDock);
    m_timeDynamicsPanelDock->hide(); // Hidden by default

    // Bitrate panel dock (bottom, tabbed with hex viewer)
    m_bitratePanelDock = new QDockWidget(tr("Bitrate"), this);
    m_bitratePanelDock->setWidget(m_bitratePanel);
    m_bitratePanelDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_bitratePanelDock);
    tabifyDockWidget(m_hexViewerPanelDock, m_bitratePanelDock);
    m_bitratePanelDock->hide(); // Hidden by default

    // Buffer panel dock (bottom, tabbed with hex viewer)
    m_bufferPanelDock = new QDockWidget(tr("Buffer"), this);
    m_bufferPanelDock->setWidget(m_bufferPanel);
    m_bufferPanelDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_bufferPanelDock);
    tabifyDockWidget(m_hexViewerPanelDock, m_bufferPanelDock);
    m_bufferPanelDock->hide(); // Hidden by default

    // Graphics panel dock (bottom, tabbed with hex viewer)
    m_graphicsPanelDock = new QDockWidget(tr("Graphics"), this);
    m_graphicsPanelDock->setWidget(m_graphicsPanel);
    m_graphicsPanelDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_graphicsPanelDock);
    tabifyDockWidget(m_hexViewerPanelDock, m_graphicsPanelDock);
    m_graphicsPanelDock->hide(); // Hidden by default

    // Comments panel dock (bottom, tabbed with hex viewer)
    m_commentsPanelDock = new QDockWidget(tr("Comments"), this);
    m_commentsPanelDock->setWidget(m_commentsPanel);
    m_commentsPanelDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_commentsPanelDock);
    tabifyDockWidget(m_hexViewerPanelDock, m_commentsPanelDock);
    m_commentsPanelDock->hide(); // Hidden by default

    // EPG panel dock (bottom, tabbed with hex viewer)
    m_epgPanelDock = new QDockWidget(tr("EPG"), this);
    m_epgPanelDock->setWidget(m_epgPanel);
    m_epgPanelDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_epgPanelDock);
    tabifyDockWidget(m_hexViewerPanelDock, m_epgPanelDock);
    m_epgPanelDock->hide(); // Hidden by default

    // Block Stats panel dock (bottom, tabbed with hex viewer)
    m_blockStatsPanelDock = new QDockWidget(tr("Block Statistics"), this);
    m_blockStatsPanelDock->setWidget(m_blockStatsPanel);
    m_blockStatsPanelDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_blockStatsPanelDock);
    tabifyDockWidget(m_hexViewerPanelDock, m_blockStatsPanelDock);
    m_blockStatsPanelDock->hide(); // Hidden by default

    // Log Viewer dock (bottom, tabbed with hex viewer)
    m_logViewerDock = new QDockWidget(tr("Log Viewer"), this);
    m_logViewerDock->setWidget(m_logViewer);
    m_logViewerDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_logViewerDock);
    tabifyDockWidget(m_hexViewerPanelDock, m_logViewerDock);
    m_logViewerDock->hide(); // Hidden by default

    // Make thumbnails the active top tab
    m_thumbnailDock->raise();
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

    // Recent Files submenu
    m_recentFilesMenu = fileMenu->addMenu(tr("Open &Recent"));
    for (int i = 0; i < MaxRecentFiles; ++i) {
        QAction* action = new QAction(this);
        action->setVisible(false);
        connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
        m_recentFileActions.append(action);
        m_recentFilesMenu->addAction(action);
    }
    m_recentFilesMenu->addSeparator();
    QAction* clearRecentAction = m_recentFilesMenu->addAction(tr("Clear Recent Files"));
    connect(clearRecentAction, &QAction::triggered, this, &MainWindow::clearRecentFiles);
    updateRecentFilesMenu();

    fileMenu->addSeparator();

    QAction* saveStreamInfoAction = fileMenu->addAction(tr("&Save Stream Info..."));
    saveStreamInfoAction->setShortcut(tr("Ctrl+S"));
    connect(saveStreamInfoAction, &QAction::triggered, this, &MainWindow::saveStreamInfo);

    fileMenu->addSeparator();

    QAction* exportFrameAction = fileMenu->addAction(tr("Export Frame as &YUV..."));
    exportFrameAction->setShortcut(tr("Ctrl+E"));
    connect(exportFrameAction, &QAction::triggered, this, &MainWindow::exportFrameAsYUV);

    QAction* exportRangeAction = fileMenu->addAction(tr("Export Frame &Range as YUV..."));
    exportRangeAction->setShortcut(tr("Ctrl+Shift+E"));
    connect(exportRangeAction, &QAction::triggered, this, &MainWindow::exportFrameRangeAsYUV);

    QAction* exportCSVAction = fileMenu->addAction(tr("Export &CSV Metrics..."));
    exportCSVAction->setShortcut(tr("Ctrl+M"));
    connect(exportCSVAction, &QAction::triggered, this, &MainWindow::exportCSVMetrics);

    fileMenu->addSeparator();

    QAction* screenshotAction = fileMenu->addAction(tr("Capture &Screenshot..."));
    screenshotAction->setShortcut(tr("Ctrl+P"));
    connect(screenshotAction, &QAction::triggered, this, &MainWindow::captureScreenshot);

    fileMenu->addSeparator();

    QAction* quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    // View menu
    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));

    m_toggleStreamAction = viewMenu->addAction(tr("Stream &Info"));
    m_toggleStreamAction->setCheckable(true);
    m_toggleStreamAction->setChecked(true);
    connect(m_toggleStreamAction, &QAction::triggered, this, &MainWindow::toggleStreamPanel);

    m_toggleExplorerAction = viewMenu->addAction(tr("&Explorer"));
    m_toggleExplorerAction->setCheckable(true);
    m_toggleExplorerAction->setChecked(false);
    connect(m_toggleExplorerAction, &QAction::triggered, this, &MainWindow::toggleExplorerPanel);

    m_toggleBarChartAction = viewMenu->addAction(tr("&Bar Chart"));
    m_toggleBarChartAction->setCheckable(true);
    m_toggleBarChartAction->setChecked(true);
    connect(m_toggleBarChartAction, &QAction::triggered, this, &MainWindow::toggleBarChart);

    m_toggleAreaChartAction = viewMenu->addAction(tr("&Area Chart"));
    m_toggleAreaChartAction->setCheckable(true);
    m_toggleAreaChartAction->setChecked(true);
    connect(m_toggleAreaChartAction, &QAction::triggered, this, &MainWindow::toggleAreaChart);

    m_toggleThumbnailAction = viewMenu->addAction(tr("&Thumbnails"));
    m_toggleThumbnailAction->setCheckable(true);
    m_toggleThumbnailAction->setChecked(true);
    connect(m_toggleThumbnailAction, &QAction::triggered, this, &MainWindow::toggleThumbnailBar);

    m_toggleOverlayPanelAction = viewMenu->addAction(tr("&Overlay Panel"));
    m_toggleOverlayPanelAction->setCheckable(true);
    m_toggleOverlayPanelAction->setChecked(true);
    connect(m_toggleOverlayPanelAction, &QAction::triggered, this, &MainWindow::toggleOverlayPanel);

    m_toggleGOPViewerAction = viewMenu->addAction(tr("&GOP Structure"));
    m_toggleGOPViewerAction->setCheckable(true);
    m_toggleGOPViewerAction->setChecked(true);
    connect(m_toggleGOPViewerAction, &QAction::triggered, this, &MainWindow::toggleGOPViewer);

    m_togglePropertyPanelAction = viewMenu->addAction(tr("&Properties"));
    m_togglePropertyPanelAction->setCheckable(true);
    m_togglePropertyPanelAction->setChecked(false);
    connect(m_togglePropertyPanelAction, &QAction::triggered, this, &MainWindow::togglePropertyPanel);

    m_toggleHexViewerAction = viewMenu->addAction(tr("&Hex Viewer"));
    m_toggleHexViewerAction->setCheckable(true);
    m_toggleHexViewerAction->setChecked(false);
    connect(m_toggleHexViewerAction, &QAction::triggered, this, &MainWindow::toggleHexViewer);

    m_toggleMessagesAction = viewMenu->addAction(tr("&Messages"));
    m_toggleMessagesAction->setCheckable(true);
    m_toggleMessagesAction->setChecked(false);
    connect(m_toggleMessagesAction, &QAction::triggered, this, &MainWindow::toggleMessages);

    m_toggleTR101290Action = viewMenu->addAction(tr("&TR 101-290"));
    m_toggleTR101290Action->setCheckable(true);
    m_toggleTR101290Action->setChecked(false);
    connect(m_toggleTR101290Action, &QAction::triggered, this, &MainWindow::toggleTR101290);

    m_toggleTimeDynamicsAction = viewMenu->addAction(tr("Time &Dynamics"));
    m_toggleTimeDynamicsAction->setCheckable(true);
    m_toggleTimeDynamicsAction->setChecked(false);
    connect(m_toggleTimeDynamicsAction, &QAction::triggered, this, &MainWindow::toggleTimeDynamics);

    m_toggleBitrateAction = viewMenu->addAction(tr("&Bitrate"));
    m_toggleBitrateAction->setCheckable(true);
    m_toggleBitrateAction->setChecked(false);
    connect(m_toggleBitrateAction, &QAction::triggered, this, &MainWindow::toggleBitrate);

    m_toggleBufferAction = viewMenu->addAction(tr("B&uffer"));
    m_toggleBufferAction->setCheckable(true);
    m_toggleBufferAction->setChecked(false);
    connect(m_toggleBufferAction, &QAction::triggered, this, &MainWindow::toggleBuffer);

    m_toggleGraphicsAction = viewMenu->addAction(tr("&Graphics"));
    m_toggleGraphicsAction->setCheckable(true);
    m_toggleGraphicsAction->setChecked(false);
    connect(m_toggleGraphicsAction, &QAction::triggered, this, &MainWindow::toggleGraphics);

    m_toggleCommentsAction = viewMenu->addAction(tr("&Comments"));
    m_toggleCommentsAction->setCheckable(true);
    m_toggleCommentsAction->setChecked(false);
    connect(m_toggleCommentsAction, &QAction::triggered, this, &MainWindow::toggleComments);

    m_toggleEPGAction = viewMenu->addAction(tr("&EPG"));
    m_toggleEPGAction->setCheckable(true);
    m_toggleEPGAction->setChecked(false);
    connect(m_toggleEPGAction, &QAction::triggered, this, &MainWindow::toggleEPG);

    m_toggleBlockStatsAction = viewMenu->addAction(tr("Block &Statistics"));
    m_toggleBlockStatsAction->setCheckable(true);
    m_toggleBlockStatsAction->setChecked(false);
    connect(m_toggleBlockStatsAction, &QAction::triggered, this, &MainWindow::toggleBlockStats);

    m_toggleLogViewerAction = viewMenu->addAction(tr("&Log Viewer"));
    m_toggleLogViewerAction->setCheckable(true);
    m_toggleLogViewerAction->setChecked(false);
    connect(m_toggleLogViewerAction, &QAction::triggered, this, &MainWindow::toggleLogViewer);

    viewMenu->addSeparator();

    // Layout presets
    viewMenu->addAction(tr("Layout 1 (Default)"), this, [this]() {
        // Show panels on both sides to utilize space around video
        m_barChartDock->show();
        m_areaChartDock->show();
        m_thumbnailDock->show();
        m_streamPanelDock->show();
        m_overlayPanelDock->show();
        m_gopViewerDock->show();
        m_explorerPanelDock->show();  // Show Explorer on left side
        m_propertyPanelDock->hide();
        m_hexViewerPanelDock->hide();
        m_messagesPanelDock->hide();
        m_tr101290PanelDock->hide();
        m_timeDynamicsPanelDock->hide();
        m_bitratePanelDock->hide();
        m_bufferPanelDock->hide();
        m_graphicsPanelDock->hide();
        m_commentsPanelDock->hide();
        m_epgPanelDock->hide();
        m_blockStatsPanelDock->hide();
        m_logViewerDock->hide();
    })->setShortcut(Qt::Key_F5);

    viewMenu->addAction(tr("Layout 2 (Minimal)"), this, [this]() {
        m_barChartDock->hide();
        m_areaChartDock->hide();
        m_thumbnailDock->hide();
        m_streamPanelDock->hide();
        m_overlayPanelDock->hide();
        m_gopViewerDock->hide();
        m_explorerPanelDock->hide();
        m_propertyPanelDock->hide();
        m_hexViewerPanelDock->hide();
        m_messagesPanelDock->hide();
        m_tr101290PanelDock->hide();
        m_timeDynamicsPanelDock->hide();
        m_bitratePanelDock->hide();
        m_bufferPanelDock->hide();
        m_graphicsPanelDock->hide();
        m_commentsPanelDock->hide();
        m_epgPanelDock->hide();
        m_blockStatsPanelDock->hide();
        m_logViewerDock->hide();
    })->setShortcut(Qt::Key_F6);

    viewMenu->addAction(tr("Layout 3 (TS Analysis)"), this, [this]() {
        m_barChartDock->hide();
        m_areaChartDock->hide();
        m_thumbnailDock->hide();
        m_streamPanelDock->show();
        m_overlayPanelDock->hide();
        m_gopViewerDock->hide();
        m_explorerPanelDock->show();
        m_propertyPanelDock->show();
        m_hexViewerPanelDock->show();
        m_messagesPanelDock->show();
        m_tr101290PanelDock->show();
        m_timeDynamicsPanelDock->show();
        m_bitratePanelDock->show();
        m_bufferPanelDock->hide();
        m_graphicsPanelDock->hide();
        m_commentsPanelDock->hide();
        m_epgPanelDock->hide();
        m_blockStatsPanelDock->hide();
        m_logViewerDock->hide();
    })->setShortcut(Qt::Key_F7);

    viewMenu->addAction(tr("Layout 4 (Block Analysis)"), this, [this]() {
        m_barChartDock->show();
        m_areaChartDock->hide();
        m_thumbnailDock->show();
        m_streamPanelDock->show();
        m_overlayPanelDock->show();
        m_gopViewerDock->show();
        m_explorerPanelDock->hide();
        m_propertyPanelDock->hide();
        m_hexViewerPanelDock->hide();
        m_messagesPanelDock->hide();
        m_tr101290PanelDock->hide();
        m_timeDynamicsPanelDock->hide();
        m_bitratePanelDock->hide();
        m_bufferPanelDock->hide();
        m_graphicsPanelDock->hide();
        m_commentsPanelDock->hide();
        m_epgPanelDock->hide();
        m_blockStatsPanelDock->show();
        m_logViewerDock->hide();
    })->setShortcut(Qt::Key_F8);

    viewMenu->addAction(tr("Layout 5 (All Panels)"), this, [this]() {
        m_barChartDock->show();
        m_areaChartDock->show();
        m_thumbnailDock->show();
        m_streamPanelDock->show();
        m_overlayPanelDock->show();
        m_gopViewerDock->show();
        m_explorerPanelDock->show();
        m_propertyPanelDock->show();
        m_hexViewerPanelDock->show();
        m_messagesPanelDock->show();
        m_tr101290PanelDock->show();
        m_timeDynamicsPanelDock->show();
        m_bitratePanelDock->show();
        m_bufferPanelDock->show();
        m_graphicsPanelDock->show();
        m_commentsPanelDock->show();
        m_epgPanelDock->show();
        m_blockStatsPanelDock->show();
        m_logViewerDock->show();
    })->setShortcut(Qt::Key_F9);

    viewMenu->addSeparator();

    QAction* toggleGOPModeAction = viewMenu->addAction(tr("GOP: Toggle Thumbnail/Text"));
    toggleGOPModeAction->setShortcut(Qt::ALT | Qt::Key_T);
    connect(toggleGOPModeAction, &QAction::triggered, this, &MainWindow::toggleGOPDisplayMode);

    QAction* toggleGOPDependencyAction = viewMenu->addAction(tr("GOP: Toggle Dependency Arrows"));
    toggleGOPDependencyAction->setShortcut(Qt::ALT | Qt::Key_D);
    connect(toggleGOPDependencyAction, &QAction::triggered, this, &MainWindow::toggleGOPDependencyArrows);

    viewMenu->addAction(tr("Layout 6 (All Visible)"), this, [this]() {
        // Show panels on left and right sides to utilize black space around video

        // Left side - show Explorer
        m_explorerPanelDock->show();

        // Right side - show Stream Info, Overlays, Properties, Block Stats
        m_streamPanelDock->show();
        m_overlayPanelDock->show();
        m_propertyPanelDock->show();
        m_blockStatsPanelDock->show();

        // Top - show charts
        m_barChartDock->show();
        m_areaChartDock->show();

        // Bottom - show key panels
        m_thumbnailDock->show();
        m_gopViewerDock->show();
        m_messagesPanelDock->show();
        m_tr101290PanelDock->show();
        m_timeDynamicsPanelDock->show();
        m_bitratePanelDock->show();
        m_bufferPanelDock->show();
        m_graphicsPanelDock->show();
        m_commentsPanelDock->show();
        m_epgPanelDock->show();
        m_hexViewerPanelDock->show();
        m_logViewerDock->show();

        // Raise important panels to front
        m_barChartDock->raise();
        m_thumbnailDock->raise();
        m_messagesPanelDock->raise();
    })->setShortcut(Qt::Key_F10);

    viewMenu->addSeparator();

    // Overlay submenu
    QMenu* overlayMenu = viewMenu->addMenu(tr("&Overlays"));

    m_overlayMotionVectorsAction = overlayMenu->addAction(tr("Motion &Vectors"));
    m_overlayMotionVectorsAction->setCheckable(true);
    m_overlayMotionVectorsAction->setShortcut(Qt::ALT | Qt::Key_3);
    connect(m_overlayMotionVectorsAction, &QAction::triggered, this, &MainWindow::toggleMotionVectors);

    m_overlayPartitionsAction = overlayMenu->addAction(tr("&Partitions"));
    m_overlayPartitionsAction->setCheckable(true);
    m_overlayPartitionsAction->setShortcut(Qt::ALT | Qt::Key_2);
    connect(m_overlayPartitionsAction, &QAction::triggered, this, &MainWindow::togglePartitions);

    m_overlayFrameTypesAction = overlayMenu->addAction(tr("&Frame Types"));
    m_overlayFrameTypesAction->setCheckable(true);
    m_overlayFrameTypesAction->setShortcut(Qt::ALT | Qt::Key_4);
    connect(m_overlayFrameTypesAction, &QAction::triggered, this, &MainWindow::toggleFrameTypes);

    m_overlayQPHeatmapAction = overlayMenu->addAction(tr("&QP Heatmap"));
    m_overlayQPHeatmapAction->setCheckable(true);
    m_overlayQPHeatmapAction->setShortcut(Qt::ALT | Qt::Key_5);
    connect(m_overlayQPHeatmapAction, &QAction::triggered, this, &MainWindow::toggleQPHeatmap);

    overlayMenu->addSeparator();

    m_cursorModeAction = overlayMenu->addAction(tr("&Cursor Mode (Block Inspector)"));
    m_cursorModeAction->setCheckable(true);
    m_cursorModeAction->setShortcut(Qt::ALT | Qt::Key_C);
    connect(m_cursorModeAction, &QAction::triggered, this, &MainWindow::toggleCursorMode);

    overlayMenu->addSeparator();

    m_standardGridModeAction = overlayMenu->addAction(tr("&Standard Grid Mode"));
    m_standardGridModeAction->setCheckable(true);
    m_standardGridModeAction->setChecked(true);  // Default to enabled
    m_standardGridModeAction->setShortcut(Qt::ALT | Qt::Key_G);
    connect(m_standardGridModeAction, &QAction::triggered, this, &MainWindow::toggleStandardGridMode);

    // Navigation menu
    QMenu* navMenu = menuBar()->addMenu(tr("&Navigation"));

    QAction* playAction = navMenu->addAction(tr("&Play"));
    playAction->setShortcut(Qt::Key_Space);
    connect(playAction, &QAction::triggered, this, &MainWindow::play);

    QAction* pauseAction = navMenu->addAction(tr("P&ause"));
    connect(pauseAction, &QAction::triggered, this, &MainWindow::pause);

    navMenu->addSeparator();

    QAction* stepForwardAction = navMenu->addAction(tr("Step &Forward"));
    stepForwardAction->setShortcuts({QKeySequence(Qt::ALT | Qt::Key_Right), QKeySequence(Qt::Key_Right)});
    connect(stepForwardAction, &QAction::triggered, this, &MainWindow::stepForward);

    QAction* stepBackwardAction = navMenu->addAction(tr("Step &Backward"));
    stepBackwardAction->setShortcuts({QKeySequence(Qt::ALT | Qt::Key_Left), QKeySequence(Qt::Key_Left)});
    connect(stepBackwardAction, &QAction::triggered, this, &MainWindow::stepBackward);

    // Tools menu
    QMenu* toolsMenu = menuBar()->addMenu(tr("&Tools"));

    QAction* qualityMetricsAction = toolsMenu->addAction(tr("&Quality Metrics (PSNR/SSIM)..."));
    qualityMetricsAction->setShortcut(tr("Ctrl+Shift+Q"));
    connect(qualityMetricsAction, &QAction::triggered, this, &MainWindow::showQualityMetrics);

    QAction* referenceComparisonAction = toolsMenu->addAction(tr("&Reference Comparison..."));
    referenceComparisonAction->setShortcut(tr("Ctrl+R"));
    connect(referenceComparisonAction, &QAction::triggered, this, &MainWindow::showReferenceComparison);

    toolsMenu->addSeparator();

    QAction* yuvViewerAction = toolsMenu->addAction(tr("&YUV Viewer..."));
    yuvViewerAction->setShortcut(tr("Ctrl+Y"));
    yuvViewerAction->setStatusTip(tr("Open and analyze raw YUV files"));
    connect(yuvViewerAction, &QAction::triggered, this, &MainWindow::showYUVViewer);

    toolsMenu->addSeparator();

    QAction* duplicateDetectionAction = toolsMenu->addAction(tr("&Duplicate Frame Detection..."));
    duplicateDetectionAction->setShortcut(tr("Ctrl+D"));
    duplicateDetectionAction->setStatusTip(tr("Detect duplicate and freeze frames"));
    connect(duplicateDetectionAction, &QAction::triggered, this, &MainWindow::showDuplicateFrameDetection);

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction* aboutAction = helpMenu->addAction(tr("&About"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
}

void MainWindow::createToolbar() {
    QToolBar* toolbar = addToolBar(tr("Main Toolbar"));
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(24, 24));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // File operations
    QAction* openAction = toolbar->addAction(tr("Open"));
    openAction->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    openAction->setToolTip(tr("Open video file (Ctrl+O)"));
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

    toolbar->addSeparator();

    // Playback controls
    QAction* playAction = toolbar->addAction(tr("Play"));
    playAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    playAction->setToolTip(tr("Play video (Space)"));
    connect(playAction, &QAction::triggered, this, &MainWindow::play);

    QAction* pauseAction = toolbar->addAction(tr("Pause"));
    pauseAction->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    pauseAction->setToolTip(tr("Pause playback"));
    connect(pauseAction, &QAction::triggered, this, &MainWindow::pause);

    toolbar->addSeparator();

    // Frame navigation
    QAction* stepBackAction = toolbar->addAction(tr("Step Back"));
    stepBackAction->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    stepBackAction->setToolTip(tr("Previous frame (Alt+Left)"));
    connect(stepBackAction, &QAction::triggered, this, &MainWindow::stepBackward);

    QAction* stepFwdAction = toolbar->addAction(tr("Step Forward"));
    stepFwdAction->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    stepFwdAction->setToolTip(tr("Next frame (Alt+Right)"));
    connect(stepFwdAction, &QAction::triggered, this, &MainWindow::stepForward);

    toolbar->addSeparator();

    // Frame info display
    QLabel* frameLabel = new QLabel(tr("Frame: 0 / 0"), toolbar);
    frameLabel->setObjectName("frameLabel");
    frameLabel->setMinimumWidth(120);
    frameLabel->setAlignment(Qt::AlignCenter);
    toolbar->addWidget(frameLabel);

    toolbar->addSeparator();

    // File path display
    m_filePathLabel = new QLabel(tr("No file loaded"), toolbar);
    m_filePathLabel->setObjectName("filePathLabel");
    m_filePathLabel->setMinimumWidth(300);
    m_filePathLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    toolbar->addWidget(m_filePathLabel);

    toolbar->addSeparator();

    // Zoom controls
    QAction* zoomInAction = toolbar->addAction(tr("Zoom In"));
    zoomInAction->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    zoomInAction->setToolTip(tr("Zoom in"));
    connect(zoomInAction, &QAction::triggered, m_videoOutput, &VideoOutput::zoomIn);

    QAction* zoomOutAction = toolbar->addAction(tr("Zoom Out"));
    zoomOutAction->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    zoomOutAction->setToolTip(tr("Zoom out"));
    connect(zoomOutAction, &QAction::triggered, m_videoOutput, &VideoOutput::zoomOut);

    QAction* zoomFitAction = toolbar->addAction(tr("Fit"));
    zoomFitAction->setToolTip(tr("Fit to window"));
    connect(zoomFitAction, &QAction::triggered, m_videoOutput, &VideoOutput::zoomFit);
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Video File"), QString(),
        tr("Video Files (*.mp4 *.mkv *.avi *.flv *.mov *.ts *.m2ts *.mts *.h264 *.h265 *.hevc *.264 *.265);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    loadFile(fileName);
}

void MainWindow::loadFile(const QString& fileName) {
    // Add to recent files
    addToRecentFiles(fileName);

    // Store current file path
    m_currentFilePath = fileName;

    // Update window title with filename (similar to Qt Creator style)
    QFileInfo fileInfo(fileName);
    QString windowTitle = QString("%1 - VideoStudio - Professional Video Analysis Tool - https://github.com/build-your-own-x-with-ai - 作者：AI开发日志")
        .arg(fileInfo.fileName());
    setWindowTitle(windowTitle);

    // Update file path label in toolbar
    QString displayPath = fileInfo.fileName(); // Show just filename by default
    m_filePathLabel->setText(displayPath);
    m_filePathLabel->setToolTip(fileName); // Full path in tooltip

    // Check file type
    bool isTSFile = fileName.endsWith(".ts", Qt::CaseInsensitive) ||
                    fileName.endsWith(".m2ts", Qt::CaseInsensitive) ||
                    fileName.endsWith(".mts", Qt::CaseInsensitive);

    bool isMP4File = fileName.endsWith(".mp4", Qt::CaseInsensitive) ||
                     fileName.endsWith(".mov", Qt::CaseInsensitive) ||
                     fileName.endsWith(".m4v", Qt::CaseInsensitive);

    bool isMKVFile = fileName.endsWith(".mkv", Qt::CaseInsensitive) ||
                     fileName.endsWith(".webm", Qt::CaseInsensitive);

    bool isAVIFile = fileName.endsWith(".avi", Qt::CaseInsensitive);

    bool isFLVFile = fileName.endsWith(".flv", Qt::CaseInsensitive);

    // Show Log Viewer in center during file opening
    m_logViewer->clear();  // Clear previous logs

    // Remove log viewer from dock and show as floating widget in center
    if (m_logViewerDock->widget() == m_logViewer) {
        m_logViewerDock->setWidget(nullptr);
    }
    m_logViewer->setParent(this);
    m_logViewer->setWindowFlags(Qt::Widget);

    // Position in center of main window
    int width = this->width() * 0.6;
    int height = this->height() * 0.5;
    int x = (this->width() - width) / 2;
    int y = (this->height() - height) / 2;
    m_logViewer->setGeometry(x, y, width, height);
    m_logViewer->show();
    m_logViewer->raise();

    // Create progress dialog with actual progress bar
    QProgressDialog* progressDialog = new QProgressDialog(tr("Opening video file..."), tr("Cancel"), 0, 100, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(0);  // Show immediately
    progressDialog->setValue(0);
    progressDialog->setAutoClose(false);
    progressDialog->setAutoReset(false);

    // Connect decoder progress signals to both log viewer and progress dialog
    connect(m_decoder.get(), &VideoDecoder::indexingProgress, this, [this, progressDialog](int current, int total) {
        if (current > 0) {
            QString message;
            int percentage = 0;
            if (total > 0) {
                percentage = (current * 100) / total;
                message = QString("Building frame index... %1 / %2 frames (%3%)")
                    .arg(current).arg(total).arg(percentage);

                // Update progress dialog
                progressDialog->setMaximum(total);
                progressDialog->setValue(current);
                progressDialog->setLabelText(message);
            } else {
                message = QString("Building frame index... %1 frames").arg(current);
                progressDialog->setLabelText(message);
            }
            qDebug() << message;
        }
    }, Qt::QueuedConnection);

    // Connect decoder log messages to log viewer
    connect(m_decoder.get(), &VideoDecoder::logMessage, this, [](const QString& message) {
        qDebug() << message;
    }, Qt::QueuedConnection);

    // Try to open with video decoder in background
    QFuture<bool> decoderFuture = QtConcurrent::run([this, fileName]() {
        return m_decoder->openFile(fileName);
    });

    QFutureWatcher<bool>* decoderWatcher = new QFutureWatcher<bool>(this);
    connect(decoderWatcher, &QFutureWatcher<bool>::finished, this, [=]() {
        bool decoderSuccess = decoderWatcher->result();
        decoderWatcher->deleteLater();

        // Disconnect decoder signals
        disconnect(m_decoder.get(), &VideoDecoder::indexingProgress, nullptr, nullptr);
        disconnect(m_decoder.get(), &VideoDecoder::logMessage, nullptr, nullptr);

        if (decoderSuccess) {
            qDebug() << "Video decoder opened successfully, loading preview...";

            m_statusLabel->setText(tr("Opened: %1").arg(fileName));

            // Update all widgets with decoder
            m_barChart->setFrameIndex(&m_decoder->getFrameIndex());
            m_areaChart->setFrameIndex(&m_decoder->getFrameIndex());
            m_streamPanel->setDecoder(m_decoder.get());
            m_thumbnailBar->setDecoder(m_decoder.get());
            m_gopViewer->setFrameIndex(&m_decoder->getFrameIndex());
            m_gopViewer->setVideoDecoder(m_decoder.get());
            m_bufferPanel->setDecoder(m_decoder.get());
            m_blockStatsPanel->setVideoDecoder(m_decoder.get());

            // Generate thumbnails in background (already async)
            m_thumbnailBar->generateThumbnails();

            // Display first frame - use seekToFrame to ensure proper decoder state
            qDebug() << "Attempting to display first frame, frame count:" << m_decoder->getFrameCount();
            if (m_decoder->seekToFrame(0)) {
                qDebug() << "seekToFrame(0) succeeded, decoding next frame...";
                AVFrame* frame = m_decoder->decodeNextFrame();
                if (frame) {
                    qDebug() << "Successfully decoded first frame";
                    m_videoOutput->displayFrame(frame);
                    m_blockStatsPanel->updateStatistics(frame);
                    int currentFrame = m_decoder->getCurrentFrameNumber();
                    // Set to frame 0 since we just decoded the first frame
                    m_barChart->setCurrentFrame(0);
                    m_thumbnailBar->setCurrentFrame(0);
                    m_gopViewer->setCurrentFrame(0);
                    updateFrameLabel(0, m_decoder->getFrameCount());
                } else {
                    qDebug() << "Failed to decode first frame after seekToFrame(0)";
                }
            } else {
                qDebug() << "seekToFrame(0) failed";
            }

            qDebug() << "Video loaded successfully!";

            // Start audio playback
            m_mediaPlayer->setSource(QUrl::fromLocalFile(fileName));
            // Don't auto-play, wait for user to press Play button

            // Show video info in status bar
            QString info = QString("%1 | %2x%3 | %4 fps | %5 frames")
                .arg(m_decoder->getCodecName())
                .arg(m_decoder->getWidth())
                .arg(m_decoder->getHeight())
                .arg(m_decoder->getFrameRate(), 0, 'f', 2)
                .arg(m_decoder->getFrameCount());
            m_statusLabel->setText(info);

            // Don't close progress dialog yet if it's a TS/MP4/MKV/AVI/FLV file
            // Wait for container parsing to complete
            if (!isTSFile && !isMP4File && !isMKVFile && !isAVIFile && !isFLVFile) {
                // Close progress dialog after all initialization is complete
                progressDialog->close();
                delete progressDialog;

                // Animate Log Viewer back to dock after a short delay
                QTimer::singleShot(300, this, &MainWindow::animateLogViewerToDock);
            }
        } else {
            qDebug() << "Video decoder failed to open file";

            // For TS files, decoder failure is expected - continue with TS parsing
            // For other files, show error
            if (!isTSFile) {
                progressDialog->close();
                delete progressDialog;
                QMessageBox::critical(this, tr("Error"), tr("Failed to open video file"));

                // Animate Log Viewer back to dock even on failure
                QTimer::singleShot(300, this, &MainWindow::animateLogViewerToDock);
                return;
            }
        }

        // If it's a TS file, also parse it for stream analysis
        if (isTSFile) {
            // Create TS parser if not exists
            if (!m_tsParser) {
                m_tsParser = std::make_unique<TSParser>();
            }

            m_statusLabel->setText(tr("Parsing TS structure..."));

            // Parse TS file in background using QtConcurrent
            QFuture<bool> future = QtConcurrent::run([this, fileName]() {
                return m_tsParser->parseFile(fileName);
            });

            // Watch for completion
            QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
            connect(watcher, &QFutureWatcher<bool>::finished, this, [=]() {
                bool success = watcher->result();
                watcher->deleteLater();

                if (success) {
                    // Update TS analysis panels in main thread
                    updateTSPanels();
                    m_statusLabel->setText(tr("TS file loaded successfully"));
                } else {
                    m_statusLabel->setText(tr("Failed to parse TS structure"));
                }

                // Close progress dialog
                progressDialog->close();
                delete progressDialog;

                // Animate Log Viewer back to dock after TS parsing completes
                QTimer::singleShot(300, this, &MainWindow::animateLogViewerToDock);
            });
            watcher->setFuture(future);
            return;
        }

        // If it's an MP4 file, parse container structure
        if (isMP4File) {
            // Remove Packet List tab if present (MP4 doesn't have packets)
            for (int i = 0; i < m_centralTabs->count(); ++i) {
                if (m_centralTabs->widget(i) == m_packetView) {
                    m_centralTabs->removeTab(i);
                    break;
                }
            }

            // Create MP4 parser if not exists
            if (!m_mp4Parser) {
                m_mp4Parser = std::make_unique<MP4Parser>();
            }

            m_statusLabel->setText(tr("Parsing MP4 structure..."));

            // Connect MP4 parser log messages to log viewer
            connect(m_mp4Parser.get(), &MP4Parser::logMessage, this, [](const QString& message) {
                qDebug() << message;
            }, Qt::QueuedConnection);

            // Parse MP4 file in background
            QFuture<bool> future = QtConcurrent::run([this, fileName]() {
                return m_mp4Parser->parseFile(fileName);
            });

            QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
            connect(watcher, &QFutureWatcher<bool>::finished, this, [=]() {
                bool success = watcher->result();
                watcher->deleteLater();

                // Disconnect MP4 parser log messages
                disconnect(m_mp4Parser.get(), &MP4Parser::logMessage, nullptr, nullptr);

                if (success) {
                    updateMP4Panels();

                    // Parse NAL units for MP4 files
                    if (!m_nalUnitParser) {
                        m_nalUnitParser = std::make_unique<NALUnitParser>();
                    }

                    m_statusLabel->setText(tr("Parsing NAL units..."));

                    // Parse NAL units in background
                    QFuture<bool> nalFuture = QtConcurrent::run([this, fileName]() {
                        return m_nalUnitParser->parseFile(fileName, m_decoder.get(), "MP4");
                    });

                    QFutureWatcher<bool>* nalWatcher = new QFutureWatcher<bool>(this);
                    connect(nalWatcher, &QFutureWatcher<bool>::finished, this, [=]() {
                        bool nalSuccess = nalWatcher->result();
                        nalWatcher->deleteLater();

                        if (nalSuccess) {
                            // Add NAL Unit List tab
                            bool tabExists = false;
                            for (int i = 0; i < m_centralTabs->count(); ++i) {
                                if (m_centralTabs->widget(i) == m_nalUnitView) {
                                    tabExists = true;
                                    break;
                                }
                            }
                            if (!tabExists) {
                                m_centralTabs->addTab(m_nalUnitView, "NAL Unit List");
                            }

                            m_nalUnitView->setNALUnitParser(m_nalUnitParser.get());
                            m_nalUnitView->buildNALUnitList();

                            // Set NAL parser in hex viewer and property panel
                            m_hexViewerPanel->setNALUnitParser(m_nalUnitParser.get());
                            m_propertyPanel->setNALUnitParser(m_nalUnitParser.get());

                            m_statusLabel->setText(tr("MP4 file loaded successfully"));
                        } else {
                            m_statusLabel->setText(tr("MP4 loaded (NAL parsing failed)"));
                        }

                        // Close progress dialog
                        progressDialog->close();
                        delete progressDialog;

                        // Move Log Viewer back to dock
                        if (m_logViewer && m_logViewerDock) {
                            m_logViewerDock->setWidget(m_logViewer);
                            m_logViewerDock->show();
                            m_toggleLogViewerAction->setChecked(true);
                        }
                    });
                    nalWatcher->setFuture(nalFuture);
                } else {
                    m_statusLabel->setText(tr("Failed to parse MP4 structure"));

                    // Close progress dialog
                    progressDialog->close();
                    delete progressDialog;

                    // Move Log Viewer back to dock
                    if (m_logViewer && m_logViewerDock) {
                        m_logViewerDock->setWidget(m_logViewer);
                        m_logViewerDock->show();
                        m_toggleLogViewerAction->setChecked(true);
                    }
                }
            });
            watcher->setFuture(future);
            return;
        }

        // If it's an MKV file, parse container structure
        if (isMKVFile) {
            // Remove Packet List tab if present (MKV doesn't have packets)
            for (int i = 0; i < m_centralTabs->count(); ++i) {
                if (m_centralTabs->widget(i) == m_packetView) {
                    m_centralTabs->removeTab(i);
                    break;
                }
            }

            // Create MKV parser if not exists
            if (!m_mkvParser) {
                m_mkvParser = std::make_unique<MKVParser>();
            }

            m_statusLabel->setText(tr("Parsing MKV structure..."));

            // Parse MKV file in background
            QFuture<bool> future = QtConcurrent::run([this, fileName]() {
                return m_mkvParser->parseFile(fileName);
            });

            QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
            connect(watcher, &QFutureWatcher<bool>::finished, this, [=]() {
                bool success = watcher->result();
                watcher->deleteLater();

                if (success) {
                    updateMKVPanels();

                    // Parse NAL units for MKV files
                    if (!m_nalUnitParser) {
                        m_nalUnitParser = std::make_unique<NALUnitParser>();
                    }

                    m_statusLabel->setText(tr("Parsing NAL units..."));

                    // Parse NAL units in background
                    QFuture<bool> nalFuture = QtConcurrent::run([this, fileName]() {
                        return m_nalUnitParser->parseFile(fileName, m_decoder.get(), "MKV");
                    });

                    QFutureWatcher<bool>* nalWatcher = new QFutureWatcher<bool>(this);
                    connect(nalWatcher, &QFutureWatcher<bool>::finished, this, [=]() {
                        bool nalSuccess = nalWatcher->result();
                        nalWatcher->deleteLater();

                        if (nalSuccess) {
                            // Add NAL Unit List tab
                            bool tabExists = false;
                            for (int i = 0; i < m_centralTabs->count(); ++i) {
                                if (m_centralTabs->widget(i) == m_nalUnitView) {
                                    tabExists = true;
                                    break;
                                }
                            }
                            if (!tabExists) {
                                m_centralTabs->addTab(m_nalUnitView, "NAL Unit List");
                            }

                            m_nalUnitView->setNALUnitParser(m_nalUnitParser.get());
                            m_nalUnitView->buildNALUnitList();

                            // Set NAL parser in hex viewer and property panel
                            m_hexViewerPanel->setNALUnitParser(m_nalUnitParser.get());
                            m_propertyPanel->setNALUnitParser(m_nalUnitParser.get());

                            m_statusLabel->setText(tr("MKV file loaded successfully"));
                        } else {
                            m_statusLabel->setText(tr("MKV loaded (NAL parsing failed)"));
                        }

                        // Close progress dialog
                        progressDialog->close();
                        delete progressDialog;

                        // Move Log Viewer back to dock
                        if (m_logViewer && m_logViewerDock) {
                            m_logViewerDock->setWidget(m_logViewer);
                            m_logViewerDock->show();
                            m_toggleLogViewerAction->setChecked(true);
                        }
                    });
                    nalWatcher->setFuture(nalFuture);
                } else {
                    m_statusLabel->setText(tr("Failed to parse MKV structure"));

                    // Close progress dialog
                    progressDialog->close();
                    delete progressDialog;

                    // Move Log Viewer back to dock
                    if (m_logViewer && m_logViewerDock) {
                        m_logViewerDock->setWidget(m_logViewer);
                        m_logViewerDock->show();
                        m_toggleLogViewerAction->setChecked(true);
                    }
                }
            });
            watcher->setFuture(future);
            return;
        }

        // If it's an AVI file, parse container structure
        if (isAVIFile) {
            // Remove Packet List tab if present (AVI doesn't have packets)
            for (int i = 0; i < m_centralTabs->count(); ++i) {
                if (m_centralTabs->widget(i) == m_packetView) {
                    m_centralTabs->removeTab(i);
                    break;
                }
            }

            // Create AVI parser if not exists
            if (!m_aviParser) {
                m_aviParser = std::make_unique<AVIParser>();
            }

            m_statusLabel->setText(tr("Parsing AVI structure..."));

            // Parse AVI file in background
            QFuture<bool> future = QtConcurrent::run([this, fileName]() {
                return m_aviParser->parseFile(fileName);
            });

            QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
            connect(watcher, &QFutureWatcher<bool>::finished, this, [=]() {
                bool success = watcher->result();
                watcher->deleteLater();

                if (success) {
                    updateAVIPanels();
                    m_statusLabel->setText(tr("AVI file loaded successfully"));
                } else {
                    m_statusLabel->setText(tr("Failed to parse AVI structure"));
                }

                // Close progress dialog
                progressDialog->close();
                delete progressDialog;

                // Move Log Viewer back to dock (without animation)
                if (m_logViewer && m_logViewerDock) {
                    m_logViewerDock->setWidget(m_logViewer);
                    m_logViewerDock->show();
                    m_toggleLogViewerAction->setChecked(true);
                }
            });
            watcher->setFuture(future);
            return;
        }

        // If it's an FLV file, parse container structure
        if (isFLVFile) {
            // Remove Packet List tab if present (FLV doesn't have packets)
            for (int i = 0; i < m_centralTabs->count(); ++i) {
                if (m_centralTabs->widget(i) == m_packetView) {
                    m_centralTabs->removeTab(i);
                    break;
                }
            }

            // Create FLV parser if not exists
            if (!m_flvParser) {
                m_flvParser = std::make_unique<FLVParser>();
            }

            m_statusLabel->setText(tr("Parsing FLV structure..."));

            // Parse FLV file in background
            QFuture<bool> future = QtConcurrent::run([this, fileName]() {
                return m_flvParser->parseFile(fileName);
            });

            QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
            connect(watcher, &QFutureWatcher<bool>::finished, this, [=]() {
                bool success = watcher->result();
                watcher->deleteLater();

                if (success) {
                    updateFLVPanels();
                    m_statusLabel->setText(tr("FLV file loaded successfully"));
                } else {
                    m_statusLabel->setText(tr("Failed to parse FLV structure"));
                }

                // Close progress dialog
                progressDialog->close();
                delete progressDialog;

                // Move Log Viewer back to dock (without animation)
                if (m_logViewer && m_logViewerDock) {
                    m_logViewerDock->setWidget(m_logViewer);
                    m_logViewerDock->show();
                    m_toggleLogViewerAction->setChecked(true);
                }
            });
            watcher->setFuture(future);
            return;
        }
    });
    decoderWatcher->setFuture(decoderFuture);
}

void MainWindow::play() {
    if (!m_decoder->isOpen()) {
        return;
    }

    // If at end of video, restart from beginning
    int currentFrame = m_decoder->getCurrentFrameNumber();
    int frameCount = m_decoder->getFrameCount();
    if (currentFrame >= frameCount - 1) {
        if (m_decoder->seekToFrame(0)) {
            AVFrame* frame = m_decoder->decodeNextFrame();
            if (frame) {
                m_videoOutput->displayFrame(frame);
                m_blockStatsPanel->updateStatistics(frame);
                m_barChart->setCurrentFrame(0);
                m_thumbnailBar->setCurrentFrame(0);
                m_gopViewer->setCurrentFrame(0);
                updateFrameLabel(0, frameCount);
            }
        }
        // Restart audio from beginning
        m_mediaPlayer->setPosition(0);
    }

    m_isPlaying = true;
    double frameRate = m_decoder->getFrameRate();
    int interval = frameRate > 0 ? static_cast<int>(1000.0 / frameRate) : 40;
    m_playbackTimer->start(interval);

    // Start audio playback
    m_mediaPlayer->play();

    m_statusLabel->setText(tr("Playing..."));
}

void MainWindow::pause() {
    m_isPlaying = false;
    m_playbackTimer->stop();

    // Pause audio playback
    m_mediaPlayer->pause();

    m_statusLabel->setText(tr("Paused"));
}

void MainWindow::stepForward() {
    if (!m_decoder->isOpen()) {
        return;
    }

    AVFrame* frame = m_decoder->decodeNextFrame();
    if (frame) {
        m_videoOutput->displayFrame(frame);
        m_blockStatsPanel->updateStatistics(frame);
        int currentFrame = m_decoder->getCurrentFrameNumber();
        int frameCount = m_decoder->getFrameCount();
        // getCurrentFrameNumber() returns next position, clamp for display
        if (currentFrame >= frameCount) {
            currentFrame = frameCount - 1;
        }
        m_barChart->setCurrentFrame(currentFrame);
        m_thumbnailBar->setCurrentFrame(currentFrame);
        m_gopViewer->setCurrentFrame(currentFrame);
        m_statusLabel->setText(tr("Frame %1 / %2")
            .arg(currentFrame + 1)
            .arg(frameCount));
        updateFrameLabel(currentFrame, frameCount);
    } else {
        // Reached end of video
        pause();
        m_statusLabel->setText(tr("End of video"));
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
                m_blockStatsPanel->updateStatistics(frame);
                m_barChart->setCurrentFrame(currentFrame - 1);
                m_thumbnailBar->setCurrentFrame(currentFrame - 1);
                m_gopViewer->setCurrentFrame(currentFrame - 1);
                m_statusLabel->setText(tr("Frame %1 / %2")
                    .arg(currentFrame)
                    .arg(m_decoder->getFrameCount()));
                updateFrameLabel(currentFrame - 1, m_decoder->getFrameCount());
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
            m_blockStatsPanel->updateStatistics(frame);
            m_barChart->setCurrentFrame(frameNumber);
            m_thumbnailBar->setCurrentFrame(frameNumber);
            m_gopViewer->setCurrentFrame(frameNumber);
            m_statusLabel->setText(tr("Frame %1 / %2")
                .arg(frameNumber + 1)
                .arg(m_decoder->getFrameCount()));
            updateFrameLabel(frameNumber, m_decoder->getFrameCount());

            // Sync audio position to current frame
            double frameRate = m_decoder->getFrameRate();
            if (frameRate > 0) {
                qint64 positionMs = static_cast<qint64>(frameNumber * 1000.0 / frameRate);
                m_mediaPlayer->setPosition(positionMs);
            }
        }
    }
}

void MainWindow::onPlaybackTimer() {
    stepForward();
}

void MainWindow::toggleStreamPanel() {
    bool visible = !m_streamPanelDock->isVisible();
    m_streamPanelDock->setVisible(visible);
    m_toggleStreamAction->setChecked(visible);
}

void MainWindow::toggleExplorerPanel() {
    bool visible = !m_explorerPanelDock->isVisible();
    m_explorerPanelDock->setVisible(visible);
    m_toggleExplorerAction->setChecked(visible);
}

void MainWindow::toggleBarChart() {
    bool visible = !m_barChartDock->isVisible();
    m_barChartDock->setVisible(visible);
    m_toggleBarChartAction->setChecked(visible);
}

void MainWindow::toggleAreaChart() {
    bool visible = !m_areaChartDock->isVisible();
    m_areaChartDock->setVisible(visible);
    m_toggleAreaChartAction->setChecked(visible);
}

void MainWindow::toggleThumbnailBar() {
    bool visible = !m_thumbnailDock->isVisible();
    m_thumbnailDock->setVisible(visible);
    m_toggleThumbnailAction->setChecked(visible);
}

void MainWindow::toggleOverlayPanel() {
    bool visible = !m_overlayPanelDock->isVisible();
    m_overlayPanelDock->setVisible(visible);
    m_toggleOverlayPanelAction->setChecked(visible);
}

void MainWindow::toggleGOPViewer() {
    bool visible = !m_gopViewerDock->isVisible();
    m_gopViewerDock->setVisible(visible);
    m_toggleGOPViewerAction->setChecked(visible);
}

void MainWindow::togglePropertyPanel() {
    bool visible = !m_propertyPanelDock->isVisible();
    m_propertyPanelDock->setVisible(visible);
    m_togglePropertyPanelAction->setChecked(visible);
}

void MainWindow::toggleHexViewer() {
    bool visible = !m_hexViewerPanelDock->isVisible();
    m_hexViewerPanelDock->setVisible(visible);
    m_toggleHexViewerAction->setChecked(visible);
}

void MainWindow::toggleMessages() {
    bool visible = !m_messagesPanelDock->isVisible();
    m_messagesPanelDock->setVisible(visible);
    m_toggleMessagesAction->setChecked(visible);
}

void MainWindow::toggleTR101290() {
    bool visible = !m_tr101290PanelDock->isVisible();
    m_tr101290PanelDock->setVisible(visible);
    m_toggleTR101290Action->setChecked(visible);
}

void MainWindow::toggleTimeDynamics() {
    bool visible = !m_timeDynamicsPanelDock->isVisible();
    m_timeDynamicsPanelDock->setVisible(visible);
    m_toggleTimeDynamicsAction->setChecked(visible);
}

void MainWindow::toggleBitrate() {
    bool visible = !m_bitratePanelDock->isVisible();
    m_bitratePanelDock->setVisible(visible);
    m_toggleBitrateAction->setChecked(visible);
}

void MainWindow::toggleBuffer() {
    bool visible = !m_bufferPanelDock->isVisible();
    m_bufferPanelDock->setVisible(visible);
    m_toggleBufferAction->setChecked(visible);
}

void MainWindow::toggleGraphics() {
    bool visible = !m_graphicsPanelDock->isVisible();
    m_graphicsPanelDock->setVisible(visible);
    m_toggleGraphicsAction->setChecked(visible);
}

void MainWindow::toggleComments() {
    bool visible = !m_commentsPanelDock->isVisible();
    m_commentsPanelDock->setVisible(visible);
    m_toggleCommentsAction->setChecked(visible);
}

void MainWindow::toggleEPG() {
    bool visible = !m_epgPanelDock->isVisible();
    m_epgPanelDock->setVisible(visible);
    m_toggleEPGAction->setChecked(visible);
}

void MainWindow::toggleBlockStats() {
    bool visible = !m_blockStatsPanelDock->isVisible();
    m_blockStatsPanelDock->setVisible(visible);
    m_toggleBlockStatsAction->setChecked(visible);
}

void MainWindow::toggleLogViewer() {
    bool visible = !m_logViewerDock->isVisible();
    m_logViewerDock->setVisible(visible);
    m_toggleLogViewerAction->setChecked(visible);
}

void MainWindow::toggleMotionVectors() {
    m_videoOutput->toggleOverlay(OverlayType::MotionVectors);
    bool checked = m_videoOutput->isOverlayEnabled(OverlayType::MotionVectors);
    m_overlayMotionVectorsAction->setChecked(checked);
    m_overlayPanel->setMotionVectorsChecked(checked);
}

void MainWindow::togglePartitions() {
    m_videoOutput->toggleOverlay(OverlayType::Partitions);
    bool checked = m_videoOutput->isOverlayEnabled(OverlayType::Partitions);
    m_overlayPartitionsAction->setChecked(checked);
    m_overlayPanel->setPartitionsChecked(checked);
}

void MainWindow::toggleFrameTypes() {
    m_videoOutput->toggleOverlay(OverlayType::FrameTypes);
    bool checked = m_videoOutput->isOverlayEnabled(OverlayType::FrameTypes);
    m_overlayFrameTypesAction->setChecked(checked);
    m_overlayPanel->setFrameTypesChecked(checked);
}

void MainWindow::toggleQPHeatmap() {
    m_videoOutput->toggleOverlay(OverlayType::QuantizationParameter);
    bool checked = m_videoOutput->isOverlayEnabled(OverlayType::QuantizationParameter);
    m_overlayQPHeatmapAction->setChecked(checked);
    m_overlayPanel->setQPHeatmapChecked(checked);
}

void MainWindow::toggleCursorMode() {
    bool enabled = !m_videoOutput->isCursorModeEnabled();
    m_videoOutput->setCursorMode(enabled);
    m_cursorModeAction->setChecked(enabled);
}

void MainWindow::toggleStandardGridMode() {
    bool enabled = !m_videoOutput->isStandardGridMode();
    m_videoOutput->setStandardGridMode(enabled);
    m_standardGridModeAction->setChecked(enabled);
}

void MainWindow::toggleGOPDisplayMode() {
    m_gopViewer->toggleDisplayMode();
}

void MainWindow::toggleGOPDependencyArrows() {
    m_gopViewer->toggleDependencyArrows();
}

void MainWindow::updateUI() {
    // Update UI state based on decoder state
}

void MainWindow::updateFrameLabel(int currentFrame, int totalFrames) {
    QLabel* frameLabel = findChild<QLabel*>("frameLabel");
    if (frameLabel) {
        frameLabel->setText(tr("Frame: %1 / %2").arg(currentFrame + 1).arg(totalFrames));
    }
}

void MainWindow::updateTSPanels() {
    // Add Packet List tab if not already added
    bool hasPacketList = false;
    for (int i = 0; i < m_centralTabs->count(); ++i) {
        if (m_centralTabs->widget(i) == m_packetView) {
            hasPacketList = true;
            break;
        }
    }
    if (!hasPacketList) {
        m_centralTabs->addTab(m_packetView, "Packet List");
    }

    // Update TS analysis panels
    m_explorerPanel->setTSParser(m_tsParser.get());
    m_propertyPanel->setTSParser(m_tsParser.get());
    m_hexViewerPanel->setTSParser(m_tsParser.get());
    m_packetView->setTSParser(m_tsParser.get());
    m_messagesPanel->setTSParser(m_tsParser.get());
    m_tr101290Panel->setTSParser(m_tsParser.get());

    // Connect Messages Panel to TR 101-290 Panel
    m_messagesPanel->setTR101290Panel(m_tr101290Panel);

    m_timeDynamicsPanel->setTSParser(m_tsParser.get());
    m_bitratePanel->setTSParser(m_tsParser.get());
    m_graphicsPanel->setTSParser(m_tsParser.get());
    m_commentsPanel->setTSParser(m_tsParser.get());
    m_epgPanel->setTSParser(m_tsParser.get());

    // Update stream panel with TS info (in addition to video info)
    m_streamPanel->setTSParser(m_tsParser.get());

    // Connect signals
    connect(m_explorerPanel, &ExplorerPanel::pidSelected,
            m_propertyPanel, &PropertyPanel::onPIDSelected, Qt::UniqueConnection);
    connect(m_explorerPanel, &ExplorerPanel::pidSelected,
            m_hexViewerPanel, &HexViewerPanel::onPIDSelected, Qt::UniqueConnection);
    connect(m_packetView, &PacketView::packetSelected,
            m_propertyPanel, &PropertyPanel::displayPacket, Qt::UniqueConnection);
    connect(m_packetView, &PacketView::packetSelected,
            m_hexViewerPanel, &HexViewerPanel::displayPacket, Qt::UniqueConnection);

    // Connect NAL Unit View signals
    connect(m_nalUnitView, &NALUnitView::nalUnitSelected,
            m_hexViewerPanel, &HexViewerPanel::displayNALUnit, Qt::UniqueConnection);
    connect(m_nalUnitView, &NALUnitView::frameSelected,
            this, &MainWindow::onFrameClicked, Qt::UniqueConnection);

    connect(m_propertyPanel, &PropertyPanel::addToGraphics,
            m_graphicsPanel, &GraphicsPanel::addParameter, Qt::UniqueConnection);
    connect(m_messagesPanel, &MessagesPanel::messageDoubleClicked,
            m_packetView, &PacketView::setCurrentPacket, Qt::UniqueConnection);
    connect(m_tr101290Panel, &TR101290Panel::errorDoubleClicked,
            m_packetView, &PacketView::setCurrentPacket, Qt::UniqueConnection);
    connect(m_timeDynamicsPanel, &TimeDynamicsPanel::packetClicked,
            m_packetView, &PacketView::setCurrentPacket, Qt::UniqueConnection);
    connect(m_bitratePanel, &BitratePanel::packetClicked,
            m_packetView, &PacketView::setCurrentPacket, Qt::UniqueConnection);
    connect(m_commentsPanel, &CommentsPanel::commentSelected,
            this, [this](int64_t offset, int packetIndex, int frameIndex) {
        Q_UNUSED(frameIndex);
        if (packetIndex >= 0) {
            m_packetView->setCurrentPacket(packetIndex);
        } else {
            const auto& packets = m_tsParser->getPackets();
            for (int i = 0; i < packets.size(); ++i) {
                if (packets[i].offset == offset) {
                    m_packetView->setCurrentPacket(i);
                    break;
                }
            }
        }
    });

    // Connect Explorer Panel mode signals to Property Panel
    connect(m_explorerPanel, &ExplorerPanel::setCompareMode,
            this, [this](uint16_t pid) {
        Q_UNUSED(pid);
        m_propertyPanel->setMode(PropertyMode::Compare);
    });
    connect(m_explorerPanel, &ExplorerPanel::setSyncMode,
            this, [this](uint16_t pid) {
        Q_UNUSED(pid);
        m_propertyPanel->setMode(PropertyMode::Sync);
    });

    // Update dock title
    m_explorerPanelDock->setWindowTitle(tr("TS Explorer"));

    // Show TS analysis panels
    m_explorerPanelDock->show();
    m_propertyPanelDock->show();
    m_hexViewerPanelDock->show();
    m_messagesPanelDock->show();
    m_tr101290PanelDock->show();
    m_epgPanelDock->show();

    // Update menu actions
    m_toggleExplorerAction->setChecked(true);
    m_togglePropertyPanelAction->setChecked(true);
    m_toggleHexViewerAction->setChecked(true);
    m_toggleMessagesAction->setChecked(true);
    m_toggleTR101290Action->setChecked(true);
    m_toggleEPGAction->setChecked(true);
}

void MainWindow::updateMP4Panels() {
    qDebug() << "updateMP4Panels called";

    if (!m_mp4Parser) {
        qDebug() << "updateMP4Panels: m_mp4Parser is null";
        return;
    }

    if (!m_explorerPanel) {
        qDebug() << "updateMP4Panels: m_explorerPanel is null";
        return;
    }

    if (!m_propertyPanel) {
        qDebug() << "updateMP4Panels: m_propertyPanel is null";
        return;
    }

    if (!m_hexViewerPanel) {
        qDebug() << "updateMP4Panels: m_hexViewerPanel is null";
        return;
    }

    qDebug() << "updateMP4Panels: setting parsers...";

    // Update Explorer panel with MP4 structure
    m_explorerPanel->setMP4Parser(m_mp4Parser.get());
    qDebug() << "updateMP4Panels: explorer panel updated";

    // Update Property panel with MP4 parser
    m_propertyPanel->setMP4Parser(m_mp4Parser.get());
    qDebug() << "updateMP4Panels: property panel updated";

    // Update Hex Viewer panel with MP4 parser
    m_hexViewerPanel->setMP4Parser(m_mp4Parser.get());
    qDebug() << "updateMP4Panels: hex viewer panel updated";

    // Connect signals for MP4 mode
    connect(m_explorerPanel, &ExplorerPanel::packetSelected,
            m_propertyPanel, &PropertyPanel::onAtomSelected, Qt::UniqueConnection);
    qDebug() << "updateMP4Panels: connected explorer to property";

    // Connect Property Panel to Graphics Panel
    connect(m_propertyPanel, &PropertyPanel::addToGraphics,
            m_graphicsPanel, &GraphicsPanel::addParameter, Qt::UniqueConnection);
    qDebug() << "updateMP4Panels: connected property to graphics";

    // Connect Explorer to Hex Viewer (pass offset and size)
    connect(m_explorerPanel, &ExplorerPanel::packetSelected,
            this, [this](int64_t offset) {
        // Find atom by offset to get size
        const auto& atoms = m_mp4Parser->getAtoms();
        std::function<const MP4Atom*(const QVector<MP4Atom>&, int64_t)> findAtom;
        findAtom = [&](const QVector<MP4Atom>& atms, int64_t off) -> const MP4Atom* {
            for (const auto& atom : atms) {
                if (atom.offset == off) return &atom;
                if (!atom.children.isEmpty()) {
                    const MP4Atom* found = findAtom(atom.children, off);
                    if (found) return found;
                }
            }
            return nullptr;
        };
        const MP4Atom* atom = findAtom(atoms, offset);
        if (atom) {
            m_hexViewerPanel->displayAtom(atom->offset, atom->size);
        }
    });
    qDebug() << "updateMP4Panels: connected explorer to hex viewer";

    // Connect NAL Unit View signals
    connect(m_nalUnitView, &NALUnitView::nalUnitSelected,
            m_hexViewerPanel, &HexViewerPanel::displayNALUnit, Qt::UniqueConnection);
    connect(m_nalUnitView, &NALUnitView::nalUnitSelected,
            m_propertyPanel, &PropertyPanel::displayNALUnit, Qt::UniqueConnection);
    connect(m_nalUnitView, &NALUnitView::audioFrameSelected,
            m_hexViewerPanel, &HexViewerPanel::displayAudioFrame, Qt::UniqueConnection);
    connect(m_nalUnitView, &NALUnitView::audioFrameSelected,
            m_propertyPanel, &PropertyPanel::displayAudioFrame, Qt::UniqueConnection);
    connect(m_nalUnitView, &NALUnitView::frameSelected,
            this, &MainWindow::onFrameClicked, Qt::UniqueConnection);
    qDebug() << "updateMP4Panels: connected NAL Unit View signals";

    // Update dock title
    m_explorerPanelDock->setWindowTitle(tr("MP4 Explorer"));
    qDebug() << "updateMP4Panels: updated dock title";

    // Show Explorer and Property panels (don't auto-show Hex Viewer to avoid freeze)
    m_explorerPanelDock->show();
    m_propertyPanelDock->show();
    m_toggleExplorerAction->setChecked(true);
    m_togglePropertyPanelAction->setChecked(true);
    qDebug() << "updateMP4Panels: completed successfully";
}

void MainWindow::updateMKVPanels() {
    // Update Explorer panel with MKV structure
    m_explorerPanel->setMKVParser(m_mkvParser.get());

    // Update Property panel with MKV parser
    m_propertyPanel->setMKVParser(m_mkvParser.get());

    // Update Hex Viewer panel with MKV parser
    m_hexViewerPanel->setMKVParser(m_mkvParser.get());

    // Connect signals for MKV mode
    connect(m_explorerPanel, &ExplorerPanel::packetSelected,
            m_propertyPanel, &PropertyPanel::onElementSelected, Qt::UniqueConnection);

    // Connect Property Panel to Graphics Panel
    connect(m_propertyPanel, &PropertyPanel::addToGraphics,
            m_graphicsPanel, &GraphicsPanel::addParameter, Qt::UniqueConnection);

    // Connect Explorer to Hex Viewer (pass offset and size)
    connect(m_explorerPanel, &ExplorerPanel::packetSelected,
            this, [this](int64_t offset) {
        // Find element by offset to get size
        const auto& elements = m_mkvParser->getElements();
        std::function<const EBMLElement*(const QVector<EBMLElement>&, int64_t)> findElement;
        findElement = [&](const QVector<EBMLElement>& elems, int64_t off) -> const EBMLElement* {
            for (const auto& elem : elems) {
                if (elem.offset == off) return &elem;
                if (!elem.children.isEmpty()) {
                    const EBMLElement* found = findElement(elem.children, off);
                    if (found) return found;
                }
            }
            return nullptr;
        };
        const EBMLElement* element = findElement(elements, offset);
        if (element) {
            m_hexViewerPanel->displayElement(element->offset, element->totalSize);
        }
    });

    // Connect NAL Unit View signals
    connect(m_nalUnitView, &NALUnitView::nalUnitSelected,
            m_hexViewerPanel, &HexViewerPanel::displayNALUnit, Qt::UniqueConnection);
    connect(m_nalUnitView, &NALUnitView::nalUnitSelected,
            m_propertyPanel, &PropertyPanel::displayNALUnit, Qt::UniqueConnection);
    connect(m_nalUnitView, &NALUnitView::audioFrameSelected,
            m_hexViewerPanel, &HexViewerPanel::displayAudioFrame, Qt::UniqueConnection);
    connect(m_nalUnitView, &NALUnitView::audioFrameSelected,
            m_propertyPanel, &PropertyPanel::displayAudioFrame, Qt::UniqueConnection);
    connect(m_nalUnitView, &NALUnitView::frameSelected,
            this, &MainWindow::onFrameClicked, Qt::UniqueConnection);

    // Update dock title
    m_explorerPanelDock->setWindowTitle(tr("MKV Explorer"));

    // Show Explorer and Property panels (don't auto-show Hex Viewer to avoid freeze)
    m_explorerPanelDock->show();
    m_propertyPanelDock->show();
    m_toggleExplorerAction->setChecked(true);
    m_togglePropertyPanelAction->setChecked(true);
}

void MainWindow::updateAVIPanels() {
    // Update Explorer panel with AVI structure
    m_explorerPanel->setAVIParser(m_aviParser.get());

    // Update Property panel with AVI parser
    m_propertyPanel->setAVIParser(m_aviParser.get());

    // Update Hex Viewer panel with AVI parser
    m_hexViewerPanel->setAVIParser(m_aviParser.get());

    // Connect signals for AVI mode
    connect(m_explorerPanel, &ExplorerPanel::packetSelected,
            m_propertyPanel, &PropertyPanel::onChunkSelected, Qt::UniqueConnection);

    // Connect Property Panel to Graphics Panel
    connect(m_propertyPanel, &PropertyPanel::addToGraphics,
            m_graphicsPanel, &GraphicsPanel::addParameter, Qt::UniqueConnection);

    // Connect Explorer to Hex Viewer (pass offset and size)
    connect(m_explorerPanel, &ExplorerPanel::packetSelected,
            this, [this](int64_t offset) {
        // Find chunk by offset to get size
        const auto& chunks = m_aviParser->getChunks();
        std::function<const AVIChunk*(const QVector<AVIChunk>&, int64_t)> findChunk;
        findChunk = [&](const QVector<AVIChunk>& chnks, int64_t off) -> const AVIChunk* {
            for (const auto& chunk : chnks) {
                if (chunk.offset == off) return &chunk;
                if (!chunk.children.isEmpty()) {
                    const AVIChunk* found = findChunk(chunk.children, off);
                    if (found) return found;
                }
            }
            return nullptr;
        };
        const AVIChunk* chunk = findChunk(chunks, offset);
        if (chunk) {
            m_hexViewerPanel->displayChunk(chunk->offset, chunk->totalSize);
        }
    });

    // Update dock title
    m_explorerPanelDock->setWindowTitle(tr("AVI Explorer"));

    // Show Explorer and Property panels (don't auto-show Hex Viewer to avoid freeze)
    m_explorerPanelDock->show();
    m_propertyPanelDock->show();
    m_toggleExplorerAction->setChecked(true);
    m_togglePropertyPanelAction->setChecked(true);
}

void MainWindow::updateFLVPanels() {
    // Update Explorer panel with FLV structure
    m_explorerPanel->setFLVParser(m_flvParser.get());

    // Update Property panel with FLV parser
    m_propertyPanel->setFLVParser(m_flvParser.get());

    // Update Hex Viewer panel with FLV parser
    m_hexViewerPanel->setFLVParser(m_flvParser.get());

    // Connect signals for FLV mode
    connect(m_explorerPanel, &ExplorerPanel::packetSelected,
            m_propertyPanel, &PropertyPanel::onTagSelected, Qt::UniqueConnection);

    // Connect Property Panel to Graphics Panel
    connect(m_propertyPanel, &PropertyPanel::addToGraphics,
            m_graphicsPanel, &GraphicsPanel::addParameter, Qt::UniqueConnection);

    // Connect Explorer to Hex Viewer (pass offset and size)
    connect(m_explorerPanel, &ExplorerPanel::packetSelected,
            this, [this](int64_t offset) {
        // Find tag by offset to get size
        const auto& tags = m_flvParser->getTags();
        for (const auto& tag : tags) {
            if (tag.offset == offset) {
                m_hexViewerPanel->displayTag(tag.offset, tag.totalSize);
                break;
            }
        }
    });

    // Update dock title
    m_explorerPanelDock->setWindowTitle(tr("FLV Explorer"));

    // Show Explorer and Property panels (don't auto-show Hex Viewer to avoid freeze)
    m_explorerPanelDock->show();
    m_propertyPanelDock->show();
    m_toggleExplorerAction->setChecked(true);
    m_togglePropertyPanelAction->setChecked(true);
}

void MainWindow::saveStreamInfo() {
    if (!m_tsParser) {
        QMessageBox::warning(this, tr("Error"), tr("No TS file loaded. Please open a TS file first."));
        return;
    }

    SaveStreamInfoDialog dialog(m_tsParser.get(), this);
    dialog.exec();
}

void MainWindow::exportFrameAsYUV() {
    if (!m_decoder->isOpen()) {
        QMessageBox::warning(this, tr("Error"), tr("No video file loaded. Please open a video file first."));
        return;
    }

    YUVExportDialog dialog(m_decoder.get(), this);
    dialog.exec();
}

void MainWindow::exportFrameRangeAsYUV() {
    if (!m_decoder->isOpen()) {
        QMessageBox::warning(this, tr("Error"), tr("No video file loaded. Please open a video file first."));
        return;
    }

    YUVExportDialog dialog(m_decoder.get(), this);
    dialog.exec();
}

void MainWindow::appendLog(const QString& message, QtMsgType type) {
    if (m_logViewer) {
        m_logViewer->appendLog(message, type);
    }
}

void MainWindow::animateLogViewerToDock() {
    // Create animation for moving log viewer back to dock
    QPropertyAnimation* animation = new QPropertyAnimation(m_logViewer, "geometry");
    animation->setDuration(500);  // 500ms animation
    animation->setEasingCurve(QEasingCurve::InOutQuad);

    // Start position (current center position)
    QRect startRect = m_logViewer->geometry();
    animation->setStartValue(startRect);

    // End position (bottom dock area, temporarily show to get position)
    m_logViewerDock->setWidget(m_logViewer);
    m_logViewerDock->show();

    // Get the dock widget's geometry in main window coordinates
    QRect dockRect = m_logViewerDock->geometry();
    QPoint dockPos = m_logViewerDock->mapTo(this, QPoint(0, 0));
    QRect endRect(dockPos, m_logViewerDock->size());

    // Hide dock temporarily during animation
    m_logViewerDock->hide();

    // Restore log viewer as floating widget for animation
    m_logViewerDock->setWidget(nullptr);
    m_logViewer->setParent(this);
    m_logViewer->setWindowFlags(Qt::Widget);
    m_logViewer->setGeometry(startRect);
    m_logViewer->show();
    m_logViewer->raise();

    animation->setEndValue(endRect);

    // When animation finishes, move log viewer back to dock
    connect(animation, &QPropertyAnimation::finished, this, [this]() {
        m_logViewerDock->setWidget(m_logViewer);
        m_logViewerDock->show();
        m_toggleLogViewerAction->setChecked(true);
    });

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::showQualityMetrics() {
    if (!m_decoder || !m_decoder->isOpen()) {
        QMessageBox::warning(this, tr("No Video Loaded"),
            tr("Please open a video file first. This will be used as the reference video."));
        return;
    }

    QString currentFile = m_decoder->getFileName();
    QualityMetricsDialog dialog(currentFile, this);
    dialog.exec();
}

void MainWindow::showReferenceComparison() {
    if (!m_decoder || !m_decoder->isOpen()) {
        QMessageBox::warning(this, tr("No Video Loaded"),
            tr("Please open a video file first."));
        return;
    }

    ReferenceComparisonDialog dialog(m_decoder.get(), this);
    dialog.exec();
}

void MainWindow::showYUVViewer() {
    YUVViewerDialog dialog(this);
    dialog.exec();
}

void MainWindow::showAboutDialog() {
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::showDuplicateFrameDetection() {
    if (!m_decoder || !m_decoder->isOpen()) {
        QMessageBox::warning(this, tr("No Video Loaded"),
            tr("Please open a video file first."));
        return;
    }

    DuplicateFrameDetectionDialog dialog(m_decoder.get(), this);

    // Connect dialog signal to seek slot
    connect(&dialog, &DuplicateFrameDetectionDialog::seekToFrame,
            this, &MainWindow::onFrameClicked);

    // Connect duplicate frames signal to GOP viewer
    connect(&dialog, &DuplicateFrameDetectionDialog::duplicateFramesDetected,
            m_gopViewer, &GOPViewer::setDuplicateFrames);

    dialog.exec();
}

void MainWindow::captureScreenshot() {
    if (!m_decoder || !m_decoder->isOpen()) {
        QMessageBox::warning(this, tr("No Video Loaded"),
            tr("Please open a video file first."));
        return;
    }

    // Get the actual rendered image from VideoOutput (without black borders)
    QImage screenshot = m_videoOutput->getCurrentImage();

    if (screenshot.isNull()) {
        QMessageBox::warning(this, tr("Screenshot Failed"),
            tr("Failed to capture screenshot. No frame available."));
        return;
    }

    // Ask user for save location
    QString defaultFileName = QString("frame_%1.png").arg(m_decoder->getCurrentFrameNumber());
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save Screenshot"),
        defaultFileName,
        tr("PNG Image (*.png);;JPEG Image (*.jpg *.jpeg);;All Files (*)")
    );

    if (fileName.isEmpty()) {
        return;
    }

    // Determine format from extension
    QString format = "PNG";
    if (fileName.endsWith(".jpg", Qt::CaseInsensitive) ||
        fileName.endsWith(".jpeg", Qt::CaseInsensitive)) {
        format = "JPEG";
    }

    // Save screenshot
    if (screenshot.save(fileName, format.toUtf8().constData())) {
        QMessageBox::information(this, tr("Screenshot Saved"),
            tr("Screenshot saved to:\n%1").arg(fileName));
    } else {
        QMessageBox::critical(this, tr("Save Failed"),
            tr("Failed to save screenshot to:\n%1").arg(fileName));
    }
}

void MainWindow::exportCSVMetrics() {
    if (!m_decoder || !m_decoder->isOpen()) {
        QMessageBox::warning(this, tr("No Video Loaded"),
            tr("Please open a video file first."));
        return;
    }

    CSVExportDialog dialog(m_decoder.get(), this);
    dialog.exec();
}

void MainWindow::openRecentFile() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        QString filePath = action->data().toString();
        if (QFile::exists(filePath)) {
            loadFile(filePath);
        } else {
            QMessageBox::warning(this, tr("File Not Found"),
                tr("The file no longer exists:\n%1").arg(filePath));
            // Remove from recent files
            QSettings settings("VideoStudio", "VideoStudio");
            QStringList files = settings.value("recentFiles").toStringList();
            files.removeAll(filePath);
            settings.setValue("recentFiles", files);
            updateRecentFilesMenu();
        }
    }
}

void MainWindow::clearRecentFiles() {
    QSettings settings("VideoStudio", "VideoStudio");
    settings.remove("recentFiles");
    updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu() {
    QSettings settings("VideoStudio", "VideoStudio");
    QStringList files = settings.value("recentFiles").toStringList();

    int numRecentFiles = qMin(files.size(), MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = QString("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
        m_recentFileActions[i]->setText(text);
        m_recentFileActions[i]->setData(files[i]);
        m_recentFileActions[i]->setToolTip(files[i]);
        m_recentFileActions[i]->setVisible(true);
    }

    for (int i = numRecentFiles; i < MaxRecentFiles; ++i) {
        m_recentFileActions[i]->setVisible(false);
    }

    m_recentFilesMenu->setEnabled(numRecentFiles > 0);
}

void MainWindow::addToRecentFiles(const QString& filePath) {
    QSettings settings("VideoStudio", "VideoStudio");
    QStringList files = settings.value("recentFiles").toStringList();

    // Remove if already exists (to move it to top)
    files.removeAll(filePath);

    // Add to beginning
    files.prepend(filePath);

    // Keep only MaxRecentFiles
    while (files.size() > MaxRecentFiles) {
        files.removeLast();
    }

    settings.setValue("recentFiles", files);
    updateRecentFilesMenu();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    // Accept drag if it contains URLs (files)
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();

        // Process only the first file
        if (!urlList.isEmpty()) {
            QString filePath = urlList.first().toLocalFile();

            // Check if file exists
            if (QFile::exists(filePath)) {
                // Check if it's a supported video file
                QStringList supportedExtensions = {
                    ".mp4", ".mkv", ".avi", ".flv", ".mov",
                    ".ts", ".m2ts", ".mts",
                    ".h264", ".h265", ".hevc", ".264", ".265",
                    ".yuv", ".webm", ".m4v"
                };

                bool isSupported = false;
                for (const QString& ext : supportedExtensions) {
                    if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
                        isSupported = true;
                        break;
                    }
                }

                if (isSupported) {
                    // Load the file
                    loadFile(filePath);
                    event->acceptProposedAction();
                } else {
                    QMessageBox::warning(this, tr("Unsupported File"),
                        tr("The dropped file is not a supported video format:\n%1").arg(filePath));
                }
            } else {
                QMessageBox::warning(this, tr("File Not Found"),
                    tr("The dropped file does not exist:\n%1").arg(filePath));
            }
        }
    }
}

} // namespace VideoStudio
