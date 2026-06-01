#ifndef THUMBNAILBAR_H
#define THUMBNAILBAR_H

#include <QWidget>
#include <QScrollArea>
#include <QVector>
#include <QPixmap>

namespace VideoStudio {

class VideoDecoder;
class FrameIndex;

class ThumbnailBar : public QWidget {
    Q_OBJECT

public:
    explicit ThumbnailBar(QWidget* parent = nullptr);
    ~ThumbnailBar();

    void setDecoder(VideoDecoder* decoder);
    void setCurrentFrame(int frameNumber);
    void generateThumbnails();
    void clear();

signals:
    void frameClicked(int frameNumber);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    int frameNumberAtPosition(int x) const;
    QColor getFrameTypeColor(int frameNumber) const;

    VideoDecoder* m_decoder;
    QVector<QPixmap> m_thumbnails;
    QVector<int> m_thumbnailFrameNumbers;  // Frame number for each thumbnail
    int m_currentFrame;
    int m_thumbnailWidth;
    int m_thumbnailHeight;
};

} // namespace VideoStudio

#endif // THUMBNAILBAR_H
