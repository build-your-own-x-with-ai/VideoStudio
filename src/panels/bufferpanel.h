#ifndef BUFFERPANEL_H
#define BUFFERPANEL_H

#include <QWidget>
#include <QVector>
#include <QPair>

namespace VideoStudio {

class VideoDecoder;

class BufferPanel : public QWidget {
    Q_OBJECT

public:
    explicit BufferPanel(QWidget* parent = nullptr);
    ~BufferPanel();

    void setDecoder(VideoDecoder* decoder);
    void updateBufferData();
    void clear();

    enum class ScaleMode {
        Defined,    // Use defined buffer size from stream
        Actual      // Use actual maximum buffer occupancy
    };

    void setVerticalScaleMode(ScaleMode mode);
    void setHorizontalScale(double scale);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void drawChart(QPainter& painter);
    void drawGrid(QPainter& painter, const QRect& chartRect);
    void drawBufferCurve(QPainter& painter, const QRect& chartRect);
    void drawTooltip(QPainter& painter);

    int64_t estimateBufferSize() const;
    int64_t calculateBufferOccupancy(int frameIndex) const;

    VideoDecoder* m_decoder;

    // Buffer data: frame index -> buffer occupancy in bits
    QVector<QPair<int, int64_t>> m_bufferData;

    int64_t m_maxBufferSize;      // CPB size in bits
    int64_t m_maxOccupancy;       // Maximum observed occupancy

    ScaleMode m_scaleMode;
    double m_horizontalScale;

    // Mouse tracking
    QPoint m_mousePos;
    bool m_mouseInChart;
    int m_hoveredFrame;
};

} // namespace VideoStudio

#endif // BUFFERPANEL_H
