#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QDockWidget>
#include <QLabel>
#include <QScrollArea>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <memory>

class QProgressBar;

namespace VideoStudio {

class VideoDecoder;
class VideoOutput;
class BarChart;
class AreaChart;
class ThumbnailBar;
class StreamPanel;
class OverlayPanel;
class GOPViewer;
class ExplorerPanel;
class PropertyPanel;
class HexViewerPanel;
class PacketView;
class NALUnitView;
class MessagesPanel;
class TR101290Panel;
class TimeDynamicsPanel;
class BitratePanel;
class BufferPanel;
class GraphicsPanel;
class CommentsPanel;
class EPGPanel;
class BlockStatsPanel;
class LogViewer;
class TSParser;
class MP4Parser;
class MKVParser;
class AVIParser;
class FLVParser;
class NALUnitParser;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void openFile();
    void openRecentFile();
    void clearRecentFiles();
    void play();
    void pause();
    void stepForward();
    void stepBackward();
    void onFrameClicked(int frameNumber);
    void onPlaybackTimer();
    void toggleStreamPanel();
    void toggleExplorerPanel();
    void toggleBarChart();
    void toggleAreaChart();
    void toggleThumbnailBar();
    void toggleOverlayPanel();
    void toggleGOPViewer();
    void togglePropertyPanel();
    void toggleHexViewer();
    void toggleMessages();
    void toggleTR101290();
    void toggleTimeDynamics();
    void toggleBitrate();
    void toggleBuffer();
    void toggleGraphics();
    void toggleComments();
    void toggleEPG();
    void toggleBlockStats();
    void toggleLogViewer();
    void toggleMotionVectors();
    void togglePartitions();
    void toggleFrameTypes();
    void toggleCursorMode();
    void toggleStandardGridMode();
    void saveStreamInfo();
    void exportFrameAsYUV();
    void exportFrameRangeAsYUV();
    void exportCSVMetrics();
    void showQualityMetrics();
    void showReferenceComparison();
    void showYUVViewer();
    void captureScreenshot();
    void showAboutDialog();
    void showDuplicateFrameDetection();

public slots:
    void appendLog(const QString& message, QtMsgType type);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void animateLogViewerToDock();
    void createActions();
    void createMenus();
    void createToolbar();
    void createWidgets();
    void createDockWidgets();
    void updateUI();
    void updateFrameLabel(int currentFrame, int totalFrames);
    void updateTSPanels();  // Update TS analysis panels after parsing
    void updateMP4Panels(); // Update MP4 analysis panels after parsing
    void updateMKVPanels(); // Update MKV analysis panels after parsing
    void updateAVIPanels(); // Update AVI analysis panels after parsing
    void updateFLVPanels(); // Update FLV analysis panels after parsing
    void loadFile(const QString& filePath);
    void updateRecentFilesMenu();
    void addToRecentFiles(const QString& filePath);

    std::unique_ptr<VideoDecoder> m_decoder;
    std::unique_ptr<TSParser> m_tsParser;
    std::unique_ptr<MP4Parser> m_mp4Parser;
    std::unique_ptr<MKVParser> m_mkvParser;
    std::unique_ptr<AVIParser> m_aviParser;
    std::unique_ptr<FLVParser> m_flvParser;
    std::unique_ptr<NALUnitParser> m_nalUnitParser;
    QTabWidget* m_centralTabs;
    VideoOutput* m_videoOutput;
    BarChart* m_barChart;
    AreaChart* m_areaChart;
    ThumbnailBar* m_thumbnailBar;
    QScrollArea* m_thumbnailScrollArea;
    StreamPanel* m_streamPanel;
    OverlayPanel* m_overlayPanel;
    GOPViewer* m_gopViewer;
    ExplorerPanel* m_explorerPanel;
    PropertyPanel* m_propertyPanel;
    HexViewerPanel* m_hexViewerPanel;
    PacketView* m_packetView;
    NALUnitView* m_nalUnitView;
    MessagesPanel* m_messagesPanel;
    TR101290Panel* m_tr101290Panel;
    TimeDynamicsPanel* m_timeDynamicsPanel;
    BitratePanel* m_bitratePanel;
    BufferPanel* m_bufferPanel;
    GraphicsPanel* m_graphicsPanel;
    CommentsPanel* m_commentsPanel;
    EPGPanel* m_epgPanel;
    BlockStatsPanel* m_blockStatsPanel;
    LogViewer* m_logViewer;

    QDockWidget* m_barChartDock;
    QDockWidget* m_areaChartDock;
    QDockWidget* m_thumbnailDock;
    QDockWidget* m_streamPanelDock;
    QDockWidget* m_overlayPanelDock;
    QDockWidget* m_gopViewerDock;
    QDockWidget* m_explorerPanelDock;
    QDockWidget* m_propertyPanelDock;
    QDockWidget* m_hexViewerPanelDock;
    QDockWidget* m_messagesPanelDock;
    QDockWidget* m_tr101290PanelDock;
    QDockWidget* m_timeDynamicsPanelDock;
    QDockWidget* m_bitratePanelDock;
    QDockWidget* m_bufferPanelDock;
    QDockWidget* m_graphicsPanelDock;
    QDockWidget* m_commentsPanelDock;
    QDockWidget* m_epgPanelDock;
    QDockWidget* m_blockStatsPanelDock;
    QDockWidget* m_logViewerDock;

    QTimer* m_playbackTimer;
    bool m_isPlaying;

    // Audio playback
    QMediaPlayer* m_mediaPlayer;
    QAudioOutput* m_audioOutput;

    QProgressBar* m_progressBar;  // Progress bar for loading operations
    QLabel* m_statusLabel;        // Status label for messages

    QAction* m_overlayMotionVectorsAction;
    QAction* m_overlayPartitionsAction;
    QAction* m_overlayFrameTypesAction;
    QAction* m_cursorModeAction;
    QAction* m_standardGridModeAction;

    // View menu actions
    QAction* m_toggleStreamAction;
    QAction* m_toggleExplorerAction;
    QAction* m_toggleBarChartAction;
    QAction* m_toggleAreaChartAction;
    QAction* m_toggleThumbnailAction;
    QAction* m_toggleOverlayPanelAction;
    QAction* m_toggleGOPViewerAction;
    QAction* m_togglePropertyPanelAction;
    QAction* m_toggleHexViewerAction;
    QAction* m_toggleMessagesAction;
    QAction* m_toggleTR101290Action;
    QAction* m_toggleTimeDynamicsAction;
    QAction* m_toggleBitrateAction;
    QAction* m_toggleBufferAction;
    QAction* m_toggleGraphicsAction;
    QAction* m_toggleCommentsAction;
    QAction* m_toggleEPGAction;
    QAction* m_toggleBlockStatsAction;
    QAction* m_toggleLogViewerAction;

    QString m_currentFilePath;
    QLabel* m_filePathLabel;

    QMenu* m_recentFilesMenu;
    QList<QAction*> m_recentFileActions;
    static const int MaxRecentFiles = 10;

    static MainWindow* s_instance;  // Singleton instance for message handler
};

} // namespace VideoStudio

#endif // MAINWINDOW_H
