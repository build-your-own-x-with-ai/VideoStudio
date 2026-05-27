#include "TimestampChart.h"
#include <QPaintEvent>
#include <QMouseEvent>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

TimestampChart::TimestampChart(QWidget* parent)
    : QWidget(parent), analyzer(nullptr), hasData(false),
      leftMargin(80), rightMargin(40), topMargin(20), bottomMargin(60),
      chartHeight(150), chartSpacing(20), mouseInWidget(false) {
    analyzer = new TimestampAnalyzer();
    setMinimumSize(800, 600);
    setStyleSheet("background-color: white;");
    setMouseTracking(true);
}

void TimestampChart::setFrameData(const QVector<FrameInfo>& frames) {
    frameData = frames;
    hasData = !frames.isEmpty();
    if (hasData) {
        analyzer->analyze(frames);
    }
    update();
}

void TimestampChart::clear() {
    frameData.clear();
    analyzer->clear();
    hasData = false;
    update();
}

void TimestampChart::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!hasData) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, "暂无数据");
        return;
    }

    drawPTSDTSChart(painter);
    drawDiffChart(painter);
    drawIntervalChart(painter);
    drawStatistics(painter);

    if (mouseInWidget) {
        drawTooltip(painter);
    }
}

void TimestampChart::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
}

void TimestampChart::mouseMoveEvent(QMouseEvent* event) {
    mousePos = event->pos();
    mouseInWidget = true;
    update();
}

void TimestampChart::leaveEvent(QEvent* event) {
    mouseInWidget = false;
    update();
}

void TimestampChart::drawPTSDTSChart(QPainter& painter) {
    int chartWidth = width() - leftMargin - rightMargin;
    int chartY = topMargin;

    // Draw title
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(leftMargin, chartY + 15, "PTS/DTS 时间轴");

    chartY += 25;

    QVector<double> ptsCurve = analyzer->getPTSCurve();
    QVector<double> dtsCurve = analyzer->getDTSCurve();

    if (ptsCurve.isEmpty()) return;

    // Find min and max for Y axis
    double minVal = std::min(*std::min_element(ptsCurve.begin(), ptsCurve.end()),
                             *std::min_element(dtsCurve.begin(), dtsCurve.end()));
    double maxVal = std::max(*std::max_element(ptsCurve.begin(), ptsCurve.end()),
                             *std::max_element(dtsCurve.begin(), dtsCurve.end()));
    double range = maxVal - minVal;
    if (range == 0) range = 1;

    // Draw axes
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(leftMargin, chartY + chartHeight,
                     leftMargin + chartWidth, chartY + chartHeight);
    painter.drawLine(leftMargin, chartY, leftMargin, chartY + chartHeight);

    // Draw Y axis labels
    painter.setFont(QFont("Arial", 8));
    for (int i = 0; i <= 4; i++) {
        int64_t val = static_cast<int64_t>(minVal + (range * i) / 4);
        int y = chartY + chartHeight - (chartHeight * i) / 4;
        painter.drawLine(leftMargin - 5, y, leftMargin, y);
        painter.drawText(QRect(0, y - 10, leftMargin - 10, 20),
                        Qt::AlignRight | Qt::AlignVCenter,
                        QString::number(val));
    }

    // Y axis label
    painter.save();
    painter.translate(15, chartY + chartHeight / 2);
    painter.rotate(-90);
    painter.drawText(QRect(-40, -10, 80, 20), Qt::AlignCenter, "时间戳值");
    painter.restore();

    // Draw PTS curve (blue)
    painter.setPen(QPen(QColor(0, 100, 255), 2));
    for (int i = 1; i < ptsCurve.size(); i++) {
        int x1 = leftMargin + (chartWidth * (i-1)) / ptsCurve.size();
        int x2 = leftMargin + (chartWidth * i) / ptsCurve.size();
        int y1 = chartY + chartHeight - (chartHeight * (ptsCurve[i-1] - minVal)) / range;
        int y2 = chartY + chartHeight - (chartHeight * (ptsCurve[i] - minVal)) / range;
        painter.drawLine(x1, y1, x2, y2);
    }

    // Draw DTS curve (green)
    painter.setPen(QPen(QColor(0, 200, 100), 2));
    for (int i = 1; i < dtsCurve.size(); i++) {
        int x1 = leftMargin + (chartWidth * (i-1)) / dtsCurve.size();
        int x2 = leftMargin + (chartWidth * i) / dtsCurve.size();
        int y1 = chartY + chartHeight - (chartHeight * (dtsCurve[i-1] - minVal)) / range;
        int y2 = chartY + chartHeight - (chartHeight * (dtsCurve[i] - minVal)) / range;
        painter.drawLine(x1, y1, x2, y2);
    }

    // Mark anomalies
    TimestampStats stats = analyzer->getStats();
    painter.setPen(QPen(Qt::red, 6));
    for (int frameIdx : stats.discontinuityFrames) {
        int x = leftMargin + (chartWidth * frameIdx) / ptsCurve.size();
        painter.drawPoint(x, chartY + chartHeight / 2);
    }

    // Draw X axis labels
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 8));
    for (int i = 0; i <= 5; i++) {
        int frameNum = (frameData.size() * i) / 5;
        int x = leftMargin + (chartWidth * i) / 5;
        painter.drawLine(x, chartY + chartHeight, x, chartY + chartHeight + 5);
        painter.drawText(QRect(x - 30, chartY + chartHeight + 8, 60, 20),
                        Qt::AlignCenter, QString::number(frameNum));
    }

    painter.drawText(QRect(leftMargin, chartY + chartHeight + 30, chartWidth, 20),
                    Qt::AlignCenter, "帧号");

    // Draw legend
    int legendX = leftMargin + chartWidth - 150;
    int legendY = chartY + 10;
    painter.setPen(QPen(QColor(0, 100, 255), 2));
    painter.drawLine(legendX, legendY, legendX + 30, legendY);
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 9));
    painter.drawText(legendX + 35, legendY + 4, "PTS");

    painter.setPen(QPen(QColor(0, 200, 100), 2));
    painter.drawLine(legendX + 70, legendY, legendX + 100, legendY);
    painter.setPen(Qt::black);
    painter.drawText(legendX + 105, legendY + 4, "DTS");
}

void TimestampChart::drawDiffChart(QPainter& painter) {
    int chartWidth = width() - leftMargin - rightMargin;
    int chartY = topMargin + chartHeight + chartSpacing + 55;

    // Draw title
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(leftMargin, chartY + 15, "PTS - DTS 差值");

    chartY += 25;

    QVector<double> diffCurve = analyzer->getPTSDTSDiff();
    if (diffCurve.isEmpty()) return;

    double minVal = *std::min_element(diffCurve.begin(), diffCurve.end());
    double maxVal = *std::max_element(diffCurve.begin(), diffCurve.end());
    double range = maxVal - minVal;
    if (range == 0) range = 1;

    // Draw axes
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(leftMargin, chartY + chartHeight,
                     leftMargin + chartWidth, chartY + chartHeight);
    painter.drawLine(leftMargin, chartY, leftMargin, chartY + chartHeight);

    // Draw Y axis labels
    painter.setFont(QFont("Arial", 8));
    for (int i = 0; i <= 4; i++) {
        int64_t val = static_cast<int64_t>(minVal + (range * i) / 4);
        int y = chartY + chartHeight - (chartHeight * i) / 4;
        painter.drawLine(leftMargin - 5, y, leftMargin, y);
        painter.drawText(QRect(0, y - 10, leftMargin - 10, 20),
                        Qt::AlignRight | Qt::AlignVCenter,
                        QString::number(val));
    }

    // Y axis label
    painter.save();
    painter.translate(15, chartY + chartHeight / 2);
    painter.rotate(-90);
    painter.drawText(QRect(-40, -10, 80, 20), Qt::AlignCenter, "差值");
    painter.restore();

    // Draw diff curve
    painter.setPen(QPen(QColor(200, 100, 0), 2));
    for (int i = 1; i < diffCurve.size(); i++) {
        int x1 = leftMargin + (chartWidth * (i-1)) / diffCurve.size();
        int x2 = leftMargin + (chartWidth * i) / diffCurve.size();
        int y1 = chartY + chartHeight - (chartHeight * (diffCurve[i-1] - minVal)) / range;
        int y2 = chartY + chartHeight - (chartHeight * (diffCurve[i] - minVal)) / range;
        painter.drawLine(x1, y1, x2, y2);
    }

    // Draw X axis labels
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 8));
    for (int i = 0; i <= 5; i++) {
        int frameNum = (frameData.size() * i) / 5;
        int x = leftMargin + (chartWidth * i) / 5;
        painter.drawLine(x, chartY + chartHeight, x, chartY + chartHeight + 5);
        painter.drawText(QRect(x - 30, chartY + chartHeight + 8, 60, 20),
                        Qt::AlignCenter, QString::number(frameNum));
    }

    painter.drawText(QRect(leftMargin, chartY + chartHeight + 30, chartWidth, 20),
                    Qt::AlignCenter, "帧号");
}

void TimestampChart::drawIntervalChart(QPainter& painter) {
    int chartWidth = width() - leftMargin - rightMargin;
    int chartY = topMargin + (chartHeight + chartSpacing + 55) * 2;

    // Draw title
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(leftMargin, chartY + 15, "帧间隔");

    chartY += 25;

    QVector<double> intervals = analyzer->getFrameIntervals();
    if (intervals.isEmpty()) return;

    double minVal = *std::min_element(intervals.begin(), intervals.end());
    double maxVal = *std::max_element(intervals.begin(), intervals.end());
    double range = maxVal - minVal;
    if (range == 0) range = 1;

    // Draw axes
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(leftMargin, chartY + chartHeight,
                     leftMargin + chartWidth, chartY + chartHeight);
    painter.drawLine(leftMargin, chartY, leftMargin, chartY + chartHeight);

    // Draw Y axis labels
    painter.setFont(QFont("Arial", 8));
    for (int i = 0; i <= 4; i++) {
        double val = minVal + (range * i) / 4;
        int y = chartY + chartHeight - (chartHeight * i) / 4;
        painter.drawLine(leftMargin - 5, y, leftMargin, y);
        painter.drawText(QRect(0, y - 10, leftMargin - 10, 20),
                        Qt::AlignRight | Qt::AlignVCenter,
                        QString::number(val, 'f', 4));
    }

    // Y axis label
    painter.save();
    painter.translate(15, chartY + chartHeight / 2);
    painter.rotate(-90);
    painter.drawText(QRect(-40, -10, 80, 20), Qt::AlignCenter, "间隔 (秒)");
    painter.restore();

    // Draw interval bars
    painter.setPen(Qt::black);
    painter.setBrush(QColor(100, 150, 255));
    int barWidth = std::max(1, chartWidth / static_cast<int>(intervals.size()) - 1);
    for (int i = 0; i < intervals.size(); i++) {
        int x = leftMargin + (chartWidth * i) / intervals.size();
        int barHeight = (chartHeight * (intervals[i] - minVal)) / range;
        painter.drawRect(x, chartY + chartHeight - barHeight, barWidth, barHeight);
    }

    // Draw average line
    TimestampStats stats = analyzer->getStats();
    if (stats.avgFrameInterval > 0) {
        int avgY = chartY + chartHeight - (chartHeight * (stats.avgFrameInterval - minVal)) / range;
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter.drawLine(leftMargin, avgY, leftMargin + chartWidth, avgY);

        painter.setFont(QFont("Arial", 8));
        painter.drawText(leftMargin + chartWidth + 5, avgY + 4,
                        QString("平均: %1s").arg(stats.avgFrameInterval, 0, 'f', 4));
    }

    // Draw X axis labels
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 8));
    for (int i = 0; i <= 5; i++) {
        int frameNum = (frameData.size() * i) / 5;
        int x = leftMargin + (chartWidth * i) / 5;
        painter.drawLine(x, chartY + chartHeight, x, chartY + chartHeight + 5);
        painter.drawText(QRect(x - 30, chartY + chartHeight + 8, 60, 20),
                        Qt::AlignCenter, QString::number(frameNum));
    }

    painter.drawText(QRect(leftMargin, chartY + chartHeight + 30, chartWidth, 20),
                    Qt::AlignCenter, "帧号");
}

void TimestampChart::drawStatistics(QPainter& painter) {
    TimestampStats stats = analyzer->getStats();

    int statsX = width() - rightMargin - 250;
    int statsY = topMargin + 50;

    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 9));

    QStringList statsList;
    statsList << "时间戳统计:";
    statsList << QString("平均帧间隔: %1 秒").arg(stats.avgFrameInterval, 0, 'f', 4);
    statsList << QString("最小帧间隔: %1 秒").arg(stats.minFrameInterval, 0, 'f', 4);
    statsList << QString("最大帧间隔: %1 秒").arg(stats.maxFrameInterval, 0, 'f', 4);
    statsList << QString("抖动 (标准差): %1 秒").arg(stats.jitter, 0, 'f', 4);
    statsList << "";
    statsList << "异常检测:";
    statsList << QString("不连续点: %1").arg(stats.discontinuities);
    statsList << QString("PTS 倒序: %1").arg(stats.ptsReversals);
    statsList << QString("DTS 倒序: %1").arg(stats.dtsReversals);
    statsList << QString("重复 PTS: %1").arg(stats.duplicatePTS);
    statsList << QString("重复 DTS: %1").arg(stats.duplicateDTS);

    int lineHeight = 16;
    for (int i = 0; i < statsList.size(); i++) {
        painter.drawText(statsX, statsY + i * lineHeight, statsList[i]);
    }
}

void TimestampChart::drawTooltip(QPainter& painter) {
    int chartWidth = width() - leftMargin - rightMargin;
    int chart1Y = topMargin + 25;
    int chart2Y = topMargin + chartHeight + chartSpacing + 55 + 25;
    int chart3Y = topMargin + (chartHeight + chartSpacing + 55) * 2 + 25;

    // Check which chart the mouse is over
    if (mousePos.x() >= leftMargin && mousePos.x() <= leftMargin + chartWidth && !frameData.isEmpty()) {
        int frameIndex = ((mousePos.x() - leftMargin) * frameData.size()) / chartWidth;
        frameIndex = std::max(0, std::min(frameIndex, static_cast<int>(frameData.size()) - 1));

        const FrameInfo& frame = frameData[frameIndex];

        QString tooltip;
        if (mousePos.y() >= chart1Y && mousePos.y() <= chart1Y + chartHeight) {
            tooltip = QString("帧 #%1\nPTS: %2\nDTS: %3\n类型: %4")
                .arg(frame.frameNumber)
                .arg(frame.pts)
                .arg(frame.dts)
                .arg(frame.frameType);
        } else if (mousePos.y() >= chart2Y && mousePos.y() <= chart2Y + chartHeight) {
            tooltip = QString("帧 #%1\nPTS-DTS: %2")
                .arg(frame.frameNumber)
                .arg(frame.pts - frame.dts);
        } else if (mousePos.y() >= chart3Y && mousePos.y() <= chart3Y + chartHeight && frameIndex > 0) {
            QVector<double> intervals = analyzer->getFrameIntervals();
            if (frameIndex - 1 < intervals.size()) {
                tooltip = QString("帧 #%1\n间隔: %2 秒")
                    .arg(frame.frameNumber)
                    .arg(intervals[frameIndex - 1], 0, 'f', 4);
            }
        }

        if (!tooltip.isEmpty()) {
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
