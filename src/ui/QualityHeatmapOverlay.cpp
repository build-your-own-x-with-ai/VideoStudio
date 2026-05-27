#include "QualityHeatmapOverlay.h"
#include <QPaintEvent>
#include <cmath>

QualityHeatmapOverlay::QualityHeatmapOverlay(QWidget* parent)
    : QWidget(parent), mode(None), blockRows(0), blockCols(0) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
}

void QualityHeatmapOverlay::setHeatmapMode(HeatmapMode newMode) {
    mode = newMode;
    update();
}

void QualityHeatmapOverlay::setHeatmapData(const QVector<double>& data, int rows, int cols) {
    heatmapData = data;
    blockRows = rows;
    blockCols = cols;
    update();
}

void QualityHeatmapOverlay::setHeatmapImage(const QImage& image) {
    heatmapImage = image;
    update();
}

void QualityHeatmapOverlay::clear() {
    heatmapData.clear();
    heatmapImage = QImage();
    blockRows = 0;
    blockCols = 0;
    update();
}

void QualityHeatmapOverlay::paintEvent(QPaintEvent* event) {
    if (mode == None) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (mode == Temperature || mode == Subtraction) {
        drawImageHeatmap(painter);
    } else if (mode == PSNR || mode == SSIM) {
        drawBlockHeatmap(painter);
    }
}

void QualityHeatmapOverlay::drawBlockHeatmap(QPainter& painter) {
    if (heatmapData.isEmpty() || blockRows == 0 || blockCols == 0) {
        return;
    }

    int blockWidth = width() / blockCols;
    int blockHeight = height() / blockRows;

    for (int by = 0; by < blockRows; by++) {
        for (int bx = 0; bx < blockCols; bx++) {
            int index = by * blockCols + bx;
            if (index >= heatmapData.size()) continue;

            double value = heatmapData[index];
            QColor color = valueToColor(value, mode);

            int x = bx * blockWidth;
            int y = by * blockHeight;

            painter.fillRect(x, y, blockWidth, blockHeight, color);
        }
    }
}

void QualityHeatmapOverlay::drawImageHeatmap(QPainter& painter) {
    if (heatmapImage.isNull()) {
        return;
    }

    // 缩放热力图以适应窗口大小
    QImage scaled = heatmapImage.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    painter.drawImage(0, 0, scaled);
}

QColor QualityHeatmapOverlay::valueToColor(double value, HeatmapMode mode) {
    if (mode == PSNR) {
        // PSNR: 绿色(高质量) -> 黄色 -> 红色(低质量)
        // 范围：20-50 dB
        double normalized = (value - 20.0) / 30.0;
        normalized = std::max(0.0, std::min(1.0, normalized));

        int r, g, b;
        if (normalized > 0.5) {
            // 绿色到黄色
            r = static_cast<int>(255 * (1.0 - normalized) * 2);
            g = 255;
            b = 0;
        } else {
            // 黄色到红色
            r = 255;
            g = static_cast<int>(255 * normalized * 2);
            b = 0;
        }

        return QColor(r, g, b, 150);

    } else if (mode == SSIM) {
        // SSIM: 绿色(高质量) -> 黄色 -> 红色(低质量)
        // 范围：0-1
        double normalized = value;
        normalized = std::max(0.0, std::min(1.0, normalized));

        int r, g, b;
        if (normalized > 0.5) {
            // 绿色到黄色
            r = static_cast<int>(255 * (1.0 - normalized) * 2);
            g = 255;
            b = 0;
        } else {
            // 黄色到红色
            r = 255;
            g = static_cast<int>(255 * normalized * 2);
            b = 0;
        }

        return QColor(r, g, b, 150);
    }

    return QColor(128, 128, 128, 150);
}
