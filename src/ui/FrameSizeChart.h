#ifndef FRAMESIZECHART_H
#define FRAMESIZECHART_H

#include <QWidget>
#include <QPainter>
#include <QVector>
#include "core/MetricsCollector.h"

class FrameSizeChart : public QWidget {
    Q_OBJECT

public:
    explicit FrameSizeChart(QWidget* parent = nullptr);
    void setDistribution(const FrameSizeDistribution& dist);
    void setFrameData(const QVector<FrameInfo>& frames);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void drawTimelineView(QPainter& painter);
    void drawHistogram(QPainter& painter);
    void drawStatistics(QPainter& painter);
    void drawLegend(QPainter& painter);
    void drawTooltip(QPainter& painter);

    QString formatSize(int bytes);
    QString formatTime(double seconds);

    FrameSizeDistribution distribution;
    QVector<FrameInfo> frameData;
    bool hasData;

    // Layout
    int leftMargin;
    int rightMargin;
    int topMargin;
    int bottomMargin;
    int timelineHeight;
    int histogramHeight;
    int spacing;

    // Mouse interaction
    QPoint mousePos;
    bool mouseInWidget;
};

#endif // FRAMESIZECHART_H
