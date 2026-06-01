#include "csvexportdialog.h"
#include "core/videodecoder.h"
#include "core/framedata.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QLabel>

namespace VideoStudio {

CSVExportDialog::CSVExportDialog(VideoDecoder* decoder, QWidget* parent)
    : QDialog(parent)
    , m_decoder(decoder)
{
    setWindowTitle(tr("导出 CSV 数据"));
    setMinimumWidth(500);
    setupUI();
}

void CSVExportDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Output file selection
    QGroupBox* fileGroup = new QGroupBox(tr("输出文件"));
    QHBoxLayout* fileLayout = new QHBoxLayout(fileGroup);

    m_outputPathEdit = new QLineEdit();
    m_outputPathEdit->setPlaceholderText(tr("选择输出文件路径..."));
    fileLayout->addWidget(m_outputPathEdit);

    m_browseButton = new QPushButton(tr("浏览..."));
    connect(m_browseButton, &QPushButton::clicked, this, &CSVExportDialog::browseOutputFile);
    fileLayout->addWidget(m_browseButton);

    mainLayout->addWidget(fileGroup);

    // Data selection
    QGroupBox* dataGroup = new QGroupBox(tr("导出数据"));
    QVBoxLayout* dataLayout = new QVBoxLayout(dataGroup);

    m_exportFrameListCheck = new QCheckBox(tr("帧列表（帧号、类型、大小、PTS、DTS、QP、比特率）"));
    m_exportFrameListCheck->setChecked(true);
    dataLayout->addWidget(m_exportFrameListCheck);

    m_exportStatisticsCheck = new QCheckBox(tr("统计信息（总帧数、I/P/B 帧数量、平均比特率）"));
    m_exportStatisticsCheck->setChecked(true);
    dataLayout->addWidget(m_exportStatisticsCheck);

    m_exportGOPStructureCheck = new QCheckBox(tr("GOP 结构（GOP 长度、关键帧间隔）"));
    m_exportGOPStructureCheck->setChecked(false);
    dataLayout->addWidget(m_exportGOPStructureCheck);

    m_exportBitrateCheck = new QCheckBox(tr("比特率数据（每帧瞬时比特率）"));
    m_exportBitrateCheck->setChecked(false);
    dataLayout->addWidget(m_exportBitrateCheck);

    mainLayout->addWidget(dataGroup);

    // Frame range
    QGroupBox* rangeGroup = new QGroupBox(tr("帧范围"));
    QFormLayout* rangeLayout = new QFormLayout(rangeGroup);

    m_startFrameSpin = new QSpinBox();
    m_startFrameSpin->setMinimum(0);
    m_startFrameSpin->setMaximum(m_decoder ? m_decoder->getFrameCount() - 1 : 0);
    m_startFrameSpin->setValue(0);
    rangeLayout->addRow(tr("起始帧:"), m_startFrameSpin);

    m_endFrameSpin = new QSpinBox();
    m_endFrameSpin->setMinimum(0);
    m_endFrameSpin->setMaximum(m_decoder ? m_decoder->getFrameCount() - 1 : 0);
    m_endFrameSpin->setValue(m_decoder ? m_decoder->getFrameCount() - 1 : 0);
    rangeLayout->addRow(tr("结束帧:"), m_endFrameSpin);

    mainLayout->addWidget(rangeGroup);

    // Format options
    QGroupBox* formatGroup = new QGroupBox(tr("格式选项"));
    QFormLayout* formatLayout = new QFormLayout(formatGroup);

    m_delimiterCombo = new QComboBox();
    m_delimiterCombo->addItem(tr("逗号 (,)"), ",");
    m_delimiterCombo->addItem(tr("分号 (;)"), ";");
    m_delimiterCombo->addItem(tr("制表符 (Tab)"), "\t");
    formatLayout->addRow(tr("分隔符:"), m_delimiterCombo);

    m_decimalSeparatorCombo = new QComboBox();
    m_decimalSeparatorCombo->addItem(tr("点 (.)"), ".");
    m_decimalSeparatorCombo->addItem(tr("逗号 (,)"), ",");
    formatLayout->addRow(tr("小数分隔符:"), m_decimalSeparatorCombo);

    mainLayout->addWidget(formatGroup);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_exportButton = new QPushButton(tr("导出"));
    m_exportButton->setDefault(true);
    connect(m_exportButton, &QPushButton::clicked, this, &CSVExportDialog::exportData);
    buttonLayout->addWidget(m_exportButton);

    m_cancelButton = new QPushButton(tr("取消"));
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void CSVExportDialog::browseOutputFile() {
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("保存 CSV 文件"),
        QString(),
        tr("CSV 文件 (*.csv);;所有文件 (*)")
    );

    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
            fileName += ".csv";
        }
        m_outputPathEdit->setText(fileName);
    }
}

void CSVExportDialog::exportData() {
    QString outputPath = m_outputPathEdit->text();
    if (outputPath.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("请选择输出文件路径。"));
        return;
    }

    if (!m_exportFrameListCheck->isChecked() &&
        !m_exportStatisticsCheck->isChecked() &&
        !m_exportGOPStructureCheck->isChecked() &&
        !m_exportBitrateCheck->isChecked()) {
        QMessageBox::warning(this, tr("错误"), tr("请至少选择一种数据类型导出。"));
        return;
    }

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("错误"), tr("无法创建输出文件: %1").arg(outputPath));
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    QString delimiter = m_delimiterCombo->currentData().toString();
    QString decimalSep = m_decimalSeparatorCombo->currentData().toString();

    int startFrame = m_startFrameSpin->value();
    int endFrame = m_endFrameSpin->value();

    if (startFrame > endFrame) {
        QMessageBox::warning(this, tr("错误"), tr("起始帧不能大于结束帧。"));
        return;
    }

    const FrameIndex& frameIndex = m_decoder->getFrameIndex();

    // Export statistics
    if (m_exportStatisticsCheck->isChecked()) {
        out << "=== " << tr("统计信息") << " ===" << "\n";
        out << tr("视频文件") << delimiter << m_decoder->getFileName() << "\n";
        out << tr("编解码器") << delimiter << m_decoder->getCodecName() << "\n";
        out << tr("分辨率") << delimiter << m_decoder->getWidth() << "x" << m_decoder->getHeight() << "\n";
        out << tr("帧率") << delimiter << QString::number(m_decoder->getFrameRate(), 'f', 2).replace(".", decimalSep) << " fps\n";
        out << tr("总帧数") << delimiter << frameIndex.frameCount() << "\n";
        out << tr("I 帧数量") << delimiter << frameIndex.getIFrameCount() << "\n";
        out << tr("P 帧数量") << delimiter << frameIndex.getPFrameCount() << "\n";
        out << tr("B 帧数量") << delimiter << frameIndex.getBFrameCount() << "\n";

        double avgBitrate = frameIndex.getAverageBitrate();
        out << tr("平均比特率") << delimiter << QString::number(avgBitrate / 1000.0, 'f', 2).replace(".", decimalSep) << " kbps\n";
        out << tr("最大帧大小") << delimiter << frameIndex.getMaxFrameSize() << " bytes\n";
        out << tr("最小帧大小") << delimiter << frameIndex.getMinFrameSize() << " bytes\n";
        out << "\n";
    }

    // Export frame list
    if (m_exportFrameListCheck->isChecked()) {
        out << "=== " << tr("帧列表") << " ===" << "\n";
        out << tr("帧号") << delimiter
            << tr("类型") << delimiter
            << tr("大小(bytes)") << delimiter
            << tr("PTS") << delimiter
            << tr("DTS") << delimiter
            << tr("偏移") << delimiter
            << tr("QP") << delimiter
            << tr("关键帧") << delimiter
            << tr("比特率(kbps)") << delimiter
            << tr("时间戳(s)") << "\n";

        for (int i = startFrame; i <= endFrame && i < frameIndex.frameCount(); ++i) {
            const FrameInfo* frame = frameIndex.getFrame(i);
            if (!frame) continue;

            out << frame->frameNumber << delimiter
                << getFrameTypeString(frame->frameType) << delimiter
                << frame->size << delimiter
                << frame->pts << delimiter
                << frame->dts << delimiter
                << frame->offset << delimiter
                << frame->qp << delimiter
                << (frame->isKeyFrame ? "Y" : "N") << delimiter
                << QString::number(frame->bitrate / 1000.0, 'f', 2).replace(".", decimalSep) << delimiter
                << QString::number(frame->timestamp, 'f', 3).replace(".", decimalSep) << "\n";
        }
        out << "\n";
    }

    // Export GOP structure
    if (m_exportGOPStructureCheck->isChecked()) {
        out << "=== " << tr("GOP 结构") << " ===" << "\n";
        out << tr("GOP 编号") << delimiter
            << tr("起始帧") << delimiter
            << tr("结束帧") << delimiter
            << tr("长度") << delimiter
            << tr("I 帧数") << delimiter
            << tr("P 帧数") << delimiter
            << tr("B 帧数") << "\n";

        int gopNumber = 0;
        int gopStart = -1;
        int gopIFrames = 0, gopPFrames = 0, gopBFrames = 0;

        for (int i = startFrame; i <= endFrame && i < frameIndex.frameCount(); ++i) {
            const FrameInfo* frame = frameIndex.getFrame(i);
            if (!frame) continue;

            if (frame->isKeyFrame) {
                // Output previous GOP
                if (gopStart >= 0) {
                    int gopLength = i - gopStart;
                    out << gopNumber << delimiter
                        << gopStart << delimiter
                        << (i - 1) << delimiter
                        << gopLength << delimiter
                        << gopIFrames << delimiter
                        << gopPFrames << delimiter
                        << gopBFrames << "\n";
                    gopNumber++;
                }

                // Start new GOP
                gopStart = i;
                gopIFrames = 0;
                gopPFrames = 0;
                gopBFrames = 0;
            }

            // Count frame types
            if (frame->frameType == AV_PICTURE_TYPE_I) gopIFrames++;
            else if (frame->frameType == AV_PICTURE_TYPE_P) gopPFrames++;
            else if (frame->frameType == AV_PICTURE_TYPE_B) gopBFrames++;
        }

        // Output last GOP
        if (gopStart >= 0) {
            int gopLength = endFrame - gopStart + 1;
            out << gopNumber << delimiter
                << gopStart << delimiter
                << endFrame << delimiter
                << gopLength << delimiter
                << gopIFrames << delimiter
                << gopPFrames << delimiter
                << gopBFrames << "\n";
        }
        out << "\n";
    }

    // Export bitrate data
    if (m_exportBitrateCheck->isChecked()) {
        out << "=== " << tr("比特率数据") << " ===" << "\n";
        out << tr("帧号") << delimiter
            << tr("时间戳(s)") << delimiter
            << tr("瞬时比特率(kbps)") << "\n";

        for (int i = startFrame; i <= endFrame && i < frameIndex.frameCount(); ++i) {
            const FrameInfo* frame = frameIndex.getFrame(i);
            if (!frame) continue;

            out << frame->frameNumber << delimiter
                << QString::number(frame->timestamp, 'f', 3).replace(".", decimalSep) << delimiter
                << QString::number(frame->bitrate / 1000.0, 'f', 2).replace(".", decimalSep) << "\n";
        }
        out << "\n";
    }

    file.close();

    QMessageBox::information(this, tr("成功"),
        tr("CSV 数据已成功导出到:\n%1").arg(outputPath));
    accept();
}

QString CSVExportDialog::getFrameTypeString(int frameType) const {
    switch (frameType) {
        case AV_PICTURE_TYPE_I: return "I";
        case AV_PICTURE_TYPE_P: return "P";
        case AV_PICTURE_TYPE_B: return "B";
        case AV_PICTURE_TYPE_S: return "S";
        case AV_PICTURE_TYPE_SI: return "SI";
        case AV_PICTURE_TYPE_SP: return "SP";
        case AV_PICTURE_TYPE_BI: return "BI";
        default: return "?";
    }
}

} // namespace VideoStudio
