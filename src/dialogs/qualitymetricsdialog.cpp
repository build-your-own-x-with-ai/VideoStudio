#include "qualitymetricsdialog.h"
#include "core/videodecoder.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QProcess>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <cmath>

extern "C" {
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

namespace VideoStudio {

QualityMetricsDialog::QualityMetricsDialog(const QString& referenceFile, QWidget* parent)
    : QDialog(parent)
    , m_referenceFile(referenceFile)
    , m_cancelled(false)
{
    setWindowTitle(tr("Video Quality Metrics"));
    resize(700, 600);
    setupUI();
}

QualityMetricsDialog::~QualityMetricsDialog() = default;

void QualityMetricsDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // File selection group
    QGroupBox* filesGroup = new QGroupBox(tr("Video Files"));
    QFormLayout* filesLayout = new QFormLayout(filesGroup);

    m_referenceEdit = new QLineEdit(m_referenceFile);
    m_referenceEdit->setReadOnly(true);
    filesLayout->addRow(tr("Reference:"), m_referenceEdit);

    QHBoxLayout* distortedLayout = new QHBoxLayout();
    m_distortedEdit = new QLineEdit();
    m_distortedEdit->setPlaceholderText(tr("Select distorted/compressed video file..."));
    m_browseButton = new QPushButton(tr("Browse..."));
    connect(m_browseButton, &QPushButton::clicked, this, &QualityMetricsDialog::browseDistortedFile);
    distortedLayout->addWidget(m_distortedEdit);
    distortedLayout->addWidget(m_browseButton);
    filesLayout->addRow(tr("Distorted:"), distortedLayout);

    mainLayout->addWidget(filesGroup);

    // Options group
    QGroupBox* optionsGroup = new QGroupBox(tr("Analysis Options"));
    QFormLayout* optionsLayout = new QFormLayout(optionsGroup);

    m_startFrameSpin = new QSpinBox();
    m_startFrameSpin->setMinimum(0);
    m_startFrameSpin->setMaximum(999999);
    m_startFrameSpin->setValue(0);
    optionsLayout->addRow(tr("Start Frame:"), m_startFrameSpin);

    m_endFrameSpin = new QSpinBox();
    m_endFrameSpin->setMinimum(0);
    m_endFrameSpin->setMaximum(999999);
    m_endFrameSpin->setValue(0);
    m_endFrameSpin->setSpecialValueText(tr("End of video"));
    optionsLayout->addRow(tr("End Frame:"), m_endFrameSpin);

    m_calculatePSNRCheck = new QCheckBox(tr("Calculate PSNR (Peak Signal-to-Noise Ratio)"));
    m_calculatePSNRCheck->setChecked(true);
    optionsLayout->addRow(m_calculatePSNRCheck);

    m_calculateSSIMCheck = new QCheckBox(tr("Calculate SSIM (Structural Similarity Index)"));
    m_calculateSSIMCheck->setChecked(true);
    optionsLayout->addRow(m_calculateSSIMCheck);

    m_calculateVMAFCheck = new QCheckBox(tr("Calculate VMAF (Video Multimethod Assessment Fusion)"));
    m_calculateVMAFCheck->setChecked(true);
    optionsLayout->addRow(m_calculateVMAFCheck);

    mainLayout->addWidget(optionsGroup);

    // Progress
    m_statusLabel = new QLabel(tr("Ready"));
    mainLayout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    // Results
    QGroupBox* resultsGroup = new QGroupBox(tr("Results"));
    QVBoxLayout* resultsLayout = new QVBoxLayout(resultsGroup);
    m_resultsText = new QTextEdit();
    m_resultsText->setReadOnly(true);
    m_resultsText->setFont(QFont("Courier", 10));
    resultsLayout->addWidget(m_resultsText);
    mainLayout->addWidget(resultsGroup);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_startButton = new QPushButton(tr("Start Analysis"));
    m_startButton->setEnabled(false);
    connect(m_startButton, &QPushButton::clicked, this, &QualityMetricsDialog::startAnalysis);
    buttonLayout->addWidget(m_startButton);

    m_cancelButton = new QPushButton(tr("Close"));
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void QualityMetricsDialog::browseDistortedFile() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Select Distorted Video File"),
        QString(),
        tr("Video Files (*.ts *.mp4 *.mkv *.avi *.flv *.mov *.m2ts *.mts);;All Files (*)")
    );

    if (!fileName.isEmpty()) {
        m_distortedFile = fileName;
        m_distortedEdit->setText(fileName);
        m_startButton->setEnabled(true);
    }
}

void QualityMetricsDialog::startAnalysis() {
    if (m_distortedFile.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a distorted video file."));
        return;
    }

    if (!m_calculatePSNRCheck->isChecked() && !m_calculateSSIMCheck->isChecked() && !m_calculateVMAFCheck->isChecked()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select at least one metric to calculate."));
        return;
    }

    m_cancelled = false;
    m_startButton->setEnabled(false);
    m_browseButton->setEnabled(false);
    m_progressBar->setVisible(true);
    m_resultsText->clear();
    m_cancelButton->setText(tr("Cancel"));

    calculateMetrics();

    m_startButton->setEnabled(true);
    m_browseButton->setEnabled(true);
    m_progressBar->setVisible(false);
    m_cancelButton->setText(tr("Close"));
}

void QualityMetricsDialog::cancelAnalysis() {
    m_cancelled = true;
}

void QualityMetricsDialog::calculateMetrics() {
    // Open reference video
    m_refDecoder = std::make_unique<VideoDecoder>();
    if (!m_refDecoder->openFile(m_referenceFile)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to open reference video file."));
        m_statusLabel->setText(tr("Error: Failed to open reference video"));
        return;
    }

    // Open distorted video
    m_distDecoder = std::make_unique<VideoDecoder>();
    if (!m_distDecoder->openFile(m_distortedFile)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to open distorted video file."));
        m_statusLabel->setText(tr("Error: Failed to open distorted video"));
        return;
    }

    // Check if videos have same resolution
    if (m_refDecoder->getWidth() != m_distDecoder->getWidth() ||
        m_refDecoder->getHeight() != m_distDecoder->getHeight()) {
        QMessageBox::critical(this, tr("Error"),
            tr("Videos must have the same resolution.\nReference: %1x%2\nDistorted: %3x%4")
                .arg(m_refDecoder->getWidth())
                .arg(m_refDecoder->getHeight())
                .arg(m_distDecoder->getWidth())
                .arg(m_distDecoder->getHeight()));
        m_statusLabel->setText(tr("Error: Resolution mismatch"));
        return;
    }

    int width = m_refDecoder->getWidth();
    int height = m_refDecoder->getHeight();
    int startFrame = m_startFrameSpin->value();
    int endFrame = m_endFrameSpin->value();

    if (endFrame == 0 || endFrame > m_refDecoder->getFrameCount()) {
        endFrame = m_refDecoder->getFrameCount();
    }
    if (endFrame > m_distDecoder->getFrameCount()) {
        endFrame = m_distDecoder->getFrameCount();
    }

    int totalFrames = endFrame - startFrame;
    if (totalFrames <= 0) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid frame range."));
        m_statusLabel->setText(tr("Error: Invalid frame range"));
        return;
    }

    m_progressBar->setMaximum(totalFrames);
    m_progressBar->setValue(0);

    double totalPSNR = 0.0;
    double totalSSIM = 0.0;
    int frameCount = 0;

    m_resultsText->append(tr("Starting quality analysis...\n"));
    m_resultsText->append(tr("Reference: %1").arg(m_referenceFile));
    m_resultsText->append(tr("Distorted: %1").arg(m_distortedFile));
    m_resultsText->append(tr("Resolution: %1x%2").arg(width).arg(height));
    m_resultsText->append(tr("Frame range: %1 - %2 (%3 frames)\n")
        .arg(startFrame).arg(endFrame).arg(totalFrames));

    // Seek to start frame
    if (!m_refDecoder->seekToFrame(startFrame) || !m_distDecoder->seekToFrame(startFrame)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to seek to start frame."));
        m_statusLabel->setText(tr("Error: Failed to seek"));
        return;
    }

    // Process frames
    for (int i = startFrame; i < endFrame && !m_cancelled; ++i) {
        AVFrame* refFrame = m_refDecoder->decodeNextFrame();
        AVFrame* distFrame = m_distDecoder->decodeNextFrame();

        if (!refFrame || !distFrame) {
            m_resultsText->append(tr("\nWarning: Reached end of video at frame %1").arg(i));
            break;
        }

        // Convert frames to grayscale for analysis
        // For simplicity, we'll use the Y plane (luma) from YUV
        const uint8_t* refData = refFrame->data[0];
        const uint8_t* distData = distFrame->data[0];

        if (m_calculatePSNRCheck->isChecked()) {
            double psnr = calculatePSNR(refData, distData, width, height);
            totalPSNR += psnr;
        }

        if (m_calculateSSIMCheck->isChecked()) {
            double ssim = calculateSSIM(refData, distData, width, height);
            totalSSIM += ssim;
        }

        frameCount++;
        m_progressBar->setValue(frameCount);
        m_statusLabel->setText(tr("Processing frame %1 of %2...").arg(frameCount).arg(totalFrames));
        QApplication::processEvents();
    }

    if (m_cancelled) {
        m_resultsText->append(tr("\n\nAnalysis cancelled by user."));
        m_statusLabel->setText(tr("Cancelled"));
        return;
    }

    // Display results
    m_resultsText->append(tr("\n=== Analysis Complete ===\n"));
    m_resultsText->append(tr("Frames analyzed: %1\n").arg(frameCount));

    if (m_calculatePSNRCheck->isChecked()) {
        double avgPSNR = totalPSNR / frameCount;
        m_resultsText->append(tr("Average PSNR: %1 dB").arg(avgPSNR, 0, 'f', 2));
        m_resultsText->append(tr("  (Higher is better, >40 dB is excellent, 30-40 dB is good)"));
    }

    if (m_calculateSSIMCheck->isChecked()) {
        double avgSSIM = totalSSIM / frameCount;
        m_resultsText->append(tr("\nAverage SSIM: %1").arg(avgSSIM, 0, 'f', 4));
        m_resultsText->append(tr("  (Range: 0-1, higher is better, >0.95 is excellent)"));
    }

    if (m_calculateVMAFCheck->isChecked()) {
        m_resultsText->append(tr("\nCalculating VMAF (this may take several minutes)..."));
        m_statusLabel->setText(tr("Calculating VMAF..."));
        QApplication::processEvents();

        double vmafScore = calculateVMAF(m_referenceFile, m_distortedFile, width, height, startFrame, endFrame);
        if (vmafScore >= 0) {
            m_resultsText->append(tr("VMAF Score: %1").arg(vmafScore, 0, 'f', 2));
            m_resultsText->append(tr("  (Range: 0-100, higher is better, >80 is excellent, 60-80 is good)"));
        } else {
            m_resultsText->append(tr("VMAF calculation failed. Check console for details."));
        }
    }

    m_statusLabel->setText(tr("Analysis complete"));
}

double QualityMetricsDialog::calculatePSNR(const uint8_t* ref, const uint8_t* dist, int width, int height) {
    double mse = 0.0;
    int totalPixels = width * height;

    for (int i = 0; i < totalPixels; ++i) {
        double diff = ref[i] - dist[i];
        mse += diff * diff;
    }

    mse /= totalPixels;

    if (mse == 0.0) {
        return 100.0; // Perfect match
    }

    double psnr = 10.0 * log10((255.0 * 255.0) / mse);
    return psnr;
}

double QualityMetricsDialog::calculateSSIM(const uint8_t* ref, const uint8_t* dist, int width, int height) {
    // Simplified SSIM calculation using 8x8 blocks
    const int blockSize = 8;
    const double C1 = 6.5025;  // (0.01 * 255)^2
    const double C2 = 58.5225; // (0.03 * 255)^2

    double totalSSIM = 0.0;
    int blockCount = 0;

    for (int y = 0; y <= height - blockSize; y += blockSize) {
        for (int x = 0; x <= width - blockSize; x += blockSize) {
            // Calculate mean and variance for this block
            double meanRef = 0.0, meanDist = 0.0;
            double varRef = 0.0, varDist = 0.0, covar = 0.0;

            // Calculate means
            for (int by = 0; by < blockSize; ++by) {
                for (int bx = 0; bx < blockSize; ++bx) {
                    int idx = (y + by) * width + (x + bx);
                    meanRef += ref[idx];
                    meanDist += dist[idx];
                }
            }
            meanRef /= (blockSize * blockSize);
            meanDist /= (blockSize * blockSize);

            // Calculate variances and covariance
            for (int by = 0; by < blockSize; ++by) {
                for (int bx = 0; bx < blockSize; ++bx) {
                    int idx = (y + by) * width + (x + bx);
                    double diffRef = ref[idx] - meanRef;
                    double diffDist = dist[idx] - meanDist;
                    varRef += diffRef * diffRef;
                    varDist += diffDist * diffDist;
                    covar += diffRef * diffDist;
                }
            }
            varRef /= (blockSize * blockSize);
            varDist /= (blockSize * blockSize);
            covar /= (blockSize * blockSize);

            // Calculate SSIM for this block
            double numerator = (2.0 * meanRef * meanDist + C1) * (2.0 * covar + C2);
            double denominator = (meanRef * meanRef + meanDist * meanDist + C1) *
                                 (varRef + varDist + C2);

            double ssim = numerator / denominator;
            totalSSIM += ssim;
            blockCount++;
        }
    }

    return totalSSIM / blockCount;
}

double QualityMetricsDialog::calculateVMAF(const QString& refFile, const QString& distFile,
                                            int width, int height, int startFrame, int endFrame) {
    Q_UNUSED(width);
    Q_UNUSED(height);
    Q_UNUSED(startFrame);
    Q_UNUSED(endFrame);

    // VMAF calculation using FFmpeg's libvmaf filter
    QString tempLogFile = QDir::temp().filePath("vmaf_log.json");

    // Remove old log file if exists
    if (QFile::exists(tempLogFile)) {
        QFile::remove(tempLogFile);
    }

    qDebug() << "VMAF log file path:" << tempLogFile;

    // Build FFmpeg command with VMAF filter
    // Use full path to ffmpeg since macOS app bundle doesn't have Homebrew in PATH
    QString ffmpegPath = "/opt/homebrew/bin/ffmpeg";

    // Fallback to system ffmpeg if Homebrew version not found
    if (!QFile::exists(ffmpegPath)) {
        ffmpegPath = "/usr/local/bin/ffmpeg";
    }

    QString ffmpegCmd = QString("\"%1\" -y -i \"%2\" -i \"%3\" -lavfi \"[0:v][1:v]libvmaf=log_path=%4:log_fmt=json\" -f null -")
        .arg(ffmpegPath)
        .arg(distFile)
        .arg(refFile)
        .arg(tempLogFile);

    qDebug() << "VMAF command:" << ffmpegCmd;

    // Execute FFmpeg command
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("sh", QStringList() << "-c" << ffmpegCmd);

    if (!process.waitForFinished(300000)) { // 5 minute timeout
        qDebug() << "VMAF calculation timeout";
        return -1.0;
    }

    // Read output
    QString output = QString::fromUtf8(process.readAll());

    qDebug() << "FFmpeg exit code:" << process.exitCode();
    if (process.exitCode() != 0) {
        qDebug() << "VMAF calculation failed with exit code:" << process.exitCode();
        qDebug() << "FFmpeg output:" << output.right(1000); // Last 1000 chars
        return -1.0;
    }

    // Check if log file was created
    if (!QFile::exists(tempLogFile)) {
        qDebug() << "VMAF log file was not created";
        return -1.0;
    }

    // Parse VMAF score from JSON log file
    QFile logFile(tempLogFile);
    if (!logFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open VMAF log file:" << tempLogFile;
        return -1.0;
    }

    QByteArray logData = logFile.readAll();
    logFile.close();

    qDebug() << "VMAF log file size:" << logData.size() << "bytes";

    // Parse JSON to extract VMAF score
    QJsonDocument jsonDoc = QJsonDocument::fromJson(logData);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qDebug() << "Failed to parse VMAF JSON";
        qDebug() << "Log content (first 500 bytes):" << QString::fromUtf8(logData.left(500));
        return -1.0;
    }

    QJsonObject rootObj = jsonDoc.object();

    if (rootObj.contains("pooled_metrics")) {
        QJsonObject pooledMetrics = rootObj["pooled_metrics"].toObject();

        if (pooledMetrics.contains("vmaf")) {
            QJsonObject vmafObj = pooledMetrics["vmaf"].toObject();

            if (vmafObj.contains("mean")) {
                double vmafScore = vmafObj["mean"].toDouble();
                qDebug() << "VMAF score extracted successfully:" << vmafScore;

                // Clean up temp file
                QFile::remove(tempLogFile);

                return vmafScore;
            }
        }
    }

    qDebug() << "Failed to extract VMAF score from JSON";
    qDebug() << "JSON root keys:" << rootObj.keys();
    return -1.0;
}

} // namespace VideoStudio
