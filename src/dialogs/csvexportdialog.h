#ifndef CSVEXPORTDIALOG_H
#define CSVEXPORTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QSpinBox>

namespace VideoStudio {

class VideoDecoder;
class FrameIndex;

class CSVExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit CSVExportDialog(VideoDecoder* decoder, QWidget* parent = nullptr);

private slots:
    void browseOutputFile();
    void exportData();

private:
    void setupUI();
    QString getFrameTypeString(int frameType) const;

    VideoDecoder* m_decoder;

    // UI elements
    QLineEdit* m_outputPathEdit;
    QPushButton* m_browseButton;

    // Data selection
    QCheckBox* m_exportFrameListCheck;
    QCheckBox* m_exportStatisticsCheck;
    QCheckBox* m_exportGOPStructureCheck;
    QCheckBox* m_exportBitrateCheck;

    // Frame range
    QSpinBox* m_startFrameSpin;
    QSpinBox* m_endFrameSpin;

    // Format options
    QComboBox* m_delimiterCombo;
    QComboBox* m_decimalSeparatorCombo;

    QPushButton* m_exportButton;
    QPushButton* m_cancelButton;
};

} // namespace VideoStudio

#endif // CSVEXPORTDIALOG_H
