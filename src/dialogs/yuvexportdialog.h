#ifndef YUVEXPORTDIALOG_H
#define YUVEXPORTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>

namespace VideoStudio {

class VideoDecoder;

class YUVExportDialog : public QDialog {
    Q_OBJECT

public:
    enum class YUVFormat {
        I420,      // Planar 4:2:0
        I422,      // Planar 4:2:2
        I444,      // Planar 4:4:4
        NV12,      // Semi-planar 4:2:0 (Y plane + interleaved UV)
        NV21,      // Semi-planar 4:2:0 (Y plane + interleaved VU)
        YUY2,      // Packed 4:2:2
        UYVY,      // Packed 4:2:2
        RGB24,     // RGB 24-bit
        RGB32,     // RGB 32-bit (RGBA)
        GRAY       // Grayscale (Y only)
    };

    explicit YUVExportDialog(VideoDecoder* decoder, QWidget* parent = nullptr);
    ~YUVExportDialog();

private slots:
    void selectOutputPath();
    void exportFrames();

private:
    void setupUI();
    QString getFormatDescription(YUVFormat format) const;
    QString getFormatExtension(YUVFormat format) const;

    VideoDecoder* m_decoder;

    QComboBox* m_formatCombo;
    QSpinBox* m_startFrameSpinBox;
    QSpinBox* m_endFrameSpinBox;
    QLineEdit* m_outputPathEdit;
    QPushButton* m_browseButton;
    QPushButton* m_exportButton;
    QPushButton* m_cancelButton;
    QCheckBox* m_singleFileCheckBox;
};

} // namespace VideoStudio

#endif // YUVEXPORTDIALOG_H
