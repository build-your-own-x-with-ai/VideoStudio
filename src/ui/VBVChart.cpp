#include "VBVChart.h"
#include <QPaintEvent>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <algorithm>

VBVChart::VBVChart(QWidget* parent)
    : QWidget(parent), analyzer(nullptr), hasData(false),
      leftMargin(80), rightMargin(40), topMargin(100), bottomMargin(60),
      chartHeight(300), controlPanelHeight(80), mouseInWidget(false) {
    analyzer = new VBVAnalyzer();
    setMinimumSize(800, 500);
    setStyleSheet("background-color: white;");
    setMouseTracking(true);
    setupControls();
}

void VBVChart::setupControls() {
    // Create control panel
    QGroupBox* controlGroup = new QGroupBox("VBV 缓冲区参数", this);
    controlGroup->setGeometry(leftMargin, 10, width() - leftMargin - rightMargin, controlPanelHeight);

    QHBoxLayout* layout = new QHBoxLayout(controlGroup);

    // Buffer size control
    bufferSizeLabel = new QLabel("缓冲区大小 (bits):", this);
    bufferSizeEdit = new QLineEdit("1000000", this);  // Default 1 Mbit
    bufferSizeEdit->setFixedWidth(120);

    // Bitrate control
    bitrateLabel = new QLabel("目标比特率 (bps):", this);
    bitrateEdit = new QLineEdit("500000", this);  // Default 500 kbps
    bitrateEdit->setFixedWidth(120);

    // Initial delay control
    delayLabel = new QLabel("初始延迟 (秒):", this);
    delayEdit = new QLineEdit("0.5", this);  // Default 0.5 seconds
    delayEdit->setFixedWidth(80);

    // Analyze button
    analyzeButton = new QPushButton("分析", this);
    analyzeButton->setFixedWidth(80);
    connect(analyzeButton, &QPushButton::clicked, this, &VBVChart::onAnalyzeClicked);

    layout->addWidget(bufferSizeLabel);
    layout->addWidget(bufferSizeEdit);
    layout->addWidget(bitrateLabel);
    layout->addWidget(bitrateEdit);
    layout->addWidget(delayLabel);
    layout->addWidget(delayEdit);
    layout->addWidget(analyzeButton);
    layout->addStretch();
}

void VBVChart::setFrameData(const QVector<FrameInfo>& frames) {
    frameData = frames;
    hasData = !frames.isEmpty();

    if (hasData) {
        // Auto-calculate default parameters from video
        // Calculate average bitrate from frame data
        if (frames.size() > 1) {
            double totalBits = 0;
            for (const auto& frame : frames) {
                totalBits += frame.size * 8.0;
            }
            double duration = frames.last().timestamp - frames.first().timestamp;
            if (duration > 0) {
                double avgBitrate = totalBits / duration;
                bitrateEdit->setText(QString::number(static_cast<int>(avgBitrate)));

                // Set buffer size to 2 seconds of video at average bitrate
                int bufferSize = static_cast<int>(avgBitrate * 2.0);
                bufferSizeEdit->setText(QString::number(bufferSize));
            }
        }

        // Auto-analyze with default parameters
        onAnalyzeClicked();
    }
    update();
}

void VBVChart::clear() {
    frameData.clear();
    analyzer->clear();
    hasData = false;
    update();
}

void VBVChart::onAnalyzeClicked() {
    if (!hasData) return;

    bool ok;
    double bufferSize = bufferSizeEdit->text().toDouble(&ok);
    if (!ok || bufferSize <= 0) {
        bufferSize = 1000000;  // Default
    }

    double bitrate = bitrateEdit->text().toDouble(&ok);
    if (!ok || bitrate <= 0) {
        bitrate = 500000;  // Default
    }

    double delay = delayEdit->text().toDouble(&ok);
    if (!ok || delay < 0) {
        delay = 0.5;  // Default
    }

    analyzer->analyze(frameData, bufferSize, bitrate, delay);
    update();
}

void VBVChart::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!hasData) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, "暂无数据");
        return;
    }

    drawOccupancyChart(painter);
    drawStatistics(painter);

    if (mouseInWidget) {
        drawTooltip(painter);
    }
}

void VBVChart::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    // Reposition control panel
    QGroupBox* controlGroup = findChild<QGroupBox*>();
    if (controlGroup) {
        controlGroup->setGeometry(leftMargin, 10, width() - leftMargin - rightMargin, controlPanelHeight);
    }
}

void VBVChart::mouseMoveEvent(QMouseEvent* event) {
    mousePos = event->pos();
    mouseInWidget = true;
    update();
}

void VBVChart::leaveEvent(QEvent* event) {
    mouseInWidget = false;
    update();
}

void VBVChart::drawOccupancyChart(QPainter& painter) {
    int chartWidth = width() - leftMargin - rightMargin;
    int chartY = topMargin;

    // Draw title
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(leftMargin, chartY + 15, "VBV 缓冲区占用率");

    chartY += 25;

    QVector<VBVPoint> curve = analyzer->getOccupancyCurve();
    if (curve.isEmpty()) return;

    // Y axis range: 0-100%
    double minVal = 0.0;
    double maxVal = 100.0;

    // Draw axes
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(leftMargin, chartY + chartHeight,
                     leftMargin + chartWidth, chartY + chartHeight);
    painter.drawLine(leftMargin, chartY, leftMargin, chartY + chartHeight);

    // Draw Y axis labels (percentage)
    painter.setFont(QFont("Arial", 8));
    for (int i = 0; i <= 10; i++) {
        int percent = i * 10;
        int y = chartY + chartHeight - (chartHeight * i) / 10;
        painter.drawLine(leftMargin - 5, y, leftMargin, y);
        painter.drawText(QRect(0, y - 10, leftMargin - 10, 20),
                        Qt::AlignRight | Qt::AlignVCenter,
                        QString("%1%").arg(percent));
    }

    // Y axis label
    painter.save();
    painter.translate(15, chartY + chartHeight / 2);
    painter.rotate(-90);
    painter.drawText(QRect(-50, -10, 100, 20), Qt::AlignCenter, "占用率 (%)");
    painter.restore();

    // Draw safe zone (green background, 20%-80%)
    painter.fillRect(leftMargin, chartY + chartHeight * 0.2,
                     chartWidth, chartHeight * 0.6,
                     QColor(200, 255, 200, 100));

    // Draw danger zones (red background, 0-20% and 80-100%)
    painter.fillRect(leftMargin, chartY,
                     chartWidth, chartHeight * 0.2,
                     QColor(255, 200, 200, 100));
    painter.fillRect(leftMargin, chartY + chartHeight * 0.8,
                     chartWidth, chartHeight * 0.2,
                     QColor(255, 200, 200, 100));

    // Draw occupancy curve
    painter.setPen(QPen(QColor(0, 100, 255), 2));
    for (int i = 1; i < curve.size(); i++) {
        int x1 = leftMargin + (chartWidth * (i-1)) / curve.size();
        int x2 = leftMargin + (chartWidth * i) / curve.size();
        int y1 = chartY + chartHeight - (chartHeight * curve[i-1].occupancy) / 100.0;
        int y2 = chartY + chartHeight - (chartHeight * curve[i].occupancy) / 100.0;
        painter.drawLine(x1, y1, x2, y2);
    }

    // Mark overflow/underflow points
    painter.setPen(QPen(Qt::red, 8));
    for (int i = 0; i < curve.size(); i++) {
        if (curve[i].isOverflow || curve[i].isUnderflow) {
            int x = leftMargin + (chartWidth * i) / curve.size();
            int y = chartY + chartHeight - (chartHeight * curve[i].occupancy) / 100.0;
            painter.drawPoint(x, y);
        }
    }

    // Draw reference lines
    VBVStats stats = analyzer->getStats();
    if (stats.avgOccupancy > 0) {
        int avgY = chartY + chartHeight - (chartHeight * stats.avgOccupancy) / 100.0;
        painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
        painter.drawLine(leftMargin, avgY, leftMargin + chartWidth, avgY);

        painter.setFont(QFont("Arial", 8));
        painter.drawText(leftMargin + chartWidth + 5, avgY + 4,
                        QString("平均: %1%").arg(stats.avgOccupancy, 0, 'f', 1));
    }

    // Draw X axis labels (time)
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 8));
    for (int i = 0; i <= 5; i++) {
        int frameIdx = (curve.size() * i) / 5;
        if (frameIdx >= curve.size()) frameIdx = curve.size() - 1;
        int x = leftMargin + (chartWidth * i) / 5;
        painter.drawLine(x, chartY + chartHeight, x, chartY + chartHeight + 5);
        painter.drawText(QRect(x - 40, chartY + chartHeight + 8, 80, 20),
                        Qt::AlignCenter,
                        QString("%1s").arg(curve[frameIdx].timestamp, 0, 'f', 2));
    }

    painter.drawText(QRect(leftMargin, chartY + chartHeight + 30, chartWidth, 20),
                    Qt::AlignCenter, "时间");
}

void VBVChart::drawStatistics(QPainter& painter) {
    VBVStats stats = analyzer->getStats();

    int statsX = leftMargin + 10;
    int statsY = topMargin + chartHeight + 80;

    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 9));

    QStringList statsList;
    statsList << "VBV 缓冲区统计:";
    statsList << QString("缓冲区大小: %1 bits (%2 KB)")
        .arg(static_cast<int>(stats.bufferSize))
        .arg(static_cast<int>(stats.bufferSize / 8192.0));
    statsList << QString("目标比特率: %1 bps (%2 kbps)")
        .arg(static_cast<int>(stats.targetBitrate))
        .arg(static_cast<int>(stats.targetBitrate / 1000.0));
    statsList << QString("初始延迟: %1 秒").arg(stats.initialDelay, 0, 'f', 2);
    statsList << "";
    statsList << QString("最大占用率: %1%").arg(stats.maxOccupancy, 0, 'f', 2);
    statsList << QString("最小占用率: %1%").arg(stats.minOccupancy, 0, 'f', 2);
    statsList << QString("平均占用率: %1%").arg(stats.avgOccupancy, 0, 'f', 2);
    statsList << "";
    statsList << QString("上溢次数: %1").arg(stats.overflows);
    statsList << QString("下溢次数: %1").arg(stats.underflows);

    int lineHeight = 16;
    for (int i = 0; i < statsList.size(); i++) {
        painter.drawText(statsX, statsY + i * lineHeight, statsList[i]);
    }

    // Draw legend
    int legendX = width() - rightMargin - 200;
    int legendY = statsY;

    painter.fillRect(legendX, legendY, 20, 12, QColor(200, 255, 200, 100));
    painter.drawRect(legendX, legendY, 20, 12);
    painter.drawText(legendX + 25, legendY + 10, "安全区域 (20%-80%)");

    painter.fillRect(legendX, legendY + 20, 20, 12, QColor(255, 200, 200, 100));
    painter.drawRect(legendX, legendY + 20, 20, 12);
    painter.drawText(legendX + 25, legendY + 30, "危险区域 (<20% 或 >80%)");
}

void VBVChart::drawTooltip(QPainter& painter) {
    int chartWidth = width() - leftMargin - rightMargin;
    int chartY = topMargin + 25;

    if (mousePos.x() >= leftMargin && mousePos.x() <= leftMargin + chartWidth &&
        mousePos.y() >= chartY && mousePos.y() <= chartY + chartHeight) {

        QVector<VBVPoint> curve = analyzer->getOccupancyCurve();
        if (!curve.isEmpty()) {
            int pointIndex = ((mousePos.x() - leftMargin) * curve.size()) / chartWidth;
            pointIndex = std::max(0, std::min(pointIndex, static_cast<int>(curve.size()) - 1));

            const VBVPoint& point = curve[pointIndex];

            QString status = "正常";
            if (point.isOverflow) status = "上溢";
            if (point.isUnderflow) status = "下溢";

            QString tooltip = QString("帧 #%1\n时间: %2 秒\n占用率: %3%\n状态: %4")
                .arg(point.frameNumber)
                .arg(point.timestamp, 0, 'f', 3)
                .arg(point.occupancy, 0, 'f', 2)
                .arg(status);

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
