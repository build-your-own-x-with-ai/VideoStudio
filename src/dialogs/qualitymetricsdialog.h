#ifndef QUALITYMETRICSDIALOG_H
#define QUALITYMETRICSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <memory>

namespace VideoStudio {

class VideoDecoder;

class QualityMetricsDialog : public QDialog {
    Q_OBJECT

public:
    explicit QualityMetricsDialog(const QString& referenceFile, QWidget* parent = nullptr);
    ~QualityMetricsDialog();

private slots:
    void browseDistortedFile();
    void startAnalysis();
    void cancelAnalysis();

private:
    void setupUI();
    void calculateMetrics();
    double calculatePSNR(const uint8_t* ref, const uint8_t* dist, int width, int height);
    double calculateSSIM(const uint8_t* ref, const uint8_t* dist, int width, int height);
    double calculateVMAF(const QString& refFile, const QString& distFile, int width, int height, int startFrame, int endFrame);

    QString m_referenceFile;
    QString m_distortedFile;

    QLineEdit* m_referenceEdit;
    QLineEdit* m_distortedEdit;
    QPushButton* m_browseButton;
    QPushButton* m_startButton;
    QPushButton* m_cancelButton;
    QProgressBar* m_progressBar;
    QTextEdit* m_resultsText;
    QSpinBox* m_startFrameSpin;
    QSpinBox* m_endFrameSpin;
    QCheckBox* m_calculatePSNRCheck;
    QCheckBox* m_calculateSSIMCheck;
    QCheckBox* m_calculateVMAFCheck;
    QLabel* m_statusLabel;

    std::unique_ptr<VideoDecoder> m_refDecoder;
    std::unique_ptr<VideoDecoder> m_distDecoder;

    bool m_cancelled;
};

} // namespace VideoStudio

#endif // QUALITYMETRICSDIALOG_H
