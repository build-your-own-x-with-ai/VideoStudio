#ifndef REFERENCECOMPARISONDIALOG_H
#define REFERENCECOMPARISONDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace VideoStudio {

class VideoDecoder;

class ReferenceComparisonDialog : public QDialog {
    Q_OBJECT

public:
    enum class DifferenceMode {
        Compare,        // Side-by-side comparison
        PSNR,          // Color-coded PSNR per block
        Subtraction,   // Pixel-by-pixel difference
        Temperature    // Absolute difference with color gradient
    };

    explicit ReferenceComparisonDialog(VideoDecoder* decoder, QWidget* parent = nullptr);
    ~ReferenceComparisonDialog();

private slots:
    void selectReferenceFile();
    void updateComparison();
    void onModeChanged(int index);
    void onFrameChanged(int frame);

private:
    void setupUI();
    void loadReferenceVideo(const QString& filePath);
    void calculateDifference();
    void renderCompareMode();
    void renderPSNRMode();
    void renderSubtractionMode();
    void renderTemperatureMode();

    double calculateBlockPSNR(const uint8_t* src, const uint8_t* ref,
                               int width, int height, int srcStride, int refStride);
    QColor psnrToColor(double psnr);
    QColor temperatureToColor(int diff);

    VideoDecoder* m_decoder;
    std::unique_ptr<VideoDecoder> m_referenceDecoder;

    // Store current frames
    AVFrame* m_currentSourceFrame;
    AVFrame* m_currentRefFrame;

    QComboBox* m_modeCombo;
    QLabel* m_displayLabel;
    QSlider* m_frameSlider;
    QLabel* m_frameLabel;
    QPushButton* m_selectRefButton;
    QLabel* m_metricsLabel;

    DifferenceMode m_currentMode;
    int m_currentFrame;
    QImage m_resultImage;

    QString m_referenceFilePath;
};

} // namespace VideoStudio

#endif // REFERENCECOMPARISONDIALOG_H
