#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTabWidget>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include "core/VideoDecoder.h"
#include "core/MetricsCollector.h"
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

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void updateUI();

    VideoDecoder* decoder;
    MetricsCollector* metricsCollector;
    StreamInfoPanel* streamInfoPanel;
    FrameListView* frameListView;
    QLabel* videoPreview;
    QTabWidget* tabWidget;
    QStatusBar* statusBar;
    QLabel* statusLabel;

    QAction* openAction;
    QAction* closeAction;
    QAction* exitAction;
    QAction* aboutAction;

    QString currentFilePath;
};

#endif // MAINWINDOW_H
