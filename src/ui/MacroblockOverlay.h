#ifndef MACROBLOCKOVERLAY_H
#define MACROBLOCKOVERLAY_H

#include <QWidget>
#include <QPainter>
#include <QVector>
#include "core/MacroblockAnalyzer.h"

class MacroblockOverlay : public QWidget {
    Q_OBJECT

public:
    explicit MacroblockOverlay(QWidget* parent = nullptr);

    void setMacroblocks(const QVector<MacroblockInfo>& mbs);
    void clear();

    void setShowBoundaries(bool show) { showBoundaries = show; update(); }
    void setShowMotionVectors(bool show) { showMotionVectors = show; update(); }
    void setShowQPHeatmap(bool show) { showQPHeatmap = show; update(); }
    void setShowSizes(bool show) { showSizes = show; update(); }
    void setShowExtendedParams(bool show) { showExtendedParams = show; update(); }

    bool isShowingBoundaries() const { return showBoundaries; }
    bool isShowingMotionVectors() const { return showMotionVectors; }
    bool isShowingQPHeatmap() const { return showQPHeatmap; }
    bool isShowingSizes() const { return showSizes; }
    bool isShowingExtendedParams() const { return showExtendedParams; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawBoundaries(QPainter& painter);
    void drawMotionVectors(QPainter& painter);
    void drawQPHeatmap(QPainter& painter);
    void drawSizes(QPainter& painter);
    void drawExtendedParams(QPainter& painter);

    QVector<MacroblockInfo> macroblocks;
    bool showBoundaries;
    bool showMotionVectors;
    bool showQPHeatmap;
    bool showSizes;
    bool showExtendedParams;
};

#endif // MACROBLOCKOVERLAY_H
