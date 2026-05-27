#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTabWidget>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QProgressDialog>
#include "core/VideoDecoder.h"
#include "core/MetricsCollector.h"
#include "core/VideoAnalyzerThread.h"
#include "core/BitrateAnalyzer.h"
#include "core/GOPAnalyzer.h"
#include "core/Exporter.h"
#include "StreamInfoPanel.h"
#include "FrameListView.h"
#include "BitrateChart.h"
#include "GOPViewer.h"
#include "VideoPlayer.h"
#include "FrameSizeChart.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void openFile();
    void closeFile();
    void about();
    void onFrameSelected(int frameIndex);
    void onAnalysisProgress(int current, int total);
    void onAnalysisComplete();
    void onAnalysisFailed(const QString& error);
    void exportHTMLReport();
    void exportFrameListCSV();
    void exportBitrateCSV();
    void exportGOPCSV();
    void saveScreenshot();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void updateUI();

    VideoDecoder* decoder;
    MetricsCollector* metricsCollector;
    VideoAnalyzerThread* analyzerThread;
    BitrateAnalyzer* bitrateAnalyzer;
    GOPAnalyzer* gopAnalyzer;
    StreamInfoPanel* streamInfoPanel;
    FrameListView* frameListView;
    BitrateChart* bitrateChart;
    GOPViewer* gopViewer;
    VideoPlayer* videoPlayer;
    FrameSizeChart* frameSizeChart;
    QTabWidget* tabWidget;
    QStatusBar* statusBar;
    QLabel* statusLabel;
    QProgressDialog* progressDialog;

    QAction* openAction;
    QAction* closeAction;
    QAction* exitAction;
    QAction* aboutAction;
    QAction* exportHTMLAction;
    QAction* exportFrameListAction;
    QAction* exportBitrateAction;
    QAction* exportGOPAction;
    QAction* saveScreenshotAction;

    QString currentFilePath;
};

#endif // MAINWINDOW_H
