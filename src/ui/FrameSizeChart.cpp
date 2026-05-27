#include "FrameSizeChart.h"
#include <QPaintEvent>
#include <QMouseEvent>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

FrameSizeChart::FrameSizeChart(QWidget* parent)
    : QWidget(parent), hasData(false),
      leftMargin(60), rightMargin(40), topMargin(20), bottomMargin(60),
      timelineHeight(150), histogramHeight(250), spacing(20),
      mouseInWidget(false) {
    setMinimumSize(800, 500);
    setStyleSheet("background-color: white;");
    setMouseTracking(true);
}

void FrameSizeChart::setDistribution(const FrameSizeDistribution& dist) {
    distribution = dist;
    hasData = !dist.binCounts.isEmpty();
    update();
}

void FrameSizeChart::setFrameData(const QVector<FrameInfo>& frames) {
    frameData = frames;
    update();
}

void FrameSizeChart::clear() {
    distribution = FrameSizeDistribution();
    frameData.clear();
    hasData = false;
    update();
}

void FrameSizeChart::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!hasData) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, "暂无数据");
        return;
    }

    drawTimelineView(painter);
    drawHistogram(painter);
    drawStatistics(painter);
    drawLegend(painter);

    if (mouseInWidget) {
        drawTooltip(painter);
    }
}

void FrameSizeChart::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
}

void FrameSizeChart::mouseMoveEvent(QMouseEvent* event) {
    mousePos = event->pos();
    mouseInWidget = true;
    update();
}

void FrameSizeChart::drawTimelineView(QPainter& painter) {
    if (frameData.isEmpty()) {
        return;
    }

    int chartWidth = width() - leftMargin - rightMargin;
    int chartY = topMargin;

    // Draw title
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(leftMargin, chartY + 15, "帧大小时间轴");

    chartY += 25;

    // Find max frame size for scaling
    int maxSize = 0;
    for (const auto& frame : frameData) {
        maxSize = std::max(maxSize, frame.size);
    }

    if (maxSize == 0) return;

    // Draw axes
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(leftMargin, chartY + timelineHeight,
                     leftMargin + chartWidth, chartY + timelineHeight);
    painter.drawLine(leftMargin, chartY, leftMargin, chartY + timelineHeight);

    // Draw Y axis labels (frame size)
    painter.setFont(QFont("Arial", 8));
    for (int i = 0; i <= 4; i++) {
        int size = (maxSize * i) / 4;
        int y = chartY + timelineHeight - (timelineHeight * i) / 4;
        painter.drawLine(leftMargin - 5, y, leftMargin, y);
        painter.drawText(QRect(0, y - 10, leftMargin - 10, 20),
                        Qt::AlignRight | Qt::AlignVCenter,
                        formatSize(size));
    }

    // Draw frame size lines by type
    QMap<char, QColor> typeColors;
    typeColors['I'] = QColor(255, 100, 100);
    typeColors['P'] = QColor(100, 255, 100);
    typeColors['B'] = QColor(100, 100, 255);

    for (int i = 0; i < frameData.size(); i++) {
        const FrameInfo& frame = frameData[i];
        int x = leftMargin + (chartWidth * i) / frameData.size();
        int height = (timelineHeight * frame.size) / maxSize;
        int y = chartY + timelineHeight - height;

        QColor color = typeColors.value(frame.frameType, Qt::gray);
        painter.setPen(color);
        painter.drawLine(x, chartY + timelineHeight, x, y);
    }

    // Draw X axis labels (time)
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 8));
    double duration = frameData.last().timestamp;
    for (int i = 0; i <= 5; i++) {
        double time = (duration * i) / 5;
        int x = leftMargin + (chartWidth * i) / 5;
        painter.drawLine(x, chartY + timelineHeight,
                        x, chartY + timelineHeight + 5);
        painter.drawText(QRect(x - 30, chartY + timelineHeight + 8, 60, 20),
                        Qt::AlignCenter, formatTime(time));
    }
}

void FrameSizeChart::drawHistogram(QPainter& painter) {
    if (distribution.binCounts.isEmpty()) {
        return;
    }

    int chartWidth = width() - leftMargin - rightMargin;
    int chartY = topMargin + timelineHeight + spacing + 25;

    // Draw title
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(leftMargin, chartY + 15, "帧大小分布直方图");

    chartY += 25;

    // Find max count
    int maxCount = 0;
    for (int count : distribution.binCounts) {
        maxCount = std::max(maxCount, count);
    }

    if (maxCount == 0) return;

    // Draw axes
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(leftMargin, chartY, leftMargin, chartY + histogramHeight);
    painter.drawLine(leftMargin, chartY + histogramHeight,
                     leftMargin + chartWidth, chartY + histogramHeight);

    // Draw Y axis labels (count)
    painter.setFont(QFont("Arial", 8));
    for (int i = 0; i <= 4; i++) {
        int count = (maxCount * i) / 4;
        int y = chartY + histogramHeight - (histogramHeight * i) / 4;
        painter.drawLine(leftMargin - 5, y, leftMargin, y);
        painter.drawText(QRect(0, y - 10, leftMargin - 10, 20),
                        Qt::AlignRight | Qt::AlignVCenter,
                        QString::number(count));
    }

    // Draw bars
    int numBins = distribution.binCounts.size();
    int barWidth = std::max(1, chartWidth / numBins - 1);

    QMap<QString, QColor> typeColors;
    typeColors["I"] = QColor(255, 100, 100);
    typeColors["P"] = QColor(100, 255, 100);
    typeColors["B"] = QColor(100, 100, 255);

    for (int i = 0; i < numBins; i++) {
        int x = leftMargin + (chartWidth * i) / numBins;
        int currentY = chartY + histogramHeight;

        // Stack bars by type
        for (auto it = distribution.typeDistribution.constBegin();
             it != distribution.typeDistribution.constEnd(); ++it) {
            QString type = it.key();
            const QVector<int>& typeCounts = it.value();

            if (i < typeCounts.size() && typeCounts[i] > 0) {
                int barHeight = (histogramHeight * typeCounts[i]) / maxCount;
                QColor color = typeColors.value(type, Qt::gray);

                painter.fillRect(x, currentY - barHeight, barWidth, barHeight, color);
                currentY -= barHeight;
            }
        }
    }

    // Draw X axis labels (size ranges)
    painter.setPen(Qt::black);
    int labelInterval = std::max(1, numBins / 8);
    for (int i = 0; i < numBins; i += labelInterval) {
        int x = leftMargin + (chartWidth * i) / numBins;
        painter.drawLine(x, chartY + histogramHeight,
                        x, chartY + histogramHeight + 5);

        QString label = formatSize(distribution.binEdges[i]);
        painter.save();
        painter.translate(x, chartY + histogramHeight + 10);
        painter.rotate(45);
        painter.drawText(QRect(0, 0, 100, 20), Qt::AlignLeft | Qt::AlignVCenter, label);
        painter.restore();
    }
}

void FrameSizeChart::drawStatistics(QPainter& painter) {
    int statsX = leftMargin + 10;
    int statsY = topMargin + timelineHeight + spacing + 50;

    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 9));

    QStringList stats;
    stats << QString("总帧数: %1").arg(distribution.totalFrames);
    stats << QString("最小: %1").arg(formatSize(distribution.minSize));
    stats << QString("最大: %1").arg(formatSize(distribution.maxSize));
    stats << QString("平均: %1").arg(formatSize(static_cast<int>(distribution.avgSize)));

    // Calculate median and std dev
    if (!frameData.isEmpty()) {
        QVector<int> sizes;
        for (const auto& frame : frameData) {
            sizes.append(frame.size);
        }
        std::sort(sizes.begin(), sizes.end());

        int median = sizes[sizes.size() / 2];
        stats << QString("中位数: %1").arg(formatSize(median));

        // Standard deviation
        double variance = 0.0;
        for (int size : sizes) {
            variance += std::pow(size - distribution.avgSize, 2);
        }
        variance /= sizes.size();
        double stdDev = std::sqrt(variance);
        stats << QString("标准差: %1").arg(formatSize(static_cast<int>(stdDev)));

        // Percentiles
        int p25 = sizes[sizes.size() / 4];
        int p75 = sizes[sizes.size() * 3 / 4];
        stats << QString("25%: %1").arg(formatSize(p25));
        stats << QString("75%: %1").arg(formatSize(p75));
    }

    int lineHeight = 16;
    for (int i = 0; i < stats.size(); i++) {
        painter.drawText(statsX, statsY + i * lineHeight, stats[i]);
    }
}

void FrameSizeChart::drawLegend(QPainter& painter) {
    int legendX = width() - rightMargin - 100;
    int legendY = topMargin + timelineHeight + spacing + 50;
    int boxSize = 12;
    int spacing = 5;

    QMap<QString, QColor> typeColors;
    typeColors["I 帧"] = QColor(255, 100, 100);
    typeColors["P 帧"] = QColor(100, 255, 100);
    typeColors["B 帧"] = QColor(100, 100, 255);

    painter.setFont(QFont("Arial", 9));

    int index = 0;
    for (auto it = typeColors.constBegin(); it != typeColors.constEnd(); ++it) {
        int y = legendY + index * (boxSize + spacing);

        painter.fillRect(legendX, y, boxSize, boxSize, it.value());
        painter.setPen(Qt::black);
        painter.drawRect(legendX, y, boxSize, boxSize);
        painter.drawText(legendX + boxSize + 5, y + boxSize - 2, it.key());

        index++;
    }
}

void FrameSizeChart::drawTooltip(QPainter& painter) {
    // Check if mouse is in timeline area
    int chartWidth = width() - leftMargin - rightMargin;
    int timelineY = topMargin + 25;

    if (mousePos.x() >= leftMargin && mousePos.x() <= leftMargin + chartWidth &&
        mousePos.y() >= timelineY && mousePos.y() <= timelineY + timelineHeight) {

        if (!frameData.isEmpty()) {
            int frameIndex = ((mousePos.x() - leftMargin) * frameData.size()) / chartWidth;
            frameIndex = std::max(0, std::min(frameIndex, static_cast<int>(frameData.size()) - 1));

            const FrameInfo& frame = frameData[frameIndex];

            QString tooltip = QString("帧 #%1\n类型: %2\n大小: %3\n时间: %4s")
                .arg(frame.frameNumber)
                .arg(frame.frameType)
                .arg(formatSize(frame.size))
                .arg(frame.timestamp, 0, 'f', 3);

            QFontMetrics fm(painter.font());
            QRect textRect = fm.boundingRect(QRect(0, 0, 200, 100),
                                            Qt::AlignLeft | Qt::TextWordWrap, tooltip);
            textRect.adjust(-5, -5, 5, 5);
            textRect.moveTopLeft(mousePos + QPoint(10, 10));

            painter.fillRect(textRect, QColor(255, 255, 220, 230));
            painter.setPen(Qt::black);
            painter.drawRect(textRect);
            painter.drawText(textRect, Qt::AlignLeft | Qt::TextWordWrap, tooltip);
        }
    }
}

QString FrameSizeChart::formatSize(int bytes) {
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 2);
    }
}

QString FrameSizeChart::formatTime(double seconds) {
    int totalSecs = static_cast<int>(seconds);
    int minutes = totalSecs / 60;
    int secs = totalSecs % 60;
    int ms = static_cast<int>((seconds - totalSecs) * 1000);

    return QString("%1:%2.%3")
        .arg(minutes)
        .arg(secs, 2, 10, QChar('0'))
        .arg(ms, 3, 10, QChar('0'));
}
