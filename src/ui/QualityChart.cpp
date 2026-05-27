#include "QualityChart.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFileDialog>
#include <QDir>
#include <cmath>

QualityChart::QualityChart(QWidget* parent)
    : QWidget(parent) {
    setupUI();
}

void QualityChart::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Control panel
    QWidget* controlPanel = new QWidget(this);
    QHBoxLayout* controlLayout = new QHBoxLayout(controlPanel);

    selectRefButton = new QPushButton("选择参考视频...", this);
    connect(selectRefButton, &QPushButton::clicked, this, &QualityChart::onSelectReferenceVideo);
    controlLayout->addWidget(selectRefButton);

    refVideoLabel = new QLabel("未选择参考视频", this);
    refVideoLabel->setStyleSheet("QLabel { color: #888; }");
    controlLayout->addWidget(refVideoLabel);

    controlLayout->addStretch();

    analyzeButton = new QPushButton("开始分析", this);
    analyzeButton->setEnabled(false);
    connect(analyzeButton, &QPushButton::clicked, this, &QualityChart::onAnalyze);
    controlLayout->addWidget(analyzeButton);

    mainLayout->addWidget(controlPanel);

    // Statistics box
    statsBox = new QGroupBox("质量统计", this);
    QVBoxLayout* statsLayout = new QVBoxLayout(statsBox);

    statsLabel = new QLabel("暂无数据", this);
    statsLabel->setStyleSheet("QLabel { font-family: monospace; }");
    statsLayout->addWidget(statsLabel);

    mainLayout->addWidget(statsBox);

    // Chart area
    mainLayout->addStretch(1);

    setMinimumHeight(400);
}

void QualityChart::onSelectReferenceVideo() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择参考视频",
        QDir::homePath(),
        "视频文件 (*.mp4 *.mkv *.avi *.mov *.flv *.wmv *.webm);;所有文件 (*.*)"
    );

    if (!filePath.isEmpty()) {
        referenceVideoPath = filePath;
        refVideoLabel->setText(QFileInfo(filePath).fileName());
        refVideoLabel->setStyleSheet("QLabel { color: #fff; }");
        analyzeButton->setEnabled(true);
        emit referenceVideoSelected(filePath);
    }
}

void QualityChart::onAnalyze() {
    emit analyzeRequested();
}

void QualityChart::setQualityData(const QVector<QualityMetrics>& data) {
    qualityData = data;
    update();
}

void QualityChart::setStats(const QualityStats& s) {
    stats = s;

    QString text = QString(
        "总帧数: %1\n"
        "\n"
        "PSNR:\n"
        "  平均值: %2 dB\n"
        "  最小值: %3 dB\n"
        "  最大值: %4 dB\n"
        "\n"
        "SSIM:\n"
        "  平均值: %5\n"
        "  最小值: %6\n"
        "  最大值: %7"
    ).arg(stats.totalFrames)
     .arg(stats.avgPSNR, 0, 'f', 2)
     .arg(stats.minPSNR, 0, 'f', 2)
     .arg(stats.maxPSNR, 0, 'f', 2)
     .arg(stats.avgSSIM, 0, 'f', 4)
     .arg(stats.minSSIM, 0, 'f', 4)
     .arg(stats.maxSSIM, 0, 'f', 4);

    statsLabel->setText(text);
    update();
}

void QualityChart::clear() {
    qualityData.clear();
    stats = QualityStats();
    statsLabel->setText("暂无数据");
    update();
}

void QualityChart::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);

    if (qualityData.isEmpty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawChart(painter);
}

void QualityChart::drawChart(QPainter& painter) {
    // Chart area (below stats box)
    int chartTop = statsBox->geometry().bottom() + 20;
    int chartHeight = height() - chartTop - 40;
    int chartLeft = 60;
    int chartWidth = width() - chartLeft - 40;

    if (chartHeight < 100 || chartWidth < 100) {
        return;
    }

    // Draw background
    painter.fillRect(chartLeft, chartTop, chartWidth, chartHeight, QColor(40, 40, 40));

    // Find min/max values
    double minPSNR = 100.0;
    double maxPSNR = 0.0;
    double minSSIM = 1.0;
    double maxSSIM = 0.0;

    for (const auto& qm : qualityData) {
        minPSNR = std::min(minPSNR, qm.psnr);
        maxPSNR = std::max(maxPSNR, qm.psnr);
        minSSIM = std::min(minSSIM, qm.ssim);
        maxSSIM = std::max(maxSSIM, qm.ssim);
    }

    // Add some padding to ranges
    double psnrRange = maxPSNR - minPSNR;
    minPSNR -= psnrRange * 0.1;
    maxPSNR += psnrRange * 0.1;

    double ssimRange = maxSSIM - minSSIM;
    minSSIM -= ssimRange * 0.1;
    maxSSIM += ssimRange * 0.1;

    // Clamp SSIM to [0, 1]
    minSSIM = std::max(0.0, minSSIM);
    maxSSIM = std::min(1.0, maxSSIM);

    // Draw axes
    painter.setPen(QPen(QColor(150, 150, 150), 1));
    painter.drawRect(chartLeft, chartTop, chartWidth, chartHeight);

    // Draw Y-axis labels (PSNR on left)
    painter.setPen(QColor(100, 200, 255));
    painter.setFont(QFont("Arial", 9));
    for (int i = 0; i <= 5; i++) {
        double value = minPSNR + (maxPSNR - minPSNR) * i / 5.0;
        int y = chartTop + chartHeight - (chartHeight * i / 5);
        painter.drawText(5, y + 5, QString::number(value, 'f', 1) + " dB");
        painter.setPen(QPen(QColor(60, 60, 60), 1, Qt::DashLine));
        painter.drawLine(chartLeft, y, chartLeft + chartWidth, y);
        painter.setPen(QColor(100, 200, 255));
    }

    // Draw secondary Y-axis labels (SSIM on right)
    painter.setPen(QColor(255, 200, 100));
    for (int i = 0; i <= 5; i++) {
        double value = minSSIM + (maxSSIM - minSSIM) * i / 5.0;
        int y = chartTop + chartHeight - (chartHeight * i / 5);
        painter.drawText(chartLeft + chartWidth + 5, y + 5, QString::number(value, 'f', 3));
    }

    // Draw X-axis labels (frame numbers)
    painter.setPen(QColor(150, 150, 150));
    int maxFrames = qualityData.size();
    for (int i = 0; i <= 5; i++) {
        int frame = maxFrames * i / 5;
        int x = chartLeft + (chartWidth * i / 5);
        painter.drawText(x - 20, chartTop + chartHeight + 20, QString::number(frame));
    }

    // Draw PSNR curve
    painter.setPen(QPen(QColor(100, 200, 255), 2));
    for (int i = 1; i < qualityData.size(); i++) {
        int x1 = chartLeft + (chartWidth * (i - 1) / maxFrames);
        int x2 = chartLeft + (chartWidth * i / maxFrames);

        double y1Norm = (qualityData[i - 1].psnr - minPSNR) / (maxPSNR - minPSNR);
        double y2Norm = (qualityData[i].psnr - minPSNR) / (maxPSNR - minPSNR);

        int y1 = chartTop + chartHeight - static_cast<int>(y1Norm * chartHeight);
        int y2 = chartTop + chartHeight - static_cast<int>(y2Norm * chartHeight);

        painter.drawLine(x1, y1, x2, y2);
    }

    // Draw SSIM curve
    painter.setPen(QPen(QColor(255, 200, 100), 2));
    for (int i = 1; i < qualityData.size(); i++) {
        int x1 = chartLeft + (chartWidth * (i - 1) / maxFrames);
        int x2 = chartLeft + (chartWidth * i / maxFrames);

        double y1Norm = (qualityData[i - 1].ssim - minSSIM) / (maxSSIM - minSSIM);
        double y2Norm = (qualityData[i].ssim - minSSIM) / (maxSSIM - minSSIM);

        int y1 = chartTop + chartHeight - static_cast<int>(y1Norm * chartHeight);
        int y2 = chartTop + chartHeight - static_cast<int>(y2Norm * chartHeight);

        painter.drawLine(x1, y1, x2, y2);
    }

    // Draw legend
    int legendX = chartLeft + 10;
    int legendY = chartTop + 10;

    painter.setPen(QPen(QColor(100, 200, 255), 2));
    painter.drawLine(legendX, legendY, legendX + 30, legendY);
    painter.setPen(QColor(200, 200, 200));
    painter.drawText(legendX + 35, legendY + 5, "PSNR (dB)");

    painter.setPen(QPen(QColor(255, 200, 100), 2));
    painter.drawLine(legendX, legendY + 20, legendX + 30, legendY + 20);
    painter.setPen(QColor(200, 200, 200));
    painter.drawText(legendX + 35, legendY + 25, "SSIM");

    // Draw axis labels
    painter.setPen(QColor(200, 200, 200));
    painter.drawText(chartLeft + chartWidth / 2 - 20, chartTop + chartHeight + 35, "帧号");

    painter.save();
    painter.translate(20, chartTop + chartHeight / 2);
    painter.rotate(-90);
    painter.drawText(-30, 0, "PSNR (dB)");
    painter.restore();

    painter.save();
    painter.translate(chartLeft + chartWidth + 35, chartTop + chartHeight / 2);
    painter.rotate(-90);
    painter.drawText(-15, 0, "SSIM");
    painter.restore();
}
