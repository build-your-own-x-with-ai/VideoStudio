#include "widgets/videooutput.h"
#include <QPainter>
#include <QDebug>

extern "C" {
#include <libavutil/imgutils.h>
}

namespace VideoStudio {

VideoOutput::VideoOutput(QWidget* parent)
    : QWidget(parent)
    , m_swsContext(nullptr)
    , m_lastWidth(0)
    , m_lastHeight(0)
{
    setMinimumSize(320, 240);
    setStyleSheet("background-color: black;");
}

VideoOutput::~VideoOutput() {
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
    }
}

void VideoOutput::displayFrame(AVFrame* frame) {
    if (!frame) {
        return;
    }

    convertFrameToImage(frame);
    update();
}

void VideoOutput::clear() {
    m_image = QImage();
    update();
}

void VideoOutput::convertFrameToImage(AVFrame* frame) {
    if (!frame || frame->width <= 0 || frame->height <= 0) {
        return;
    }

    // Reinitialize swscale context if dimensions changed
    if (frame->width != m_lastWidth || frame->height != m_lastHeight || !m_swsContext) {
        if (m_swsContext) {
            sws_freeContext(m_swsContext);
        }

        m_swsContext = sws_getContext(
            frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
            frame->width, frame->height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );

        m_lastWidth = frame->width;
        m_lastHeight = frame->height;
    }

    if (!m_swsContext) {
        qWarning() << "Failed to create swscale context";
        return;
    }

    // Allocate RGB buffer
    int rgbLinesize = frame->width * 3;
    QByteArray rgbBuffer(frame->height * rgbLinesize, 0);
    uint8_t* rgbData = reinterpret_cast<uint8_t*>(rgbBuffer.data());

    uint8_t* dstData[1] = { rgbData };
    int dstLinesize[1] = { rgbLinesize };

    // Convert YUV to RGB
    sws_scale(m_swsContext,
              frame->data, frame->linesize, 0, frame->height,
              dstData, dstLinesize);

    // Create QImage from RGB data
    m_image = QImage(rgbData, frame->width, frame->height, rgbLinesize, QImage::Format_RGB888).copy();
}

void VideoOutput::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (!m_image.isNull()) {
        // Scale image to fit widget while maintaining aspect ratio
        QSize scaledSize = m_image.size();
        scaledSize.scale(size(), Qt::KeepAspectRatio);

        QRect targetRect(
            (width() - scaledSize.width()) / 2,
            (height() - scaledSize.height()) / 2,
            scaledSize.width(),
            scaledSize.height()
        );

        painter.drawImage(targetRect, m_image);
    }
}

void VideoOutput::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

} // namespace VideoStudio
