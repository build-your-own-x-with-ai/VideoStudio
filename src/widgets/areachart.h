#ifndef AREACHART_H
#define AREACHART_H

#include <QWidget>
#include <QVector>
#include <QMap>

namespace VideoStudio {

class FrameIndex;

struct DataSeries {
    QString name;
    QColor color;
    QVector<double> values;
    bool visible;
};

class AreaChart : public QWidget {
    Q_OBJECT

public:
    explicit AreaChart(QWidget* parent = nullptr);
    ~AreaChart();

    void setFrameIndex(const FrameIndex* frameIndex);
    void addSeries(const QString& name, const QColor& color);
    void setSeriesVisible(const QString& name, bool visible);
    void clear();

    void setXRange(int minFrame, int maxFrame);
    void setYRange(double minValue, double maxValue);
    void setAutoScale(bool enabled);

    void zoomIn();
    void zoomOut();
    void zoomFit();

signals:
    void rangeChanged(int minFrame, int maxFrame);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateData();
    void drawGrid(QPainter& painter);
    void drawAxes(QPainter& painter);
    void drawSeries(QPainter& painter);
    void drawLegend(QPainter& painter);

    QPoint frameToPixel(int frame, double value) const;
    int pixelToFrame(int x) const;

    const FrameIndex* m_frameIndex;
    QMap<QString, DataSeries> m_series;

    int m_minFrame;
    int m_maxFrame;
    double m_minValue;
    double m_maxValue;
    bool m_autoScale;

    // Interaction state
    bool m_isDragging;
    QPoint m_dragStart;
    int m_dragStartMinFrame;
    int m_dragStartMaxFrame;

    // Layout
    int m_leftMargin;
    int m_rightMargin;
    int m_topMargin;
    int m_bottomMargin;
};

} // namespace VideoStudio

#endif // AREACHART_H
