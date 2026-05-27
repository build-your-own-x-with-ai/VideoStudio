#include "BitrateChart.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

BitrateChart::BitrateChart(QWidget* parent)
    : QWidget(parent) {
    setupUI();
}

void BitrateChart::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 统计信息
    QGroupBox* statsGroup = new QGroupBox("比特率统计", this);
    QHBoxLayout* statsLayout = new QHBoxLayout(statsGroup);

    avgBitrateLabel = new QLabel("平均: 0 kbps");
    peakBitrateLabel = new QLabel("峰值: 0 kbps");
    minBitrateLabel = new QLabel("最小: 0 kbps");

    statsLayout->addWidget(avgBitrateLabel);
    statsLayout->addWidget(peakBitrateLabel);
    statsLayout->addWidget(minBitrateLabel);
    statsLayout->addStretch();

    mainLayout->addWidget(statsGroup);

    // 图表
    chart = new QChart();
    chart->setTitle("比特率曲线");
    chart->setAnimationOptions(QChart::NoAnimation);

    series = new QLineSeries();
    series->setName("比特率");
    chart->addSeries(series);

    axisX = new QValueAxis();
    axisX->setTitleText("时间 (秒)");
    axisX->setLabelFormat("%.1f");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    axisY = new QValueAxis();
    axisY->setTitleText("比特率 (kbps)");
    axisY->setLabelFormat("%.0f");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    mainLayout->addWidget(chartView, 1);
}

void BitrateChart::setBitrateData(const QVector<BitratePoint>& points, const BitrateStats& stats) {
    this->bitratePoints = points;
    this->stats = stats;
    updateChart();
    updateStats();
}

void BitrateChart::updateChart() {
    series->clear();

    if (bitratePoints.isEmpty()) {
        return;
    }

    double maxBitrate = 0.0;
    double maxTime = 0.0;

    for (const auto& point : bitratePoints) {
        series->append(point.timestamp, point.bitrate / 1000.0);
        maxBitrate = std::max(maxBitrate, point.bitrate / 1000.0);
        maxTime = std::max(maxTime, point.timestamp);
    }

    axisX->setRange(0, maxTime);
    axisY->setRange(0, maxBitrate * 1.1);
}

void BitrateChart::updateStats() {
    avgBitrateLabel->setText(QString("平均: %1").arg(formatBitrate(stats.averageBitrate)));
    peakBitrateLabel->setText(QString("峰值: %1").arg(formatBitrate(stats.peakBitrate)));
    minBitrateLabel->setText(QString("最小: %1").arg(formatBitrate(stats.minBitrate)));
}

QString BitrateChart::formatBitrate(double bitrate) {
    if (bitrate < 1000) {
        return QString("%1 bps").arg(bitrate, 0, 'f', 0);
    } else if (bitrate < 1000000) {
        return QString("%1 kbps").arg(bitrate / 1000.0, 0, 'f', 1);
    } else {
        return QString("%1 Mbps").arg(bitrate / 1000000.0, 0, 'f', 2);
    }
}

void BitrateChart::clear() {
    series->clear();
    bitratePoints.clear();
    stats = {0.0, 0.0, 0.0, 0.0, 0.0};
    avgBitrateLabel->setText("平均: 0 kbps");
    peakBitrateLabel->setText("峰值: 0 kbps");
    minBitrateLabel->setText("最小: 0 kbps");
}
