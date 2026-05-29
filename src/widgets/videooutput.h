#ifndef VIDEOOUTPUT_H
#define VIDEOOUTPUT_H

#include <QWidget>
#include <QImage>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace VideoStudio {

class VideoOutput : public QWidget {
    Q_OBJECT

public:
    explicit VideoOutput(QWidget* parent = nullptr);
    ~VideoOutput();

    void displayFrame(AVFrame* frame);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void convertFrameToImage(AVFrame* frame);

    QImage m_image;
    SwsContext* m_swsContext;
    int m_lastWidth;
    int m_lastHeight;
};

} // namespace VideoStudio

#endif // VIDEOOUTPUT_H
