#include "widgets/thumbnailbar.h"
#include "core/videodecoder.h"
#include "core/framedata.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace VideoStudio {

ThumbnailBar::ThumbnailBar(QWidget* parent)
    : QWidget(parent)
    , m_decoder(nullptr)
    , m_currentFrame(0)
    , m_thumbnailWidth(80)
    , m_thumbnailHeight(60)
{
    setMinimumHeight(m_thumbnailHeight + 20);
    setMaximumHeight(m_thumbnailHeight + 20);
}

ThumbnailBar::~ThumbnailBar() {
}

void ThumbnailBar::setDecoder(VideoDecoder* decoder) {
    m_decoder = decoder;
    clear();
}

void ThumbnailBar::setCurrentFrame(int frameNumber) {
    m_currentFrame = frameNumber;
    update();
}

void ThumbnailBar::generateThumbnails() {
    if (!m_decoder || !m_decoder->isOpen()) {
        return;
    }

    clear();

    int frameCount = m_decoder->getFrameCount();
    const FrameIndex& frameIndex = m_decoder->getFrameIndex();

    // Collect all keyframe positions
    QVector<int> keyframePositions;
    for (int i = 0; i < frameCount; i++) {
        const FrameInfo* frame = frameIndex.getFrame(i);
        if (frame && frame->isKeyFrame) {
            keyframePositions.append(i);
        }
    }

    qDebug() << "Found" << keyframePositions.size() << "keyframes in" << frameCount << "frames";

    // If too few keyframes, fall back to evenly spaced sampling (max 30 thumbnails)
    QVector<int> thumbnailPositions;
    if (keyframePositions.size() < 5) {
        int step = qMax(1, frameCount / 30);
        for (int i = 0; i < frameCount; i += step) {
            thumbnailPositions.append(i);
        }
        qDebug() << "Using evenly spaced sampling with step:" << step;
    } else {
        // Use keyframes, but limit to max 30 to avoid UI clutter
        int step = qMax(1, keyframePositions.size() / 30);
        for (int i = 0; i < keyframePositions.size(); i += step) {
            thumbnailPositions.append(keyframePositions[i]);
        }
        qDebug() << "Using keyframe-based sampling, selected" << thumbnailPositions.size() << "keyframes";
    }

    // Save current position
    int originalFrame = m_decoder->getCurrentFrameNumber();

    // Generate thumbnails at selected positions
    for (int frameNum : thumbnailPositions) {
        if (!m_decoder->seekToFrame(frameNum)) {
            qDebug() << "Failed to seek to frame" << frameNum;
            continue;
        }

        AVFrame* frame = m_decoder->decodeNextFrame();
        if (frame) {
            // Convert frame to thumbnail
            SwsContext* swsContext = sws_getContext(
                frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                m_thumbnailWidth, m_thumbnailHeight, AV_PIX_FMT_RGB24,
                SWS_BILINEAR, nullptr, nullptr, nullptr
            );

            if (swsContext) {
                int rgbLinesize = m_thumbnailWidth * 3;
                QByteArray rgbBuffer(m_thumbnailHeight * rgbLinesize, 0);
                uint8_t* rgbData = reinterpret_cast<uint8_t*>(rgbBuffer.data());

                uint8_t* dstData[1] = { rgbData };
                int dstLinesize[1] = { rgbLinesize };

                sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height,
                         dstData, dstLinesize);

                QImage image(rgbData, m_thumbnailWidth, m_thumbnailHeight, rgbLinesize,
                           QImage::Format_RGB888);
                m_thumbnails.append(QPixmap::fromImage(image.copy()));
                m_thumbnailFrameNumbers.append(frameNum);

                sws_freeContext(swsContext);
            }
        }
    }

    // Restore original position
    if (originalFrame > 0) {
        m_decoder->seekToFrame(originalFrame);
    }

    qDebug() << "Generated" << m_thumbnails.size() << "thumbnails";
    update();
}

void ThumbnailBar::clear() {
    m_thumbnails.clear();
    m_thumbnailFrameNumbers.clear();
    m_currentFrame = 0;
    update();
}

void ThumbnailBar::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(50, 50, 50));

    if (!m_decoder || m_thumbnails.isEmpty()) {
        return;
    }

    // Draw thumbnails
    int x = 5;
    for (int i = 0; i < m_thumbnails.size(); ++i) {
        int frameNumber = m_thumbnailFrameNumbers[i];

        // Draw thumbnail
        painter.drawPixmap(x, 10, m_thumbnails[i]);

        // Draw frame type indicator
        QColor color = getFrameTypeColor(frameNumber);
        painter.fillRect(x, height() - 8, m_thumbnailWidth, 3, color);

        // Highlight current frame - check if current frame is closest to this thumbnail
        bool isClosest = false;
        if (i == 0) {
            // First thumbnail: highlight if current frame is before next thumbnail
            isClosest = (i == m_thumbnails.size() - 1) ||
                       (m_currentFrame < (m_thumbnailFrameNumbers[i] + m_thumbnailFrameNumbers[i + 1]) / 2);
        } else if (i == m_thumbnails.size() - 1) {
            // Last thumbnail: highlight if current frame is after previous thumbnail midpoint
            isClosest = m_currentFrame >= (m_thumbnailFrameNumbers[i - 1] + m_thumbnailFrameNumbers[i]) / 2;
        } else {
            // Middle thumbnails: highlight if current frame is in range
            int prevMid = (m_thumbnailFrameNumbers[i - 1] + m_thumbnailFrameNumbers[i]) / 2;
            int nextMid = (m_thumbnailFrameNumbers[i] + m_thumbnailFrameNumbers[i + 1]) / 2;
            isClosest = m_currentFrame >= prevMid && m_currentFrame < nextMid;
        }

        if (isClosest) {
            painter.setPen(QPen(Qt::yellow, 2));
            painter.drawRect(x - 1, 9, m_thumbnailWidth + 2, m_thumbnailHeight + 2);
        }

        // Draw frame number
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 8));
        painter.drawText(x, height() - 10, QString::number(frameNumber));

        x += m_thumbnailWidth + 5;
    }
}

void ThumbnailBar::mousePressEvent(QMouseEvent* event) {
    int frameNumber = frameNumberAtPosition(event->pos().x());
    if (frameNumber >= 0) {
        emit frameClicked(frameNumber);
    }
}

void ThumbnailBar::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

int ThumbnailBar::frameNumberAtPosition(int x) const {
    if (!m_decoder || m_thumbnails.isEmpty()) {
        return -1;
    }

    int thumbnailIndex = (x - 5) / (m_thumbnailWidth + 5);
    if (thumbnailIndex >= 0 && thumbnailIndex < m_thumbnails.size()) {
        return m_thumbnailFrameNumbers[thumbnailIndex];
    }

    return -1;
}

QColor ThumbnailBar::getFrameTypeColor(int frameNumber) const {
    if (!m_decoder) {
        return Qt::gray;
    }

    const FrameInfo* frame = m_decoder->getFrameIndex().getFrame(frameNumber);
    if (!frame) {
        return Qt::gray;
    }

    switch (frame->frameType) {
        case AV_PICTURE_TYPE_I:
            return QColor(255, 100, 100);
        case AV_PICTURE_TYPE_P:
            return QColor(100, 100, 255);
        case AV_PICTURE_TYPE_B:
            return QColor(100, 255, 100);
        default:
            return Qt::gray;
    }
}

} // namespace VideoStudio
