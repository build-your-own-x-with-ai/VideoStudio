#include "dialogs/referencecomparisondialog.h"
#include "core/videodecoder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QScrollArea>
#include <cmath>

namespace VideoStudio {

ReferenceComparisonDialog::ReferenceComparisonDialog(VideoDecoder* decoder, QWidget* parent)
    : QDialog(parent)
    , m_decoder(decoder)
    , m_currentMode(DifferenceMode::Compare)
    , m_currentFrame(0)
    , m_currentSourceFrame(nullptr)
    , m_currentRefFrame(nullptr)
{
    setupUI();
    setWindowTitle(tr("Reference Stream Comparison"));
    resize(1200, 800);
}

ReferenceComparisonDialog::~ReferenceComparisonDialog() = default;

void ReferenceComparisonDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Top controls
    QHBoxLayout* controlLayout = new QHBoxLayout();

    m_selectRefButton = new QPushButton(tr("Select Reference Video..."), this);
    connect(m_selectRefButton, &QPushButton::clicked, this, &ReferenceComparisonDialog::selectReferenceFile);
    controlLayout->addWidget(m_selectRefButton);

    controlLayout->addWidget(new QLabel(tr("Mode:"), this));
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem(tr("Compare (Side-by-Side)"));
    m_modeCombo->addItem(tr("PSNR (Block Visualization)"));
    m_modeCombo->addItem(tr("Subtraction"));
    m_modeCombo->addItem(tr("Temperature Map"));
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReferenceComparisonDialog::onModeChanged);
    controlLayout->addWidget(m_modeCombo);

    controlLayout->addStretch();
    mainLayout->addLayout(controlLayout);

    // Frame slider
    QHBoxLayout* sliderLayout = new QHBoxLayout();
    sliderLayout->addWidget(new QLabel(tr("Frame:"), this));

    m_frameSlider = new QSlider(Qt::Horizontal, this);
    m_frameSlider->setMinimum(0);
    if (m_decoder && m_decoder->getFrameCount() > 0) {
        m_frameSlider->setMaximum(m_decoder->getFrameCount() - 1);
    }
    connect(m_frameSlider, &QSlider::valueChanged, this, &ReferenceComparisonDialog::onFrameChanged);
    sliderLayout->addWidget(m_frameSlider);

    m_frameLabel = new QLabel(tr("0 / 0"), this);
    sliderLayout->addWidget(m_frameLabel);
    mainLayout->addLayout(sliderLayout);

    // Metrics display
    m_metricsLabel = new QLabel(this);
    m_metricsLabel->setStyleSheet("QLabel { background-color: #2b2b2b; color: #ffffff; padding: 10px; border: 1px solid #555; }");
    m_metricsLabel->setText(tr("No reference video loaded"));
    mainLayout->addWidget(m_metricsLabel);

    // Display area with scroll
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setAlignment(Qt::AlignCenter);

    m_displayLabel = new QLabel(this);
    m_displayLabel->setAlignment(Qt::AlignCenter);
    m_displayLabel->setScaledContents(false);
    m_displayLabel->setMinimumSize(640, 360);
    scrollArea->setWidget(m_displayLabel);
    mainLayout->addWidget(scrollArea);

    // Close button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton* closeButton = new QPushButton(tr("Close"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);
}

void ReferenceComparisonDialog::selectReferenceFile() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Select Reference Video"),
        QString(),
        tr("Video Files (*.mp4 *.mkv *.avi *.mov *.ts *.h264 *.h265 *.hevc);;All Files (*)")
    );

    if (!fileName.isEmpty()) {
        loadReferenceVideo(fileName);
    }
}

void ReferenceComparisonDialog::loadReferenceVideo(const QString& filePath) {
    m_referenceDecoder = std::make_unique<VideoDecoder>();

    if (!m_referenceDecoder->openFile(filePath)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to open reference video file"));
        m_referenceDecoder.reset();
        return;
    }

    m_referenceFilePath = filePath;

    // Check if frame counts match
    int sourceFrames = m_decoder ? m_decoder->getFrameCount() : 0;
    int refFrames = m_referenceDecoder->getFrameCount();

    if (sourceFrames != refFrames) {
        QMessageBox::warning(this, tr("Warning"),
            tr("Frame count mismatch:\nSource: %1 frames\nReference: %2 frames\n\nComparison will use minimum frame count.")
            .arg(sourceFrames).arg(refFrames));
    }

    // Update slider range
    int maxFrames = std::min(sourceFrames, refFrames);
    m_frameSlider->setMaximum(maxFrames > 0 ? maxFrames - 1 : 0);
    m_frameLabel->setText(tr("%1 / %2").arg(0).arg(maxFrames));

    m_metricsLabel->setText(tr("Reference loaded: %1\nFrames: %2")
        .arg(QFileInfo(filePath).fileName())
        .arg(refFrames));

    // Update comparison
    updateComparison();
}

void ReferenceComparisonDialog::onModeChanged(int index) {
    m_currentMode = static_cast<DifferenceMode>(index);
    updateComparison();
}

void ReferenceComparisonDialog::onFrameChanged(int frame) {
    m_currentFrame = frame;
    int maxFrames = m_frameSlider->maximum() + 1;
    m_frameLabel->setText(tr("%1 / %2").arg(frame).arg(maxFrames));
    updateComparison();
}

void ReferenceComparisonDialog::updateComparison() {
    if (!m_decoder || !m_referenceDecoder) {
        return;
    }

    // Seek both decoders to current frame
    m_decoder->seekToFrame(m_currentFrame);
    m_referenceDecoder->seekToFrame(m_currentFrame);

    m_currentSourceFrame = m_decoder->decodeNextFrame();
    m_currentRefFrame = m_referenceDecoder->decodeNextFrame();

    if (!m_currentSourceFrame || !m_currentRefFrame) {
        m_displayLabel->setText(tr("Failed to decode frames"));
        return;
    }

    calculateDifference();
}

void ReferenceComparisonDialog::calculateDifference() {
    switch (m_currentMode) {
        case DifferenceMode::Compare:
            renderCompareMode();
            break;
        case DifferenceMode::PSNR:
            renderPSNRMode();
            break;
        case DifferenceMode::Subtraction:
            renderSubtractionMode();
            break;
        case DifferenceMode::Temperature:
            renderTemperatureMode();
            break;
    }

    m_displayLabel->setPixmap(QPixmap::fromImage(m_resultImage));
    m_displayLabel->adjustSize();  // Adjust label size to fit the pixmap
}

void ReferenceComparisonDialog::renderCompareMode() {
    // Side-by-side comparison
    if (!m_currentSourceFrame || !m_currentRefFrame) return;

    int width = m_currentSourceFrame->width;
    int height = m_currentSourceFrame->height;

    // Create side-by-side image
    m_resultImage = QImage(width * 2, height, QImage::Format_RGB888);

    // Convert both frames to RGB
    SwsContext* swsCtx = sws_getContext(
        width, height, static_cast<AVPixelFormat>(m_currentSourceFrame->format),
        width, height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!swsCtx) return;

    // Left side: source
    uint8_t* dstData[1] = { m_resultImage.bits() };
    int dstLinesize[1] = { static_cast<int>(m_resultImage.bytesPerLine()) };
    sws_scale(swsCtx, m_currentSourceFrame->data, m_currentSourceFrame->linesize, 0, height, dstData, dstLinesize);

    // Right side: reference
    uint8_t* refDstData[1] = { m_resultImage.bits() + width * 3 };
    sws_scale(swsCtx, m_currentRefFrame->data, m_currentRefFrame->linesize, 0, height, refDstData, dstLinesize);

    sws_freeContext(swsCtx);

    // Draw divider line
    QPainter painter(&m_resultImage);
    painter.setPen(QPen(Qt::white, 2));
    painter.drawLine(width, 0, width, height);

    // Add labels with text outline for better visibility
    painter.setFont(QFont("Arial", 16, QFont::Bold));

    // Source label - draw on the right side of left image (near divider)
    QString sourceText = tr("Source");
    QFontMetrics fm(painter.font());
    int sourceTextWidth = fm.horizontalAdvance(sourceText);
    int sourceX = width - sourceTextWidth - 15;  // Right side of left image
    int sourceY = 25;

    // Draw black outline
    painter.setPen(QPen(Qt::black, 3));
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx != 0 || dy != 0) {
                painter.drawText(sourceX + dx, sourceY + dy, sourceText);
            }
        }
    }
    // Draw white text
    painter.setPen(Qt::white);
    painter.drawText(sourceX, sourceY, sourceText);

    // Reference label - draw on the left side of right image (near divider)
    QString refText = tr("Reference");
    int refX = width + 15;  // Left side of right image
    int refY = 25;

    // Draw black outline
    painter.setPen(QPen(Qt::black, 3));
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx != 0 || dy != 0) {
                painter.drawText(refX + dx, refY + dy, refText);
            }
        }
    }
    // Draw white text
    painter.setPen(Qt::white);
    painter.drawText(refX, refY, refText);
}

void ReferenceComparisonDialog::renderPSNRMode() {
    // Block-based PSNR visualization
    if (!m_currentSourceFrame || !m_currentRefFrame) return;

    int width = m_currentSourceFrame->width;
    int height = m_currentSourceFrame->height;

    // Convert frames to RGB for display
    m_resultImage = QImage(width, height, QImage::Format_RGB888);

    SwsContext* swsCtx = sws_getContext(
        width, height, static_cast<AVPixelFormat>(m_currentSourceFrame->format),
        width, height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!swsCtx) return;

    uint8_t* dstData[1] = { m_resultImage.bits() };
    int dstLinesize[1] = { static_cast<int>(m_resultImage.bytesPerLine()) };
    sws_scale(swsCtx, m_currentSourceFrame->data, m_currentSourceFrame->linesize, 0, height, dstData, dstLinesize);
    sws_freeContext(swsCtx);

    // Calculate PSNR for 16x16 blocks and overlay
    QPainter painter(&m_resultImage);
    painter.setRenderHint(QPainter::Antialiasing);

    const int blockSize = 16;
    double totalPSNR = 0.0;
    int blockCount = 0;

    for (int y = 0; y < height; y += blockSize) {
        for (int x = 0; x < width; x += blockSize) {
            int bw = std::min(blockSize, width - x);
            int bh = std::min(blockSize, height - y);

            double psnr = calculateBlockPSNR(
                m_currentSourceFrame->data[0] + y * m_currentSourceFrame->linesize[0] + x,
                m_currentRefFrame->data[0] + y * m_currentRefFrame->linesize[0] + x,
                bw, bh, m_currentSourceFrame->linesize[0], m_currentRefFrame->linesize[0]
            );

            totalPSNR += psnr;
            blockCount++;

            // Draw colored rectangle
            QColor color = psnrToColor(psnr);
            color.setAlpha(128);
            painter.fillRect(x, y, bw, bh, color);

            // Draw PSNR value
            painter.setPen(Qt::white);
            painter.setFont(QFont("Arial", 8));
            painter.drawText(QRect(x, y, bw, bh), Qt::AlignCenter, QString::number(psnr, 'f', 1));
        }
    }

    double avgPSNR = blockCount > 0 ? totalPSNR / blockCount : 0.0;
    m_metricsLabel->setText(tr("Average PSNR: %1 dB\nFrame: %2")
        .arg(avgPSNR, 0, 'f', 2)
        .arg(m_currentFrame));
}

void ReferenceComparisonDialog::renderSubtractionMode() {
    // Pixel-by-pixel subtraction
    if (!m_currentSourceFrame || !m_currentRefFrame) return;

    int width = m_currentSourceFrame->width;
    int height = m_currentSourceFrame->height;

    m_resultImage = QImage(width, height, QImage::Format_RGB888);

    // Calculate difference on Y plane
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int srcVal = m_currentSourceFrame->data[0][y * m_currentSourceFrame->linesize[0] + x];
            int refVal = m_currentRefFrame->data[0][y * m_currentRefFrame->linesize[0] + x];
            int diff = srcVal - refVal + 128; // Offset to center at 128
            diff = std::clamp(diff, 0, 255);

            m_resultImage.setPixel(x, y, qRgb(diff, diff, diff));
        }
    }

    m_metricsLabel->setText(tr("Subtraction Mode\nGray = no difference\nBrighter = positive diff\nDarker = negative diff"));
}

void ReferenceComparisonDialog::renderTemperatureMode() {
    // Absolute difference with color gradient
    if (!m_currentSourceFrame || !m_currentRefFrame) return;

    int width = m_currentSourceFrame->width;
    int height = m_currentSourceFrame->height;

    m_resultImage = QImage(width, height, QImage::Format_RGB888);

    int maxDiff = 0;
    int totalDiff = 0;
    int pixelCount = 0;

    // Calculate absolute difference on Y plane
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int srcVal = m_currentSourceFrame->data[0][y * m_currentSourceFrame->linesize[0] + x];
            int refVal = m_currentRefFrame->data[0][y * m_currentRefFrame->linesize[0] + x];
            int diff = std::abs(srcVal - refVal);

            maxDiff = std::max(maxDiff, diff);
            totalDiff += diff;
            pixelCount++;

            QColor color = temperatureToColor(diff);
            m_resultImage.setPixel(x, y, color.rgb());
        }
    }

    double avgDiff = pixelCount > 0 ? static_cast<double>(totalDiff) / pixelCount : 0.0;
    m_metricsLabel->setText(tr("Temperature Map\nAvg Diff: %1\nMax Diff: %2\nBlack=no diff, Blue=slight, Green=medium, Red=large")
        .arg(avgDiff, 0, 'f', 2)
        .arg(maxDiff));
}

double ReferenceComparisonDialog::calculateBlockPSNR(const uint8_t* src, const uint8_t* ref,
                                                      int width, int height, int srcStride, int refStride) {
    double mse = 0.0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int diff = src[y * srcStride + x] - ref[y * refStride + x];
            mse += diff * diff;
        }
    }

    mse /= (width * height);

    if (mse == 0.0) {
        return 100.0; // Perfect match
    }

    return 10.0 * std::log10(255.0 * 255.0 / mse);
}

QColor ReferenceComparisonDialog::psnrToColor(double psnr) {
    // PSNR color coding:
    // > 40 dB: Green (excellent)
    // 30-40 dB: Yellow (good)
    // 20-30 dB: Orange (fair)
    // < 20 dB: Red (poor)

    if (psnr >= 40.0) {
        return QColor(0, 255, 0);  // Green
    } else if (psnr >= 30.0) {
        return QColor(255, 255, 0);  // Yellow
    } else if (psnr >= 20.0) {
        return QColor(255, 165, 0);  // Orange
    } else {
        return QColor(255, 0, 0);  // Red
    }
}

QColor ReferenceComparisonDialog::temperatureToColor(int diff) {
    // Temperature gradient:
    // 0: Black (no difference)
    // 1-10: Blue (slight difference)
    // 11-30: Green (medium difference)
    // 31+: Red (large difference)

    if (diff == 0) {
        return QColor(0, 0, 0);  // Black
    } else if (diff <= 10) {
        // Blue gradient
        int intensity = (diff * 255) / 10;
        return QColor(0, 0, intensity);
    } else if (diff <= 30) {
        // Green gradient
        int intensity = ((diff - 10) * 255) / 20;
        return QColor(0, intensity, 0);
    } else {
        // Red gradient
        int intensity = std::min(255, ((diff - 30) * 255) / 30);
        return QColor(intensity, 0, 0);
    }
}

} // namespace VideoStudio
