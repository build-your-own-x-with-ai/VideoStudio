#ifndef BITRATECHART_H
#define BITRATECHART_H

#include <QWidget>
#include <QChartView>
#include <QLineSeries>
#include <QChart>
#include <QValueAxis>
#include <QLabel>
#include "core/BitrateAnalyzer.h"

class BitrateChart : public QWidget {
    Q_OBJECT

public:
    explicit BitrateChart(QWidget* parent = nullptr);
    void setBitrateData(const QVector<BitratePoint>& points, const BitrateStats& stats);
    void clear();

private:
    void setupUI();
    void updateChart();
    void updateStats();
    QString formatBitrate(double bitrate);

    QChartView* chartView;
    QChart* chart;
    QLineSeries* series;
    QValueAxis* axisX;
    QValueAxis* axisY;

    QLabel* avgBitrateLabel;
    QLabel* peakBitrateLabel;
    QLabel* minBitrateLabel;

    QVector<BitratePoint> bitratePoints;
    BitrateStats stats;
};

#endif // BITRATECHART_H
