#ifndef QPCHART_H
#define QPCHART_H

#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QMap>
#include "core/FrameInfo.h"

struct QPStatistics {
    int minQP;
    int maxQP;
    double avgQP;
    double medianQP;
    double stdDevQP;
    QMap<char, double> avgQPByType;  // Average QP by frame type
    QMap<int, int> qpDistribution;   // QP value -> count
    int totalFrames;
    int framesWithQP;

    QPStatistics() : minQP(0), maxQP(0), avgQP(0.0), medianQP(0.0),
                     stdDevQP(0.0), totalFrames(0), framesWithQP(0) {}
};

class QPChart : public QWidget {
    Q_OBJECT

public:
    explicit QPChart(QWidget* parent = nullptr);
    void setFrameData(const QVector<FrameInfo>& frames);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void calculateStatistics();
    void drawTimelineView(QPainter& painter);
    void drawDistributionChart(QPainter& painter);
    void drawStatistics(QPainter& painter);
    void drawLegend(QPainter& painter);
    void drawTooltip(QPainter& painter);

    QString formatQP(int qp);

    QVector<FrameInfo> frameData;
    QPStatistics stats;
    bool hasData;

    // Layout
    int leftMargin;
    int rightMargin;
    int topMargin;
    int bottomMargin;
    int timelineHeight;
    int distributionHeight;
    int spacing;

    // Mouse interaction
    QPoint mousePos;
    bool mouseInWidget;
};

#endif // QPCHART_H
