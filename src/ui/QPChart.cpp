#include "QPChart.h"
#include <QPaintEvent>
#include <QMouseEvent>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

QPChart::QPChart(QWidget* parent)
    : QWidget(parent), hasData(false),
      leftMargin(60), rightMargin(40), topMargin(20), bottomMargin(60),
      timelineHeight(150), distributionHeight(200), spacing(20),
      mouseInWidget(false) {
    setMinimumSize(800, 500);
    setStyleSheet("background-color: white;");
    setMouseTracking(true);
}

void QPChart::setFrameData(const QVector<FrameInfo>& frames) {
    frameData = frames;
    hasData = !frames.isEmpty();
    if (hasData) {
        calculateStatistics();
    }
    update();
}

void QPChart::clear() {
    frameData.clear();
    stats = QPStatistics();
    hasData = false;
    update();
}

void QPChart::calculateStatistics() {
    if (frameData.isEmpty()) {
        return;
    }

    stats = QPStatistics();
    stats.totalFrames = frameData.size();

    QVector<int> validQPs;
    QMap<char, QVector<int>> qpsByType;

    for (const auto& frame : frameData) {
        if (frame.qp >= 0) {
            validQPs.append(frame.qp);
            stats.framesWithQP++;
            stats.qpDistribution[frame.qp]++;
            qpsByType[frame.frameType].append(frame.qp);
        }
    }

    if (validQPs.isEmpty()) {
        return;
    }

    // Calculate min, max, avg
    stats.minQP = *std::min_element(validQPs.begin(), validQPs.end());
    stats.maxQP = *std::max_element(validQPs.begin(), validQPs.end());

    int64_t sum = 0;
    for (int qp : validQPs) {
        sum += qp;
    }
    stats.avgQP = static_cast<double>(sum) / validQPs.size();

    // Calculate median
    std::sort(validQPs.begin(), validQPs.end());
    stats.medianQP = validQPs[validQPs.size() / 2];

    // Calculate standard deviation
    double variance = 0.0;
    for (int qp : validQPs) {
        variance += std::pow(qp - stats.avgQP, 2);
    }
    variance /= validQPs.size();
    stats.stdDevQP = std::sqrt(variance);

    // Calculate average QP by type
    for (auto it = qpsByType.constBegin(); it != qpsByType.constEnd(); ++it) {
        char type = it.key();
        const QVector<int>& qps = it.value();
        if (!qps.isEmpty()) {
            int64_t typeSum = 0;
            for (int qp : qps) {
                typeSum += qp;
            }
            stats.avgQPByType[type] = static_cast<double>(typeSum) / qps.size();
        }
    }
}

void QPChart::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!hasData) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, "暂无数据");
        return;
    }

    if (stats.framesWithQP == 0) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter,
                        "此视频未包含 QP 信息\n提示：某些编码格式或播放器可能不提供 QP 数据");
        return;
    }

    drawTimelineView(painter);
    drawDistributionChart(painter);
    drawStatistics(painter);
    drawLegend(painter);

    if (mouseInWidget) {
        drawTooltip(painter);
    }
}

void QPChart::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
}

void QPChart::mouseMoveEvent(QMouseEvent* event) {
    mousePos = event->pos();
    mouseInWidget = true;
    update();
}

void QPChart::drawTimelineView(QPainter& painter) {
    int chartWidth = width() - leftMargin - rightMargin;
    int chartY = topMargin;

    // Draw title
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(leftMargin, chartY + 15, "QP 值时间轴");

    chartY += 25;

    if (stats.maxQP == 0) return;

    // Draw axes
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(leftMargin, chartY + timelineHeight,
                     leftMargin + chartWidth, chartY + timelineHeight);
    painter.drawLine(leftMargin, chartY, leftMargin, chartY + timelineHeight);

    // Draw Y axis labels (QP value)
    painter.setFont(QFont("Arial", 8));
    int qpRange = stats.maxQP - stats.minQP;
    int qpStep = std::max(1, qpRange / 4);
    for (int i = 0; i <= 4; i++) {
        int qp = stats.minQP + (qpRange * i) / 4;
        int y = chartY + timelineHeight - (timelineHeight * i) / 4;
        painter.drawLine(leftMargin - 5, y, leftMargin, y);
        painter.drawText(QRect(0, y - 10, leftMargin - 10, 20),
                        Qt::AlignRight | Qt::AlignVCenter,
                        QString::number(qp));
    }

    // Y axis label
    painter.save();
    painter.translate(15, chartY + timelineHeight / 2);
    painter.rotate(-90);
    painter.drawText(QRect(-30, -10, 60, 20), Qt::AlignCenter, "QP 值");
    painter.restore();

    // Draw QP lines by frame type
    QMap<char, QColor> typeColors;
    typeColors['I'] = QColor(255, 100, 100);
    typeColors['P'] = QColor(100, 255, 100);
    typeColors['B'] = QColor(100, 100, 255);

    for (int i = 0; i < frameData.size(); i++) {
        const FrameInfo& frame = frameData[i];
        if (frame.qp < 0) continue;

        int x = leftMargin + (chartWidth * i) / frameData.size();
        int qpHeight = (timelineHeight * (frame.qp - stats.minQP)) / qpRange;
        int y = chartY + timelineHeight - qpHeight;

        QColor color = typeColors.value(frame.frameType, Qt::gray);
        painter.setPen(QPen(color, 2));
        painter.drawLine(x, chartY + timelineHeight, x, y);
    }

    // Draw average line
    if (stats.avgQP > 0) {
        int avgY = chartY + timelineHeight -
                   (timelineHeight * (stats.avgQP - stats.minQP)) / qpRange;
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter.drawLine(leftMargin, avgY, leftMargin + chartWidth, avgY);

        painter.setFont(QFont("Arial", 8));
        painter.drawText(leftMargin + chartWidth + 5, avgY + 4,
                        QString("平均: %1").arg(stats.avgQP, 0, 'f', 1));
    }

    // Draw X axis labels (frame number)
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 8));
    for (int i = 0; i <= 5; i++) {
        int frameNum = (frameData.size() * i) / 5;
        int x = leftMargin + (chartWidth * i) / 5;
        painter.drawLine(x, chartY + timelineHeight,
                        x, chartY + timelineHeight + 5);
        painter.drawText(QRect(x - 30, chartY + timelineHeight + 8, 60, 20),
                        Qt::AlignCenter, QString::number(frameNum));
    }

    painter.drawText(QRect(leftMargin, chartY + timelineHeight + 30,
                          chartWidth, 20),
                    Qt::AlignCenter, "帧号");
}

void QPChart::drawDistributionChart(QPainter& painter) {
    int chartWidth = width() - leftMargin - rightMargin;
    int chartY = topMargin + timelineHeight + spacing + 25;

    // Draw title
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(leftMargin, chartY + 15, "QP 值分布");

    chartY += 25;

    if (stats.qpDistribution.isEmpty()) return;

    // Find max count
    int maxCount = 0;
    for (int count : stats.qpDistribution.values()) {
        maxCount = std::max(maxCount, count);
    }

    if (maxCount == 0) return;

    // Draw axes
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(leftMargin, chartY, leftMargin, chartY + distributionHeight);
    painter.drawLine(leftMargin, chartY + distributionHeight,
                     leftMargin + chartWidth, chartY + distributionHeight);

    // Draw Y axis labels (count)
    painter.setFont(QFont("Arial", 8));
    for (int i = 0; i <= 4; i++) {
        int count = (maxCount * i) / 4;
        int y = chartY + distributionHeight - (distributionHeight * i) / 4;
        painter.drawLine(leftMargin - 5, y, leftMargin, y);
        painter.drawText(QRect(0, y - 10, leftMargin - 10, 20),
                        Qt::AlignRight | Qt::AlignVCenter,
                        QString::number(count));
    }

    // Y axis label
    painter.save();
    painter.translate(15, chartY + distributionHeight / 2);
    painter.rotate(-90);
    painter.drawText(QRect(-30, -10, 60, 20), Qt::AlignCenter, "帧数量");
    painter.restore();

    // Draw bars
    int qpRange = stats.maxQP - stats.minQP + 1;
    int barWidth = std::max(1, chartWidth / qpRange - 2);

    for (auto it = stats.qpDistribution.constBegin();
         it != stats.qpDistribution.constEnd(); ++it) {
        int qp = it.key();
        int count = it.value();

        int x = leftMargin + (chartWidth * (qp - stats.minQP)) / qpRange;
        int barHeight = (distributionHeight * count) / maxCount;

        painter.fillRect(x, chartY + distributionHeight - barHeight,
                        barWidth, barHeight, QColor(100, 150, 255));
        painter.setPen(Qt::black);
        painter.drawRect(x, chartY + distributionHeight - barHeight,
                        barWidth, barHeight);
    }

    // Draw X axis labels (QP values)
    painter.setPen(Qt::black);
    int labelInterval = std::max(1, qpRange / 10);
    for (int qp = stats.minQP; qp <= stats.maxQP; qp += labelInterval) {
        int x = leftMargin + (chartWidth * (qp - stats.minQP)) / qpRange;
        painter.drawLine(x, chartY + distributionHeight,
                        x, chartY + distributionHeight + 5);
        painter.drawText(QRect(x - 15, chartY + distributionHeight + 8, 30, 20),
                        Qt::AlignCenter, QString::number(qp));
    }

    painter.drawText(QRect(leftMargin, chartY + distributionHeight + 30,
                          chartWidth, 20),
                    Qt::AlignCenter, "QP 值");
}

void QPChart::drawStatistics(QPainter& painter) {
    int statsX = leftMargin + 10;
    int statsY = topMargin + timelineHeight + spacing + 50;

    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 9));

    QStringList statsList;
    statsList << QString("总帧数: %1").arg(stats.totalFrames);
    statsList << QString("有 QP 数据: %1").arg(stats.framesWithQP);
    statsList << QString("最小 QP: %1").arg(stats.minQP);
    statsList << QString("最大 QP: %1").arg(stats.maxQP);
    statsList << QString("平均 QP: %1").arg(stats.avgQP, 0, 'f', 2);
    statsList << QString("中位数: %1").arg(stats.medianQP, 0, 'f', 1);
    statsList << QString("标准差: %1").arg(stats.stdDevQP, 0, 'f', 2);

    statsList << "";
    statsList << "按帧类型:";
    for (auto it = stats.avgQPByType.constBegin();
         it != stats.avgQPByType.constEnd(); ++it) {
        statsList << QString("  %1 帧: %2")
            .arg(it.key())
            .arg(it.value(), 0, 'f', 2);
    }

    int lineHeight = 16;
    for (int i = 0; i < statsList.size(); i++) {
        painter.drawText(statsX, statsY + i * lineHeight, statsList[i]);
    }
}

void QPChart::drawLegend(QPainter& painter) {
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

void QPChart::drawTooltip(QPainter& painter) {
    int chartWidth = width() - leftMargin - rightMargin;
    int timelineY = topMargin + 25;

    if (mousePos.x() >= leftMargin && mousePos.x() <= leftMargin + chartWidth &&
        mousePos.y() >= timelineY && mousePos.y() <= timelineY + timelineHeight) {

        if (!frameData.isEmpty()) {
            int frameIndex = ((mousePos.x() - leftMargin) * frameData.size()) / chartWidth;
            frameIndex = std::max(0, std::min(frameIndex, static_cast<int>(frameData.size()) - 1));

            const FrameInfo& frame = frameData[frameIndex];

            if (frame.qp >= 0) {
                QString tooltip = QString("帧 #%1\n类型: %2\nQP: %3\n大小: %4 bytes")
                    .arg(frame.frameNumber)
                    .arg(frame.frameType)
                    .arg(frame.qp)
                    .arg(frame.size);

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
}

QString QPChart::formatQP(int qp) {
    return QString::number(qp);
}
