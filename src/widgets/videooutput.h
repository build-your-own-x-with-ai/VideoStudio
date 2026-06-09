#ifndef VIDEOOUTPUT_H
#define VIDEOOUTPUT_H

#include <QWidget>
#include <QImage>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace VideoStudio {

enum class OverlayType {
    None = 0,
    SliceBoundaries = 1,
    Partitions = 2,
    MotionVectors = 3,
    FrameTypes = 4,
    BlockSizes = 5,
    ExtendedInfo = 6,
    QuantizationParameter = 7  // QP Heatmap
};

class VideoOutput : public QWidget {
    Q_OBJECT

public:
    explicit VideoOutput(QWidget* parent = nullptr);
    ~VideoOutput();

    void displayFrame(AVFrame* frame);
    void clear();

    void setOverlay(OverlayType type, bool enabled);
    bool isOverlayEnabled(OverlayType type) const;
    void toggleOverlay(OverlayType type);

    void zoomIn();
    void zoomOut();
    void zoomFit();
    double getZoomLevel() const { return m_zoomLevel; }

    // Get current rendered image (without black borders)
    QImage getCurrentImage() const { return m_image; }

    // Cursor mode for block inspection
    void setCursorMode(bool enabled);
    bool isCursorModeEnabled() const { return m_cursorModeEnabled; }

    // Block grid mode: true = standard grid, false = actual block boundaries
    void setStandardGridMode(bool enabled);
    bool isStandardGridMode() const { return m_standardGridMode; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void convertFrameToImage(AVFrame* frame);
    void drawOverlays(QPainter& painter, const QRect& videoRect);
    void drawMotionVectors(QPainter& painter, const QRect& videoRect);
    void drawBlockBoundaries(QPainter& painter, const QRect& videoRect);
    void drawFrameTypeInfo(QPainter& painter, const QRect& videoRect);
    void drawQPHeatmap(QPainter& painter, const QRect& videoRect);
    void drawCursorInfo(QPainter& painter, const QRect& videoRect);
    QString getBlockInfoAtPosition(const QPoint& pos, const QRect& videoRect);
    QColor getQPColor(int qp) const;

    QImage m_image;
    SwsContext* m_swsContext;
    int m_lastWidth;
    int m_lastHeight;

    AVFrame* m_currentFrame;
    int m_overlayFlags;
    double m_zoomLevel;

    // Cursor mode
    bool m_cursorModeEnabled;
    QPoint m_cursorPos;
    bool m_hasCursorPos;

    // Block grid mode (true = standard grid, false = actual block boundaries)
    bool m_standardGridMode;
};

} // namespace VideoStudio

#endif // VIDEOOUTPUT_H
