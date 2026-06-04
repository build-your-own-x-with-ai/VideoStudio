#ifndef DUPLICATEFRAMEDETECTIONDIALOG_H
#define DUPLICATEFRAMEDETECTIONDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>
#include <QSet>
#include "core/duplicateframedetector.h"

namespace VideoStudio {

class VideoDecoder;

class DuplicateFrameDetectionDialog : public QDialog {
    Q_OBJECT

public:
    explicit DuplicateFrameDetectionDialog(VideoDecoder* decoder, QWidget* parent = nullptr);
    ~DuplicateFrameDetectionDialog();

signals:
    void seekToFrame(int frameNumber);
    void duplicateFramesDetected(const QSet<int>& duplicateFrames);

private slots:
    void startAnalysis();
    void cancelAnalysis();
    void onProgressUpdated(int current, int total, const QString& status);
    void onAnalysisCompleted(const DetectionResult& result);
    void onGroupSelected(int row, int column);
    void goToFrame();
    void exportReport();

private:
    void setupUI();
    void displayResults(const DetectionResult& result);
    QString formatFrameList(const QVector<int>& frames) const;

    VideoDecoder* m_decoder;
    DuplicateFrameDetector* m_detector;

    // UI components
    QLabel* m_videoInfoLabel;
    QPushButton* m_startButton;
    QPushButton* m_cancelButton;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;

    QLabel* m_totalFramesLabel;
    QLabel* m_uniqueFramesLabel;
    QLabel* m_duplicateFramesLabel;
    QLabel* m_duplicateGroupsLabel;
    QLabel* m_longestFreezeLabel;

    QTableWidget* m_groupsTable;

    QPushButton* m_goToFrameButton;
    QPushButton* m_exportButton;
    QPushButton* m_closeButton;

    DetectionResult m_currentResult;
    int m_selectedGroupIndex;
};

} // namespace VideoStudio

#endif // DUPLICATEFRAMEDETECTIONDIALOG_H
