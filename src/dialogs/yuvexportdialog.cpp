#include "dialogs/yuvexportdialog.h"
#include "core/videodecoder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QDir>

extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace VideoStudio {

YUVExportDialog::YUVExportDialog(VideoDecoder* decoder, QWidget* parent)
    : QDialog(parent)
    , m_decoder(decoder)
{
    setupUI();
    setWindowTitle(tr("Export YUV Frames"));
    resize(500, 300);
}

YUVExportDialog::~YUVExportDialog() = default;

void YUVExportDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Format selection
    QGroupBox* formatGroup = new QGroupBox(tr("Output Format"), this);
    QFormLayout* formatLayout = new QFormLayout(formatGroup);

    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem(tr("I420 (Planar 4:2:0)"), static_cast<int>(YUVFormat::I420));
    m_formatCombo->addItem(tr("I422 (Planar 4:2:2)"), static_cast<int>(YUVFormat::I422));
    m_formatCombo->addItem(tr("I444 (Planar 4:4:4)"), static_cast<int>(YUVFormat::I444));
    m_formatCombo->addItem(tr("NV12 (Semi-planar 4:2:0)"), static_cast<int>(YUVFormat::NV12));
    m_formatCombo->addItem(tr("NV21 (Semi-planar 4:2:0 VU)"), static_cast<int>(YUVFormat::NV21));
    m_formatCombo->addItem(tr("YUY2 (Packed 4:2:2)"), static_cast<int>(YUVFormat::YUY2));
    m_formatCombo->addItem(tr("UYVY (Packed 4:2:2)"), static_cast<int>(YUVFormat::UYVY));
    m_formatCombo->addItem(tr("RGB24 (24-bit RGB)"), static_cast<int>(YUVFormat::RGB24));
    m_formatCombo->addItem(tr("RGB32 (32-bit RGBA)"), static_cast<int>(YUVFormat::RGB32));
    m_formatCombo->addItem(tr("GRAY (Grayscale Y only)"), static_cast<int>(YUVFormat::GRAY));
    formatLayout->addRow(tr("Format:"), m_formatCombo);

    mainLayout->addWidget(formatGroup);

    // Frame range selection
    QGroupBox* rangeGroup = new QGroupBox(tr("Frame Range"), this);
    QFormLayout* rangeLayout = new QFormLayout(rangeGroup);

    m_startFrameSpinBox = new QSpinBox(this);
    m_startFrameSpinBox->setRange(0, m_decoder ? m_decoder->getFrameCount() - 1 : 0);
    m_startFrameSpinBox->setValue(m_decoder ? m_decoder->getCurrentFrameNumber() : 0);
    rangeLayout->addRow(tr("Start Frame:"), m_startFrameSpinBox);

    m_endFrameSpinBox = new QSpinBox(this);
    m_endFrameSpinBox->setRange(0, m_decoder ? m_decoder->getFrameCount() - 1 : 0);
    m_endFrameSpinBox->setValue(m_decoder ? m_decoder->getCurrentFrameNumber() : 0);
    rangeLayout->addRow(tr("End Frame:"), m_endFrameSpinBox);

    mainLayout->addWidget(rangeGroup);

    // Output options
    QGroupBox* outputGroup = new QGroupBox(tr("Output"), this);
    QVBoxLayout* outputLayout = new QVBoxLayout(outputGroup);

    m_singleFileCheckBox = new QCheckBox(tr("Export as single file (concatenated frames)"), this);
    m_singleFileCheckBox->setChecked(false);
    outputLayout->addWidget(m_singleFileCheckBox);

    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(new QLabel(tr("Output Path:"), this));
    m_outputPathEdit = new QLineEdit(this);
    pathLayout->addWidget(m_outputPathEdit);
    m_browseButton = new QPushButton(tr("Browse..."), this);
    connect(m_browseButton, &QPushButton::clicked, this, &YUVExportDialog::selectOutputPath);
    pathLayout->addWidget(m_browseButton);
    outputLayout->addLayout(pathLayout);

    mainLayout->addWidget(outputGroup);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_exportButton = new QPushButton(tr("Export"), this);
    connect(m_exportButton, &QPushButton::clicked, this, &YUVExportDialog::exportFrames);
    buttonLayout->addWidget(m_exportButton);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);
}

void YUVExportDialog::selectOutputPath() {
    QString path;
    if (m_singleFileCheckBox->isChecked()) {
        // Select file
        YUVFormat format = static_cast<YUVFormat>(m_formatCombo->currentData().toInt());
        QString ext = getFormatExtension(format);
        path = QFileDialog::getSaveFileName(this, tr("Select Output File"),
            QString("output.%1").arg(ext),
            tr("%1 Files (*.%2);;All Files (*)").arg(ext.toUpper()).arg(ext));
    } else {
        // Select directory
        path = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"));
    }

    if (!path.isEmpty()) {
        m_outputPathEdit->setText(path);
    }
}

void YUVExportDialog::exportFrames() {
    if (!m_decoder || !m_decoder->isOpen()) {
        QMessageBox::warning(this, tr("Error"), tr("No video loaded"));
        return;
    }

    QString outputPath = m_outputPathEdit->text();
    if (outputPath.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select an output path"));
        return;
    }

    int startFrame = m_startFrameSpinBox->value();
    int endFrame = m_endFrameSpinBox->value();

    if (startFrame > endFrame) {
        QMessageBox::warning(this, tr("Error"), tr("Start frame must be <= end frame"));
        return;
    }

    YUVFormat format = static_cast<YUVFormat>(m_formatCombo->currentData().toInt());
    bool singleFile = m_singleFileCheckBox->isChecked();

    // Map format to AVPixelFormat
    AVPixelFormat avFormat;
    switch (format) {
        case YUVFormat::I420: avFormat = AV_PIX_FMT_YUV420P; break;
        case YUVFormat::I422: avFormat = AV_PIX_FMT_YUV422P; break;
        case YUVFormat::I444: avFormat = AV_PIX_FMT_YUV444P; break;
        case YUVFormat::NV12: avFormat = AV_PIX_FMT_NV12; break;
        case YUVFormat::NV21: avFormat = AV_PIX_FMT_NV21; break;
        case YUVFormat::YUY2: avFormat = AV_PIX_FMT_YUYV422; break;
        case YUVFormat::UYVY: avFormat = AV_PIX_FMT_UYVY422; break;
        case YUVFormat::RGB24: avFormat = AV_PIX_FMT_RGB24; break;
        case YUVFormat::RGB32: avFormat = AV_PIX_FMT_RGBA; break;
        case YUVFormat::GRAY: avFormat = AV_PIX_FMT_GRAY8; break;
        default: avFormat = AV_PIX_FMT_YUV420P;
    }

    // Save current position
    int originalFrame = m_decoder->getCurrentFrameNumber();

    // Create progress dialog
    QProgressDialog progress(tr("Exporting frames..."), tr("Cancel"), startFrame, endFrame, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);

    QFile outputFile;
    if (singleFile) {
        outputFile.setFileName(outputPath);
        if (!outputFile.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to create output file"));
            return;
        }
    }

    bool success = true;
    for (int frameNum = startFrame; frameNum <= endFrame; frameNum++) {
        if (progress.wasCanceled()) {
            success = false;
            break;
        }

        progress.setValue(frameNum);

        // Seek and decode frame
        if (!m_decoder->seekToFrame(frameNum)) {
            success = false;
            break;
        }

        AVFrame* frame = m_decoder->decodeNextFrame();
        if (!frame) {
            success = false;
            break;
        }

        // Convert frame to target format
        SwsContext* swsCtx = sws_getContext(
            frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
            frame->width, frame->height, avFormat,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );

        if (!swsCtx) {
            success = false;
            break;
        }

        AVFrame* convertedFrame = av_frame_alloc();
        convertedFrame->format = avFormat;
        convertedFrame->width = frame->width;
        convertedFrame->height = frame->height;
        av_frame_get_buffer(convertedFrame, 0);

        sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height,
                  convertedFrame->data, convertedFrame->linesize);

        sws_freeContext(swsCtx);

        // Write frame data
        if (singleFile) {
            // Write to single file
            for (int plane = 0; plane < 4; plane++) {
                if (!convertedFrame->data[plane]) break;
                int planeSize = av_image_get_linesize(avFormat, convertedFrame->width, plane) *
                               (convertedFrame->height >> (plane > 0 ? 1 : 0));
                if (planeSize > 0) {
                    outputFile.write(reinterpret_cast<const char*>(convertedFrame->data[plane]), planeSize);
                }
            }
        } else {
            // Write individual files
            QString ext = getFormatExtension(format);
            QString filename = QDir(outputPath).filePath(
                QString("frame_%1.%2").arg(frameNum, 6, 10, QChar('0')).arg(ext)
            );
            QFile file(filename);
            if (file.open(QIODevice::WriteOnly)) {
                for (int plane = 0; plane < 4; plane++) {
                    if (!convertedFrame->data[plane]) break;
                    int planeSize = av_image_get_linesize(avFormat, convertedFrame->width, plane) *
                                   (convertedFrame->height >> (plane > 0 ? 1 : 0));
                    if (planeSize > 0) {
                        file.write(reinterpret_cast<const char*>(convertedFrame->data[plane]), planeSize);
                    }
                }
                file.close();
            } else {
                success = false;
            }
        }

        av_frame_free(&convertedFrame);

        if (!success) break;
    }

    if (singleFile) {
        outputFile.close();
    }

    progress.setValue(endFrame);

    // Restore original position
    m_decoder->seekToFrame(originalFrame);

    if (success) {
        QMessageBox::information(this, tr("Success"),
            tr("Exported %1 frames successfully").arg(endFrame - startFrame + 1));
        accept();
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Export failed"));
    }
}

QString YUVExportDialog::getFormatExtension(YUVFormat format) const {
    switch (format) {
        case YUVFormat::I420: return "yuv";
        case YUVFormat::I422: return "yuv";
        case YUVFormat::I444: return "yuv";
        case YUVFormat::NV12: return "nv12";
        case YUVFormat::NV21: return "nv21";
        case YUVFormat::YUY2: return "yuy2";
        case YUVFormat::UYVY: return "uyvy";
        case YUVFormat::RGB24: return "rgb";
        case YUVFormat::RGB32: return "rgba";
        case YUVFormat::GRAY: return "y";
        default: return "yuv";
    }
}

} // namespace VideoStudio
