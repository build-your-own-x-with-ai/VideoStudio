#ifndef BARCHART_H
#define BARCHART_H

#include <QWidget>
#include "core/framedata.h"

namespace VideoStudio {

class BarChart : public QWidget {
    Q_OBJECT

public:
    explicit BarChart(QWidget* parent = nullptr);
    ~BarChart();

    void setFrameIndex(const FrameIndex* frameIndex);
    void setCurrentFrame(int frameNumber);

    void zoomIn();
    void zoomOut();
    void zoomFit();

signals:
    void frameClicked(int frameNumber);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QColor getFrameTypeColor(AVPictureType type) const;
    int frameNumberAtPosition(int x) const;

    const FrameIndex* m_frameIndex;
    int m_currentFrame;
    int m_maxFrameSize;

    // Zoom/scroll state
    int m_viewStartFrame;
    int m_viewEndFrame;
    double m_zoomLevel;
};

} // namespace VideoStudio

#endif // BARCHART_H
