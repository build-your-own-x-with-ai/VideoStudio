#include "widgets/gopviewer.h"
#include "core/framedata.h"
#include "core/videodecoder.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <cmath>

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace VideoStudio {

GOPViewer::GOPViewer(QWidget* parent)
    : QWidget(parent)
    , m_frameIndex(nullptr)
    , m_videoDecoder(nullptr)
    , m_currentFrame(0)
    , m_showThumbnails(false)
    , m_showDependencies(false)
    , m_frameWidth(30)
    , m_frameHeight(24)
    , m_horizontalSpacing(4)
    , m_verticalSpacing(10)
    , m_leftMargin(10)
    , m_topMargin(10)
{
    setMinimumHeight(150);
    setMouseTracking(true);
}

GOPViewer::~GOPViewer() {
}

void GOPViewer::setFrameIndex(const FrameIndex* frameIndex) {
    m_frameIndex = frameIndex;
    if (m_frameIndex) {
        analyzeGOPStructure();
    }
    m_thumbnailCache.clear();
    QSize hint = sizeHint();
    setMinimumSize(hint);
    resize(hint);
    update();
}

void GOPViewer::setVideoDecoder(VideoDecoder* decoder) {
    m_videoDecoder = decoder;
    m_thumbnailCache.clear();
    update();
}

void GOPViewer::setCurrentFrame(int frameNumber) {
    m_currentFrame = frameNumber;
    update();
}

void GOPViewer::toggleDisplayMode() {
    m_showThumbnails = !m_showThumbnails;
    if (m_showThumbnails) {
        m_frameWidth = 60;
        m_frameHeight = 45;
        m_horizontalSpacing = 6;
    } else {
        m_frameWidth = 30;
        m_frameHeight = 24;
        m_horizontalSpacing = 4;
    }
    m_thumbnailCache.clear();
    QSize hint = sizeHint();
    setMinimumSize(hint);
    resize(hint);

    // Pre-generate thumbnails in background if switching to thumbnail mode
    if (m_showThumbnails && m_videoDecoder && m_frameIndex) {
        QTimer::singleShot(100, this, &GOPViewer::generateThumbnails);
    }

    update();
}

QSize GOPViewer::sizeHint() const {
    if (m_gops.isEmpty()) {
        return QSize(800, 150);
    }

    // Find the longest GOP
    int maxGOPSize = 0;
    for (const GOPInfo& gop : m_gops) {
        int gopSize = gop.endFrame - gop.startFrame + 1;
        maxGOPSize = qMax(maxGOPSize, gopSize);
    }

    // Calculate total width needed (longest GOP + margins)
    int totalWidth = m_leftMargin + 70 + maxGOPSize * (m_frameWidth + m_horizontalSpacing) + m_leftMargin;

    // Calculate height for all GOPs
    int totalHeight = m_topMargin + m_gops.size() * (m_frameHeight + m_verticalSpacing) + 30;

    return QSize(totalWidth, totalHeight);
}

void GOPViewer::setDuplicateFrames(const QSet<int>& duplicateFrames) {
    m_duplicateFrames = duplicateFrames;
    update();
}

void GOPViewer::toggleDependencyArrows() {
    m_showDependencies = !m_showDependencies;
    update();
}

void GOPViewer::clear() {
    m_frameIndex = nullptr;
    m_gops.clear();
    m_currentFrame = 0;
    m_thumbnailCache.clear();
    QSize hint = sizeHint();
    setMinimumSize(hint);
    resize(hint);
    update();
}

void GOPViewer::analyzeGOPStructure() {
    if (!m_frameIndex) {
        return;
    }

    m_gops.clear();

    GOPInfo currentGOP;
    currentGOP.startFrame = 0;
    currentGOP.iFrameIndex = -1;

    for (int i = 0; i < m_frameIndex->frameCount(); ++i) {
        const FrameInfo* frame = m_frameIndex->getFrame(i);
        if (!frame) continue;

        if (frame->frameType == AV_PICTURE_TYPE_I) {
            // Start new GOP
            if (currentGOP.iFrameIndex != -1) {
                currentGOP.endFrame = i - 1;
                m_gops.append(currentGOP);
            }

            currentGOP = GOPInfo();
            currentGOP.startFrame = i;
            currentGOP.iFrameIndex = i;
            currentGOP.pFrames.clear();
            currentGOP.bFrames.clear();
        } else if (frame->frameType == AV_PICTURE_TYPE_P) {
            currentGOP.pFrames.append(i);
        } else if (frame->frameType == AV_PICTURE_TYPE_B) {
            currentGOP.bFrames.append(i);
        }
    }

    // Add last GOP
    if (currentGOP.iFrameIndex != -1) {
        currentGOP.endFrame = m_frameIndex->frameCount() - 1;
        m_gops.append(currentGOP);
    }

    qDebug() << "Analyzed" << m_gops.size() << "GOPs";
}

void GOPViewer::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Fill background
    painter.fillRect(rect(), QColor(40, 40, 40));

    if (!m_frameIndex || m_gops.isEmpty()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, tr("No GOP structure available"));
        return;
    }

    drawGOPStructure(painter);
}

void GOPViewer::drawGOPStructure(QPainter& painter) {
    int y = m_topMargin;
    int maxX = 0;

    // Draw all GOPs (scrolling handled by QScrollArea)
    for (int gopIdx = 0; gopIdx < m_gops.size(); ++gopIdx) {
        const GOPInfo& gop = m_gops[gopIdx];

        // Draw GOP label
        painter.setPen(QColor(180, 180, 180));
        painter.setFont(QFont("Arial", 9));
        QString gopLabel = QString("GOP %1:").arg(gopIdx + 1);
        painter.drawText(m_leftMargin, y + m_frameHeight / 2 + 4, gopLabel);

        int x = m_leftMargin + 70;

        // Draw all frames in this GOP
        for (int frameIdx = gop.startFrame; frameIdx <= gop.endFrame; ++frameIdx) {
            const FrameInfo* frame = m_frameIndex->getFrame(frameIdx);
            if (!frame) continue;

            drawFrame(painter, frameIdx, x, y, m_frameWidth, m_frameHeight);
            x += m_frameWidth + m_horizontalSpacing;
        }

        maxX = qMax(maxX, x);
        y += m_frameHeight + m_verticalSpacing;
    }

    // Draw dependency arrows if enabled
    if (m_showDependencies) {
        drawDependencyArrows(painter);
    }

    // Debug output
    qDebug() << "GOPViewer: drew" << m_gops.size() << "GOPs, maxX =" << maxX
             << "sizeHint width =" << sizeHint().width();

    // Draw scroll info at bottom
    if (!m_gops.isEmpty()) {
        QString scrollInfo = QString("Showing all %1 GOPs").arg(m_gops.size());
        painter.setPen(QColor(150, 150, 150));
        painter.setFont(QFont("Arial", 9));
        painter.drawText(QRect(10, y + 5, width() - 20, 15), Qt::AlignCenter, scrollInfo);
    }
}

void GOPViewer::drawDependencyArrows(QPainter& painter) {
    painter.setRenderHint(QPainter::Antialiasing);

    for (int gopIdx = 0; gopIdx < m_gops.size(); ++gopIdx) {
        const GOPInfo& gop = m_gops[gopIdx];
        int gopY = m_topMargin + gopIdx * (m_frameHeight + m_verticalSpacing);

        for (int frameIdx = gop.startFrame; frameIdx <= gop.endFrame; ++frameIdx) {
            const FrameInfo* frame = m_frameIndex->getFrame(frameIdx);
            if (!frame) continue;

            if (frame->frameType == AV_PICTURE_TYPE_P || frame->frameType == AV_PICTURE_TYPE_B) {
                int frameOffset = frameIdx - gop.startFrame;
                int x2 = m_leftMargin + 70 + frameOffset * (m_frameWidth + m_horizontalSpacing) + m_frameWidth / 2;
                int y2 = gopY + m_frameHeight / 2;

                int refFrame = gop.iFrameIndex;
                for (int i = frameIdx - 1; i >= gop.startFrame; --i) {
                    const FrameInfo* refInfo = m_frameIndex->getFrame(i);
                    if (refInfo && (refInfo->frameType == AV_PICTURE_TYPE_I || refInfo->frameType == AV_PICTURE_TYPE_P)) {
                        refFrame = i;
                        break;
                    }
                }

                int refOffset = refFrame - gop.startFrame;
                int x1 = m_leftMargin + 70 + refOffset * (m_frameWidth + m_horizontalSpacing) + m_frameWidth / 2;
                int y1 = gopY + m_frameHeight / 2;

                QColor arrowColor = (frame->frameType == AV_PICTURE_TYPE_P) ?
                    QColor(100, 100, 255, 180) : QColor(100, 255, 100, 180);
                painter.setPen(QPen(arrowColor, 1.5));

                QPainterPath path;
                path.moveTo(x1, y1);
                int controlY = y1 - 15;
                path.quadTo((x1 + x2) / 2, controlY, x2, y2);
                painter.drawPath(path);

                double angle = std::atan2(y2 - controlY, x2 - (x1 + x2) / 2);
                int arrowSize = 5;
                QPointF p1(x2 - arrowSize * std::cos(angle - M_PI / 6),
                          y2 - arrowSize * std::sin(angle - M_PI / 6));
                QPointF p2(x2 - arrowSize * std::cos(angle + M_PI / 6),
                          y2 - arrowSize * std::sin(angle + M_PI / 6));
                painter.drawLine(QPointF(x2, y2), p1);
                painter.drawLine(QPointF(x2, y2), p2);
            }
        }
    }
}

void GOPViewer::drawFrame(QPainter& painter, int frameNumber, int x, int y, int w, int h) {
    const FrameInfo* frame = m_frameIndex->getFrame(frameNumber);
    if (!frame) return;

    // Determine color based on frame type
    QColor frameColor;
    QString frameLabel;
    switch (frame->frameType) {
        case AV_PICTURE_TYPE_I:
            frameColor = QColor(255, 100, 100);
            frameLabel = "I";
            break;
        case AV_PICTURE_TYPE_P:
            frameColor = QColor(100, 100, 255);
            frameLabel = "P";
            break;
        case AV_PICTURE_TYPE_B:
            frameColor = QColor(100, 255, 100);
            frameLabel = "B";
            break;
        default:
            frameColor = QColor(200, 200, 200);
            frameLabel = "?";
            break;
    }

    // Highlight current frame
    if (frameNumber == m_currentFrame) {
        painter.setPen(QPen(Qt::yellow, 3));
        painter.setBrush(frameColor);
    } else if (m_duplicateFrames.contains(frameNumber)) {
        // Mark duplicate frames with orange border
        painter.setPen(QPen(QColor(255, 165, 0), 2));
        painter.setBrush(frameColor);
    } else {
        painter.setPen(QPen(Qt::white, 1));
        painter.setBrush(frameColor);
    }

    // Draw frame rectangle
    painter.drawRect(x, y, w, h);

    // Draw duplicate marker if this is a duplicate frame
    if (m_duplicateFrames.contains(frameNumber)) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 165, 0));
        painter.drawEllipse(x + w - 10, y + 2, 8, 8);
        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 6, QFont::Bold));
        painter.drawText(QRect(x + w - 10, y + 2, 8, 8), Qt::AlignCenter, "D");
    }

    if (m_showThumbnails && m_videoDecoder) {
        if (m_thumbnailCache.contains(frameNumber)) {
            painter.drawPixmap(x, y, m_thumbnailCache[frameNumber]);
        }

        painter.setFont(QFont("Arial", 7));
        painter.setPen(Qt::white);
        painter.drawText(QRect(x, y + h - 12, w, 12), Qt::AlignCenter, QString::number(frameNumber));
    } else if (m_showThumbnails) {
        // Thumbnail mode but no decoder - show frame type with frame number
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        painter.drawText(QRect(x, y, w, h - 12), Qt::AlignCenter, frameLabel);
        painter.setFont(QFont("Arial", 8));
        painter.drawText(QRect(x, y + h - 12, w, 12), Qt::AlignCenter, QString::number(frameNumber));
    } else {
        // Text mode - just show frame type
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        painter.drawText(QRect(x, y, w, h), Qt::AlignCenter, frameLabel);
    }
}

void GOPViewer::generateThumbnails() {
    if (!m_videoDecoder || !m_frameIndex || !m_showThumbnails) {
        return;
    }

    // Generate in batches to avoid UI freeze
    int batchSize = 50;
    int startIdx = m_thumbnailCache.size();
    int count = 0;

    for (const GOPInfo& gop : m_gops) {
        for (int frameIdx = gop.startFrame; frameIdx <= gop.endFrame; ++frameIdx) {
            if (frameIdx < startIdx) continue;
            if (count >= batchSize) {
                // Schedule next batch
                QTimer::singleShot(100, this, &GOPViewer::generateThumbnails);
                update();
                return;
            }

            if (!m_thumbnailCache.contains(frameIdx)) {
                if (m_videoDecoder->seekToFrame(frameIdx)) {
                    AVFrame* avFrame = m_videoDecoder->getCurrentFrame();
                    if (avFrame && avFrame->data[0]) {
                        int w = m_frameWidth;
                        int h = m_frameHeight - 12;
                        SwsContext* swsContext = sws_getContext(
                            avFrame->width, avFrame->height, (AVPixelFormat)avFrame->format,
                            w, h, AV_PIX_FMT_RGB24,
                            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

                        if (swsContext) {
                            int rgbLinesize = w * 3;
                            uint8_t* rgbData = new uint8_t[rgbLinesize * h];
                            uint8_t* dstData[1] = { rgbData };
                            int dstLinesize[1] = { rgbLinesize };

                            sws_scale(swsContext, avFrame->data, avFrame->linesize, 0, avFrame->height,
                                     dstData, dstLinesize);

                            QImage img(rgbData, w, h, rgbLinesize, QImage::Format_RGB888);
                            m_thumbnailCache[frameIdx] = QPixmap::fromImage(img.copy());

                            delete[] rgbData;
                            sws_freeContext(swsContext);
                        }
                    }
                }
                count++;
            }
        }
    }

    update();
}

void GOPViewer::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int x = event->pos().x();
        int y = event->pos().y();

        // Find which GOP row was clicked
        int rowY = m_topMargin;
        for (int gopIdx = 0; gopIdx < m_gops.size(); ++gopIdx) {
            if (y >= rowY && y < rowY + m_frameHeight) {
                const GOPInfo& gop = m_gops[gopIdx];
                int frameX = m_leftMargin + 70;

                // Find which frame was clicked in this GOP
                for (int frameIdx = gop.startFrame; frameIdx <= gop.endFrame; ++frameIdx) {
                    if (x >= frameX && x < frameX + m_frameWidth) {
                        emit frameClicked(frameIdx);
                        return;
                    }
                    frameX += m_frameWidth + m_horizontalSpacing;
                }
                return;
            }
            rowY += m_frameHeight + m_verticalSpacing;
        }
    }
}

void GOPViewer::wheelEvent(QWheelEvent* event) {
    // Let QScrollArea handle scrolling
    event->ignore();
}

void GOPViewer::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

} // namespace VideoStudio
