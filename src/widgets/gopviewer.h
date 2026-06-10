#ifndef GOPVIEWER_H
#define GOPVIEWER_H

#include <QWidget>
#include <QVector>
#include <QSet>

namespace VideoStudio {

class FrameIndex;

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
    void setCurrentFrame(int frameNumber);
    void setDuplicateFrames(const QSet<int>& duplicateFrames);
    void clear();
    void toggleDisplayMode();
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
    void drawFrame(QPainter& painter, int frameNumber, int x, int y, int width, int height);

    const FrameIndex* m_frameIndex;
    QVector<GOPInfo> m_gops;
    QSet<int> m_duplicateFrames;
    int m_currentFrame;
    bool m_showThumbnails;

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
