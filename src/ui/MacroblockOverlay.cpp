#include "MacroblockOverlay.h"
#include <QPaintEvent>
#include <cmath>

MacroblockOverlay::MacroblockOverlay(QWidget* parent)
    : QWidget(parent), showBoundaries(false), showMotionVectors(false), showQPHeatmap(false), showSizes(false), showExtendedParams(false) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
}

void MacroblockOverlay::setMacroblocks(const QVector<MacroblockInfo>& mbs) {
    macroblocks = mbs;
    update();
}

void MacroblockOverlay::clear() {
    macroblocks.clear();
    update();
}

void MacroblockOverlay::paintEvent(QPaintEvent* event) {
    if (macroblocks.isEmpty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (showQPHeatmap) {
        drawQPHeatmap(painter);
    }

    if (showBoundaries) {
        drawBoundaries(painter);
    }

    if (showMotionVectors) {
        drawMotionVectors(painter);
    }

    if (showSizes) {
        drawSizes(painter);
    }

    if (showExtendedParams) {
        drawExtendedParams(painter);
    }
}

void MacroblockOverlay::drawBoundaries(QPainter& painter) {
    if (macroblocks.isEmpty()) return;

    // Calculate scaling factor based on widget size vs video size
    // Assume macroblocks are in video coordinate space
    int videoWidth = 0;
    int videoHeight = 0;

    // Find max coordinates to determine video dimensions
    for (const auto& mb : macroblocks) {
        videoWidth = std::max(videoWidth, mb.x + mb.width);
        videoHeight = std::max(videoHeight, mb.y + mb.height);
    }

    if (videoWidth == 0 || videoHeight == 0) return;

    double scaleX = static_cast<double>(width()) / videoWidth;
    double scaleY = static_cast<double>(height()) / videoHeight;

    for (const auto& mb : macroblocks) {
        QColor color;
        switch (mb.type) {
            case 'I':
                color = QColor(255, 100, 100, 150);  // Red for Intra
                break;
            case 'P':
                color = QColor(100, 255, 100, 150);  // Green for Inter
                break;
            case 'S':
                color = QColor(100, 100, 255, 150);  // Blue for Skip
                break;
            default:
                color = QColor(200, 200, 200, 100);  // Gray for Unknown
                break;
        }

        painter.setPen(QPen(color, 1));

        // Scale macroblock coordinates to widget space
        int x = static_cast<int>(mb.x * scaleX);
        int y = static_cast<int>(mb.y * scaleY);
        int w = static_cast<int>(mb.width * scaleX);
        int h = static_cast<int>(mb.height * scaleY);

        painter.drawRect(x, y, w, h);
    }
}

void MacroblockOverlay::drawMotionVectors(QPainter& painter) {
    if (macroblocks.isEmpty()) return;

    // Calculate scaling factor
    int videoWidth = 0;
    int videoHeight = 0;
    for (const auto& mb : macroblocks) {
        videoWidth = std::max(videoWidth, mb.x + mb.width);
        videoHeight = std::max(videoHeight, mb.y + mb.height);
    }
    if (videoWidth == 0 || videoHeight == 0) return;

    double scaleX = static_cast<double>(width()) / videoWidth;
    double scaleY = static_cast<double>(height()) / videoHeight;

    painter.setPen(QPen(QColor(255, 255, 0, 200), 2));  // Yellow arrows

    for (const auto& mb : macroblocks) {
        if (mb.mvX == 0 && mb.mvY == 0) {
            continue;  // Skip zero motion vectors
        }

        // Draw arrow from macroblock center (scaled)
        int centerX = static_cast<int>((mb.x + mb.width / 2) * scaleX);
        int centerY = static_cast<int>((mb.y + mb.height / 2) * scaleY);

        // Scale motion vector for visibility (motion vectors are in quarter-pixel units)
        double scale = 0.25 * std::min(scaleX, scaleY);
        int endX = centerX + static_cast<int>(mb.mvX * scale);
        int endY = centerY + static_cast<int>(mb.mvY * scale);

        // Draw line
        painter.drawLine(centerX, centerY, endX, endY);

        // Draw arrowhead
        double angle = std::atan2(endY - centerY, endX - centerX);
        int arrowSize = 8;
        QPointF arrowP1 = QPointF(endX - arrowSize * std::cos(angle - M_PI / 6),
                                   endY - arrowSize * std::sin(angle - M_PI / 6));
        QPointF arrowP2 = QPointF(endX - arrowSize * std::cos(angle + M_PI / 6),
                                   endY - arrowSize * std::sin(angle + M_PI / 6));

        painter.drawLine(endX, endY, arrowP1.x(), arrowP1.y());
        painter.drawLine(endX, endY, arrowP2.x(), arrowP2.y());
    }
}

void MacroblockOverlay::drawQPHeatmap(QPainter& painter) {
    if (macroblocks.isEmpty()) return;

    // Find min and max QP for color mapping
    int minQP = 51;
    int maxQP = 0;
    bool hasQP = false;

    for (const auto& mb : macroblocks) {
        if (mb.qp >= 0) {
            minQP = std::min(minQP, mb.qp);
            maxQP = std::max(maxQP, mb.qp);
            hasQP = true;
        }
    }

    if (!hasQP || minQP == maxQP) {
        return;  // No QP data or all same QP
    }

    // Calculate scaling factor
    int videoWidth = 0;
    int videoHeight = 0;
    for (const auto& mb : macroblocks) {
        videoWidth = std::max(videoWidth, mb.x + mb.width);
        videoHeight = std::max(videoHeight, mb.y + mb.height);
    }
    if (videoWidth == 0 || videoHeight == 0) return;

    double scaleX = static_cast<double>(width()) / videoWidth;
    double scaleY = static_cast<double>(height()) / videoHeight;

    // Draw heatmap
    for (const auto& mb : macroblocks) {
        if (mb.qp < 0) {
            continue;
        }

        // Map QP to color (blue = low QP/high quality, red = high QP/low quality)
        double normalized = static_cast<double>(mb.qp - minQP) / (maxQP - minQP);
        int r = static_cast<int>(255 * normalized);
        int g = 0;
        int b = static_cast<int>(255 * (1.0 - normalized));

        QColor color(r, g, b, 120);

        // Scale macroblock coordinates
        int x = static_cast<int>(mb.x * scaleX);
        int y = static_cast<int>(mb.y * scaleY);
        int w = static_cast<int>(mb.width * scaleX);
        int h = static_cast<int>(mb.height * scaleY);

        painter.fillRect(x, y, w, h, color);
    }
}

void MacroblockOverlay::drawSizes(QPainter& painter) {
    if (macroblocks.isEmpty()) return;

    // Calculate scaling factor
    int videoWidth = 0;
    int videoHeight = 0;
    for (const auto& mb : macroblocks) {
        videoWidth = std::max(videoWidth, mb.x + mb.width);
        videoHeight = std::max(videoHeight, mb.y + mb.height);
    }
    if (videoWidth == 0 || videoHeight == 0) return;

    double scaleX = static_cast<double>(width()) / videoWidth;
    double scaleY = static_cast<double>(height()) / videoHeight;

    // Draw size labels and color-coded backgrounds
    for (const auto& mb : macroblocks) {
        int x = static_cast<int>(mb.x * scaleX);
        int y = static_cast<int>(mb.y * scaleY);
        int w = static_cast<int>(mb.width * scaleX);
        int h = static_cast<int>(mb.height * scaleY);

        // Color mapping based on block size
        QColor bgColor;
        if (mb.width >= 64 || mb.height >= 64) {
            bgColor = QColor(0, 0, 139, 80);      // Dark blue for 64x64
        } else if (mb.width >= 32 || mb.height >= 32) {
            bgColor = QColor(0, 0, 255, 80);      // Blue for 32x32
        } else if (mb.width >= 16 || mb.height >= 16) {
            bgColor = QColor(0, 255, 0, 80);      // Green for 16x16
        } else if (mb.width >= 8 || mb.height >= 8) {
            bgColor = QColor(255, 255, 0, 80);    // Yellow for 8x8
        } else {
            bgColor = QColor(255, 0, 0, 80);      // Red for 4x4
        }

        // Fill background
        painter.fillRect(x, y, w, h, bgColor);

        // Draw text label
        QString sizeText = QString("%1x%2").arg(mb.width).arg(mb.height);

        // Adjust font size based on block size
        QFont font = painter.font();
        int fontSize = std::max(8, std::min(w, h) / 4);
        font.setPointSize(fontSize);
        painter.setFont(font);

        // Draw text in center
        painter.setPen(QColor(255, 255, 255));
        QRect textRect(x, y, w, h);
        painter.drawText(textRect, Qt::AlignCenter, sizeText);
    }
}

void MacroblockOverlay::drawExtendedParams(QPainter& painter) {
    if (macroblocks.isEmpty()) return;

    // Calculate scaling factor
    int videoWidth = 0;
    int videoHeight = 0;
    for (const auto& mb : macroblocks) {
        videoWidth = std::max(videoWidth, mb.x + mb.width);
        videoHeight = std::max(videoHeight, mb.y + mb.height);
    }
    if (videoWidth == 0 || videoHeight == 0) return;

    double scaleX = static_cast<double>(width()) / videoWidth;
    double scaleY = static_cast<double>(height()) / videoHeight;

    // Draw extended parameters as text labels
    painter.setPen(QColor(255, 255, 255));
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    for (const auto& mb : macroblocks) {
        int x = static_cast<int>(mb.x * scaleX);
        int y = static_cast<int>(mb.y * scaleY);
        int w = static_cast<int>(mb.width * scaleX);
        int h = static_cast<int>(mb.height * scaleY);

        // Skip if block is too small to display text
        if (w < 20 || h < 20) continue;

        QStringList params;

        // QP value
        if (mb.qp >= 0) {
            params << QString("Q:%1").arg(mb.qp);
        }

        // Reference frame index
        if (mb.source >= 0) {
            params << QString("R:%1").arg(mb.source);
        }

        // Extended parameters (HEVC specific)
        if (mb.hasExtendedParams) {
            if (mb.saoEnabled) {
                params << QString("SAO:%1").arg(mb.saoType);
            }
            if (mb.merged) {
                params << "M";
            }
        }

        // Draw parameters
        if (!params.isEmpty()) {
            QString text = params.join(" ");
            QRect textRect(x + 2, y + 2, w - 4, h - 4);

            // Draw background for better readability
            QRect boundingRect = painter.fontMetrics().boundingRect(textRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text);
            painter.fillRect(boundingRect.adjusted(-2, -2, 2, 2), QColor(0, 0, 0, 180));

            // Draw text
            painter.setPen(QColor(255, 255, 0));
            painter.drawText(textRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text);
        }
    }
}


