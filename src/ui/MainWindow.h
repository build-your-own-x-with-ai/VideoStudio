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
#include "StreamInfoPanel.h"
#include "FrameListView.h"

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

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void updateUI();

    VideoDecoder* decoder;
    MetricsCollector* metricsCollector;
    VideoAnalyzerThread* analyzerThread;
    StreamInfoPanel* streamInfoPanel;
    FrameListView* frameListView;
    QLabel* videoPreview;
    QTabWidget* tabWidget;
    QStatusBar* statusBar;
    QLabel* statusLabel;
    QProgressDialog* progressDialog;

    QAction* openAction;
    QAction* closeAction;
    QAction* exitAction;
    QAction* aboutAction;

    QString currentFilePath;
    QImage firstFrameImage;
};

#endif // MAINWINDOW_H
