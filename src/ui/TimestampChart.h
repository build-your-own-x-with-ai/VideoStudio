#ifndef TIMESTAMPCHART_H
#define TIMESTAMPCHART_H

#include <QWidget>
#include <QPainter>
#include <QVector>
#include "core/FrameInfo.h"
#include "core/TimestampAnalyzer.h"

class TimestampChart : public QWidget {
    Q_OBJECT

public:
    explicit TimestampChart(QWidget* parent = nullptr);

    void setFrameData(const QVector<FrameInfo>& frames);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void drawPTSDTSChart(QPainter& painter);
    void drawDiffChart(QPainter& painter);
    void drawIntervalChart(QPainter& painter);
    void drawStatistics(QPainter& painter);
    void drawTooltip(QPainter& painter);

    TimestampAnalyzer* analyzer;
    QVector<FrameInfo> frameData;
    bool hasData;

    // Layout parameters
    int leftMargin;
    int rightMargin;
    int topMargin;
    int bottomMargin;
    int chartHeight;
    int chartSpacing;

    // Mouse tracking
    QPoint mousePos;
    bool mouseInWidget;
};

#endif // TIMESTAMPCHART_H
