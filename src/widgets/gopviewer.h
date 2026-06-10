#ifndef GOPVIEWER_H
#define GOPVIEWER_H

#include <QWidget>
#include <QVector>
#include <QSet>
#include <QPixmap>
#include <QMap>

namespace VideoStudio {

class FrameIndex;
class VideoDecoder;

struct GOPInfo {
    int startFrame;
    int endFrame;
    int iFrameIndex;
    QVector<int> pFrames;
    QVector<int> bFrames;
};

class GOPViewer : public QWidget {
    Q_OBJECT

public:
    explicit GOPViewer(QWidget* parent = nullptr);
    ~GOPViewer();

    void setFrameIndex(const FrameIndex* frameIndex);
    void setVideoDecoder(VideoDecoder* decoder);
    void setCurrentFrame(int frameNumber);
    void setDuplicateFrames(const QSet<int>& duplicateFrames);
    void clear();
    void toggleDisplayMode();
    void toggleDependencyArrows();
    QSize sizeHint() const override;

signals:
    void frameClicked(int frameNumber);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void analyzeGOPStructure();
    void drawGOPStructure(QPainter& painter);
    void drawDependencyArrows(QPainter& painter);
    void drawFrame(QPainter& painter, int frameNumber, int x, int y, int width, int height);

    const FrameIndex* m_frameIndex;
    VideoDecoder* m_videoDecoder;
    QVector<GOPInfo> m_gops;
    QSet<int> m_duplicateFrames;
    QMap<int, QPixmap> m_thumbnailCache;
    int m_currentFrame;
    bool m_showThumbnails;
    bool m_showDependencies;

    // Layout parameters
    int m_frameWidth;
    int m_frameHeight;
    int m_horizontalSpacing;
    int m_verticalSpacing;
    int m_leftMargin;
    int m_topMargin;
};

} // namespace VideoStudio

#endif // GOPVIEWER_H
