#include "widgets/videooutput.h"
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include <cmath>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/motion_vector.h>
#include <libavutil/video_enc_params.h>
}

namespace VideoStudio {

VideoOutput::VideoOutput(QWidget* parent)
    : QWidget(parent)
    , m_swsContext(nullptr)
    , m_lastWidth(0)
    , m_lastHeight(0)
    , m_currentFrame(nullptr)
    , m_overlayFlags(0)
    , m_zoomLevel(1.0)
    , m_cursorModeEnabled(false)
    , m_hasCursorPos(false)
    , m_standardGridMode(true)  // Default to standard grid mode
{
    setMinimumSize(320, 240);
    setStyleSheet("background-color: black;");
    setMouseTracking(true); // Enable mouse tracking for cursor mode
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

    m_currentFrame = frame;
    convertFrameToImage(frame);
    update();
}

void VideoOutput::clear() {
    m_image = QImage();
    m_currentFrame = nullptr;
    update();
}

void VideoOutput::setOverlay(OverlayType type, bool enabled) {
    int flag = 1 << static_cast<int>(type);
    if (enabled) {
        m_overlayFlags |= flag;
    } else {
        m_overlayFlags &= ~flag;
    }
    update();
}

bool VideoOutput::isOverlayEnabled(OverlayType type) const {
    int flag = 1 << static_cast<int>(type);
    return (m_overlayFlags & flag) != 0;
}

void VideoOutput::toggleOverlay(OverlayType type) {
    setOverlay(type, !isOverlayEnabled(type));
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

        if (m_swsContext) {
            // Set correct color space conversion parameters
            // Use BT.709 for HD content, BT.601 for SD content
            int colorspace = frame->height >= 720 ? SWS_CS_ITU709 : SWS_CS_ITU601;
            const int* inv_table = sws_getCoefficients(colorspace);
            const int* table = sws_getCoefficients(colorspace);

            sws_setColorspaceDetails(m_swsContext,
                inv_table, frame->color_range == AVCOL_RANGE_JPEG ? 1 : 0,
                table, 0,  // RGB is always full range
                0, 1 << 16, 1 << 16);
        }

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
        // Apply zoom level to the image size
        QSize scaledSize = m_image.size() * m_zoomLevel;

        // If zoom is 1.0 (fit mode), scale to fit widget
        if (m_zoomLevel == 1.0) {
            scaledSize.scale(size(), Qt::KeepAspectRatio);
        } else {
            // When zoomed, set widget size to scaled image size
            setMinimumSize(scaledSize);
            resize(scaledSize);
        }

        QRect targetRect(
            (width() - scaledSize.width()) / 2,
            (height() - scaledSize.height()) / 2,
            scaledSize.width(),
            scaledSize.height()
        );

        painter.drawImage(targetRect, m_image);

        // Draw overlays on top of video
        if (m_overlayFlags != 0) {
            drawOverlays(painter, targetRect);
        }

        // Draw cursor info if cursor mode is enabled
        if (m_cursorModeEnabled) {
            drawCursorInfo(painter, targetRect);
        }

        // Draw zoom level indicator (bottom-right corner)
        if (m_zoomLevel != 1.0) {
            QString zoomText = QString("%1%").arg(static_cast<int>(m_zoomLevel * 100));
            painter.setFont(QFont("Arial", 12, QFont::Bold));
            QFontMetrics fm(painter.font());
            int textWidth = fm.horizontalAdvance(zoomText);
            int textHeight = fm.height();

            int padding = 8;
            int boxWidth = textWidth + padding * 2;
            int boxHeight = textHeight + padding;

            // Position in bottom-right corner
            int boxX = width() - boxWidth - 10;
            int boxY = height() - boxHeight - 10;

            // Draw semi-transparent background
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 180));
            painter.drawRoundedRect(boxX, boxY, boxWidth, boxHeight, 4, 4);

            // Draw text
            painter.setPen(Qt::white);
            painter.drawText(boxX + padding, boxY + textHeight, zoomText);
        }
    }
}

void VideoOutput::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

void VideoOutput::drawOverlays(QPainter& painter, const QRect& videoRect) {
    if (!m_currentFrame) {
        return;
    }

    painter.setRenderHint(QPainter::Antialiasing);

    if (isOverlayEnabled(OverlayType::MotionVectors)) {
        drawMotionVectors(painter, videoRect);
    }

    if (isOverlayEnabled(OverlayType::Partitions)) {
        drawBlockBoundaries(painter, videoRect);
    }

    if (isOverlayEnabled(OverlayType::FrameTypes)) {
        drawFrameTypeInfo(painter, videoRect);
    }

    if (isOverlayEnabled(OverlayType::QuantizationParameter)) {
        drawQPHeatmap(painter, videoRect);
    }
}

void VideoOutput::drawMotionVectors(QPainter& painter, const QRect& videoRect) {
    if (!m_currentFrame) {
        return;
    }

    // Get motion vector side data
    AVFrameSideData* sd = av_frame_get_side_data(m_currentFrame, AV_FRAME_DATA_MOTION_VECTORS);
    if (!sd) {
        // Draw "No MV data" message for debugging
        painter.setPen(QPen(QColor(255, 255, 0, 200), 1));
        painter.setFont(QFont("Arial", 10));
        painter.drawText(videoRect.adjusted(10, 40, -10, -10),
                        Qt::AlignLeft | Qt::AlignTop,
                        "No motion vector data available");
        return;
    }

    const AVMotionVector* mvs = reinterpret_cast<const AVMotionVector*>(sd->data);
    int mvCount = sd->size / sizeof(AVMotionVector);

    qDebug() << "Drawing" << mvCount << "motion vectors";

    double scaleX = static_cast<double>(videoRect.width()) / m_currentFrame->width;
    double scaleY = static_cast<double>(videoRect.height()) / m_currentFrame->height;

    // Draw motion vectors with different colors based on magnitude
    for (int i = 0; i < mvCount; ++i) {
        const AVMotionVector* mv = &mvs[i];

        // Skip zero motion vectors
        if (mv->motion_x == 0 && mv->motion_y == 0) {
            continue;
        }

        // Calculate block center in video coordinates
        int blockCenterX = mv->src_x + mv->w / 2;
        int blockCenterY = mv->src_y + mv->h / 2;

        // Scale to widget coordinates
        int x1 = videoRect.left() + static_cast<int>(blockCenterX * scaleX);
        int y1 = videoRect.top() + static_cast<int>(blockCenterY * scaleY);

        // Scale motion vector (divide by 4 because motion vectors are in quarter-pixel units)
        int x2 = x1 + static_cast<int>(mv->motion_x * scaleX / 4.0);
        int y2 = y1 + static_cast<int>(mv->motion_y * scaleY / 4.0);

        // Calculate magnitude for color coding
        double magnitude = std::sqrt(mv->motion_x * mv->motion_x + mv->motion_y * mv->motion_y);

        // Color based on magnitude: green (small) -> yellow -> red (large)
        QColor color;
        if (magnitude < 8) {
            color = QColor(0, 255, 0, 200);  // Green for small motion
        } else if (magnitude < 16) {
            color = QColor(255, 255, 0, 200);  // Yellow for medium motion
        } else {
            color = QColor(255, 0, 0, 200);  // Red for large motion
        }

        painter.setPen(QPen(color, 2));

        // Draw motion vector line
        painter.drawLine(x1, y1, x2, y2);

        // Draw arrowhead
        double angle = std::atan2(y2 - y1, x2 - x1);
        int arrowSize = 6;
        QPointF p1(x2 - arrowSize * std::cos(angle - M_PI / 6),
                   y2 - arrowSize * std::sin(angle - M_PI / 6));
        QPointF p2(x2 - arrowSize * std::cos(angle + M_PI / 6),
                   y2 - arrowSize * std::sin(angle + M_PI / 6));
        painter.drawLine(QPointF(x2, y2), p1);
        painter.drawLine(QPointF(x2, y2), p2);
    }
}

void VideoOutput::drawBlockBoundaries(QPainter& painter, const QRect& videoRect) {
    if (!m_currentFrame) {
        return;
    }

    double scaleX = static_cast<double>(videoRect.width()) / m_currentFrame->width;
    double scaleY = static_cast<double>(videoRect.height()) / m_currentFrame->height;

    // I-frames don't have motion vectors, use fallback grid
    bool isIFrame = (m_currentFrame->pict_type == AV_PICTURE_TYPE_I);

    // Try to get motion vector data to draw actual block sizes (only for P/B frames)
    AVFrameSideData* sd = nullptr;
    if (!isIFrame) {
        sd = av_frame_get_side_data(m_currentFrame, AV_FRAME_DATA_MOTION_VECTORS);
    }

    if (sd && !isIFrame) {
        // Draw actual block boundaries from motion vector data
        const AVMotionVector* mvs = reinterpret_cast<const AVMotionVector*>(sd->data);
        int mvCount = sd->size / sizeof(AVMotionVector);

        // Check if all blocks are the same size (uniform partitioning)
        bool uniformSize = true;
        int firstBlockSize = (mvCount > 0) ? mvs[0].w : 0;

        // Also check if blocks are properly aligned (starting from 0,0 with regular grid)
        int minX = INT_MAX, minY = INT_MAX;
        for (int i = 0; i < mvCount; ++i) {
            if (mvs[i].src_x < minX) minX = mvs[i].src_x;
            if (mvs[i].src_y < minY) minY = mvs[i].src_y;
            if (mvs[i].w != firstBlockSize || mvs[i].h != firstBlockSize) {
                uniformSize = false;
            }
        }

        bool wellAligned = (minX <= 2 && minY <= 2); // Allow small offset tolerance

        // If standard grid mode is enabled OR (blocks are uniform and well-aligned), draw a clean grid
        if (m_standardGridMode || (uniformSize && wellAligned && firstBlockSize > 0)) {
            // Draw clean grid for uniform blocks
            painter.setPen(QPen(QColor(255, 255, 0, 150), 1));

            // Use the detected block size if available, otherwise default to 16
            int gridSize = (firstBlockSize > 0) ? firstBlockSize : 16;

            double blockWidthScreen = (videoRect.width() * gridSize) / static_cast<double>(m_currentFrame->width);
            double blockHeightScreen = (videoRect.height() * gridSize) / static_cast<double>(m_currentFrame->height);

            int numHorizontalBlocks = (m_currentFrame->height + gridSize - 1) / gridSize;
            for (int i = 0; i <= numHorizontalBlocks; ++i) {
                int y1 = videoRect.top() + qRound(i * blockHeightScreen);
                painter.drawLine(videoRect.left(), y1, videoRect.right(), y1);
            }

            int numVerticalBlocks = (m_currentFrame->width + gridSize - 1) / gridSize;
            for (int i = 0; i <= numVerticalBlocks; ++i) {
                int x1 = videoRect.left() + qRound(i * blockWidthScreen);
                painter.drawLine(x1, videoRect.top(), x1, videoRect.bottom());
            }

            // Draw info text
            if (m_zoomLevel >= 1.0) {
                QString infoText;
                if (m_standardGridMode) {
                    infoText = QString("Standard %1x%1 Grid (P frame, %2 blocks)")
                              .arg(gridSize).arg(mvCount);
                } else {
                    infoText = QString("Uniform %1x%1 Block Grid (P frame, %2 blocks)")
                              .arg(gridSize).arg(mvCount);
                }

                painter.setFont(QFont("Arial", 10, QFont::Bold));
                QFontMetrics fm(painter.font());
                QRect textRect = fm.boundingRect(infoText);
                int textWidth = textRect.width();
                int textHeight = textRect.height();

                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(0, 0, 0, 180));
                painter.drawRect(videoRect.left() + 8, videoRect.bottom() - textHeight - 14,
                               textWidth + 4, textHeight + 4);

                painter.setPen(Qt::white);
                painter.drawText(videoRect.left() + 10, videoRect.bottom() - 10, infoText);
            }

            return; // Skip individual block drawing
        }

        // Otherwise, draw actual block boundaries with enhanced color coding

        // Get QP data for color intensity adjustment
        AVFrameSideData* encParamsSd = av_frame_get_side_data(m_currentFrame, AV_FRAME_DATA_VIDEO_ENC_PARAMS);
        AVVideoEncParams* encParams = nullptr;
        if (encParamsSd) {
            encParams = reinterpret_cast<AVVideoEncParams*>(encParamsSd->data);
        }

        // Count block types and sizes for statistics
        QMap<int, int> blockSizeCount;
        int intraCount = 0, interCount = 0, skipCount = 0;

        // Draw each block with prediction mode color coding
        for (int i = 0; i < mvCount; ++i) {
            const AVMotionVector* mv = &mvs[i];

            int x = videoRect.left() + static_cast<int>(mv->src_x * scaleX);
            int y = videoRect.top() + static_cast<int>(mv->src_y * scaleY);
            int w = static_cast<int>(mv->w * scaleX);
            int h = static_cast<int>(mv->h * scaleY);

            // Determine block prediction mode
            bool isIntra = (mv->source == -1);
            bool isSkip = (!isIntra && mv->motion_x == 0 && mv->motion_y == 0);

            // Count block types
            if (isIntra) {
                intraCount++;
            } else if (isSkip) {
                skipCount++;
            } else {
                interCount++;
            }

            // Base color by prediction mode
            QColor baseColor;
            if (isIntra) {
                baseColor = QColor(255, 100, 100);  // Red for Intra
            } else if (isSkip) {
                baseColor = QColor(150, 150, 150);  // Gray for Skip
            } else {
                baseColor = QColor(100, 150, 255);  // Blue for Inter
            }

            // Adjust color intensity based on QP (if available)
            int blockQP = -1;
            if (encParams && i < static_cast<int>(encParams->nb_blocks)) {
                AVVideoBlockParams* block = av_video_enc_params_block(encParams, i);
                blockQP = encParams->qp + block->delta_qp;

                // Adjust alpha based on QP: high QP = more opaque (worse quality)
                int alpha = 100 + (blockQP * 3);  // QP 0->100, QP 51->253
                alpha = qMin(alpha, 200);
                baseColor.setAlpha(alpha);
            } else {
                baseColor.setAlpha(150);
            }

            // Fill block with color
            painter.setPen(Qt::NoPen);
            painter.setBrush(baseColor);
            painter.drawRect(x, y, w, h);

            // Draw block boundary
            int blockSize = mv->w;
            blockSizeCount[blockSize]++;

            // Boundary color based on block size
            QColor borderColor;
            if (blockSize >= 32) {
                borderColor = QColor(255, 255, 0, 255);  // Yellow for large blocks
            } else if (blockSize >= 16) {
                borderColor = QColor(255, 200, 0, 200);  // Orange for medium blocks
            } else {
                borderColor = QColor(200, 200, 0, 150);  // Dim yellow for small blocks
            }

            int lineWidth = (blockSize >= 32) ? 2 : 1;
            painter.setPen(QPen(borderColor, lineWidth));
            painter.drawRect(x, y, w, h);

            // Draw sub-partition boundaries for blocks >= 8x8 (at higher zoom)
            if (m_zoomLevel >= 1.5 && blockSize >= 8) {
                // Draw 4x4 sub-partitions with thin dark lines
                painter.setPen(QPen(QColor(50, 50, 50, 200), 1, Qt::SolidLine));

                // Horizontal 4x4 lines
                int subSizeScreen = w / 4;
                for (int j = 1; j < 4; j++) {
                    int subY = y + j * subSizeScreen;
                    painter.drawLine(x, subY, x + w, subY);
                }

                // Vertical 4x4 lines
                for (int j = 1; j < 4; j++) {
                    int subX = x + j * subSizeScreen;
                    painter.drawLine(subX, y, subX, y + h);
                }
            }

            // Draw reference frame index for Inter blocks (if zoom is sufficient)
            if (!isIntra && m_zoomLevel >= 1.2 && w >= 20 && h >= 20) {
                int refIdx = mv->source;
                if (refIdx >= 0) {
                    QString refText = QString::number(refIdx);

                    painter.setFont(QFont("Arial", 9, QFont::Bold));
                    QFontMetrics fm(painter.font());
                    int textWidth = fm.horizontalAdvance(refText);
                    int textHeight = fm.height();

                    int centerX = x + w / 2 - textWidth / 2;
                    int centerY = y + h / 2 + textHeight / 3;

                    // Draw black background
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(0, 0, 0, 200));
                    painter.drawRect(centerX - 2, centerY - textHeight + 2, textWidth + 4, textHeight);

                    // Draw white text
                    painter.setPen(Qt::white);
                    painter.drawText(centerX, centerY, refText);
                }
            }

            // Draw QP value for blocks with significant QP (if zoom is sufficient)
            if (blockQP >= 0 && m_zoomLevel >= 1.5 && w >= 24 && h >= 24) {
                QString qpText = QString("Q%1").arg(blockQP);

                painter.setFont(QFont("Arial", 7));
                QFontMetrics fm(painter.font());
                int textWidth = fm.horizontalAdvance(qpText);

                // Draw in top-left corner
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(0, 0, 0, 180));
                painter.drawRect(x + 2, y + 2, textWidth + 3, 10);

                painter.setPen(Qt::white);
                painter.drawText(x + 3, y + 10, qpText);
            }
        }

        // Draw prediction mode statistics legend
        if (m_zoomLevel >= 0.8) {
            int legendX = videoRect.left() + 10;
            int legendY = videoRect.top() + 10;
            painter.setFont(QFont("Arial", 9, QFont::Bold));

            // Background for entire legend
            int legendHeight = 60;
            int legendWidth = 140;
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 200));
            painter.drawRect(legendX - 4, legendY - 4, legendWidth, legendHeight);

            // Title
            painter.setPen(Qt::white);
            painter.drawText(legendX, legendY + 10, "Prediction Modes:");

            // Intra blocks
            painter.setBrush(QColor(255, 100, 100, 180));
            painter.drawRect(legendX, legendY + 16, 12, 12);
            painter.setPen(Qt::white);
            painter.drawText(legendX + 16, legendY + 26, QString("Intra: %1").arg(intraCount));

            // Inter blocks
            painter.setBrush(QColor(100, 150, 255, 180));
            painter.drawRect(legendX, legendY + 32, 12, 12);
            painter.setPen(Qt::white);
            painter.drawText(legendX + 16, legendY + 42, QString("Inter: %1").arg(interCount));

            // Skip blocks
            painter.setBrush(QColor(150, 150, 150, 180));
            painter.drawRect(legendX, legendY + 48, 12, 12);
            painter.setPen(Qt::white);
            painter.drawText(legendX + 16, legendY + 58, QString("Skip: %1").arg(skipCount));
        }
    } else {
        // Fallback: Draw standard 16x16 macroblock grid aligned to video
        painter.setPen(QPen(QColor(255, 255, 0, 150), 1));

        int blockSize = 16;

        // Calculate exact block dimensions in screen coordinates
        double blockWidthScreen = (videoRect.width() * blockSize) / static_cast<double>(m_currentFrame->width);
        double blockHeightScreen = (videoRect.height() * blockSize) / static_cast<double>(m_currentFrame->height);

        // Draw horizontal lines
        int numHorizontalBlocks = (m_currentFrame->height + blockSize - 1) / blockSize;
        for (int i = 0; i <= numHorizontalBlocks; ++i) {
            int y1 = videoRect.top() + qRound(i * blockHeightScreen);
            painter.drawLine(videoRect.left(), y1, videoRect.right(), y1);
        }

        // Draw vertical lines
        int numVerticalBlocks = (m_currentFrame->width + blockSize - 1) / blockSize;
        for (int i = 0; i <= numVerticalBlocks; ++i) {
            int x1 = videoRect.left() + qRound(i * blockWidthScreen);
            painter.drawLine(x1, videoRect.top(), x1, videoRect.bottom());
        }

        // Draw macroblock size info
        if (m_zoomLevel >= 1.0) {
            QString infoText = QString("Standard 16x16 Macroblock Grid (%1 frame)")
                              .arg(isIFrame ? "I" : "Unknown");

            painter.setFont(QFont("Arial", 10, QFont::Bold));

            // Measure text size
            QFontMetrics fm(painter.font());
            QRect textRect = fm.boundingRect(infoText);
            int textWidth = textRect.width();
            int textHeight = textRect.height();

            // Draw semi-transparent black background
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 180));
            painter.drawRect(videoRect.left() + 8, videoRect.bottom() - textHeight - 14,
                           textWidth + 4, textHeight + 4);

            // Draw text
            painter.setPen(Qt::white);
            painter.drawText(videoRect.left() + 10, videoRect.bottom() - 10, infoText);
        }
    }
}

void VideoOutput::drawFrameTypeInfo(QPainter& painter, const QRect& videoRect) {
    if (!m_currentFrame) {
        return;
    }

    QString frameTypeStr;
    QColor frameColor;

    switch (m_currentFrame->pict_type) {
        case AV_PICTURE_TYPE_I:
            frameTypeStr = "I-Frame";
            frameColor = QColor(255, 100, 100, 200);
            break;
        case AV_PICTURE_TYPE_P:
            frameTypeStr = "P-Frame";
            frameColor = QColor(100, 100, 255, 200);
            break;
        case AV_PICTURE_TYPE_B:
            frameTypeStr = "B-Frame";
            frameColor = QColor(100, 255, 100, 200);
            break;
        default:
            frameTypeStr = "Unknown";
            frameColor = QColor(200, 200, 200, 200);
            break;
    }

    // Draw frame type badge in top-left corner
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    QFontMetrics fm(painter.font());
    QRect textRect = fm.boundingRect(frameTypeStr);
    textRect.adjust(-10, -5, 10, 5);
    textRect.moveTopLeft(videoRect.topLeft() + QPoint(10, 10));

    painter.fillRect(textRect, frameColor);
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, frameTypeStr);

    // Draw key frame indicator
#ifdef AV_FRAME_FLAG_KEY
    if (m_currentFrame->flags & AV_FRAME_FLAG_KEY) {
#else
    if (m_currentFrame->key_frame) {
#endif
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        QRect keyRect(videoRect.left() + 10, videoRect.top() + textRect.height() + 15, 80, 20);
        painter.fillRect(keyRect, QColor(255, 215, 0, 200));
        painter.setPen(Qt::black);
        painter.drawText(keyRect, Qt::AlignCenter, "KEY FRAME");
    }
}

void VideoOutput::zoomIn() {
    m_zoomLevel *= 1.25;
    if (m_zoomLevel > 4.0) {
        m_zoomLevel = 4.0;
    }
    update();
}

void VideoOutput::zoomOut() {
    m_zoomLevel /= 1.25;
    if (m_zoomLevel < 0.25) {
        m_zoomLevel = 0.25;
    }
    update();
}

void VideoOutput::zoomFit() {
    m_zoomLevel = 1.0;
    update();
}

void VideoOutput::setCursorMode(bool enabled) {
    m_cursorModeEnabled = enabled;
    if (!enabled) {
        m_hasCursorPos = false;
    }
    update();
}

void VideoOutput::setStandardGridMode(bool enabled) {
    m_standardGridMode = enabled;
    update();
}

void VideoOutput::mouseMoveEvent(QMouseEvent* event) {
    if (m_cursorModeEnabled && !m_image.isNull()) {
        m_cursorPos = event->pos();
        m_hasCursorPos = true;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void VideoOutput::drawCursorInfo(QPainter& painter, const QRect& videoRect) {
    if (!m_hasCursorPos || !videoRect.contains(m_cursorPos)) {
        return;
    }

    // Calculate position in video coordinates
    int videoX = (m_cursorPos.x() - videoRect.left()) * m_image.width() / videoRect.width();
    int videoY = (m_cursorPos.y() - videoRect.top()) * m_image.height() / videoRect.height();

    // Get block information
    QString blockInfo = getBlockInfoAtPosition(QPoint(videoX, videoY), videoRect);

    if (blockInfo.isEmpty()) {
        return;
    }

    // Draw tooltip near cursor
    QFont font("Courier", 9);
    painter.setFont(font);
    QFontMetrics fm(font);
    QStringList lines = blockInfo.split('\n');

    int maxWidth = 0;
    for (const QString& line : lines) {
        maxWidth = qMax(maxWidth, fm.horizontalAdvance(line));
    }

    int tooltipWidth = maxWidth + 20;
    int tooltipHeight = lines.size() * fm.height() + 10;

    // Position tooltip near cursor, but keep it inside widget
    QPoint tooltipPos = m_cursorPos + QPoint(15, 15);
    if (tooltipPos.x() + tooltipWidth > width()) {
        tooltipPos.setX(m_cursorPos.x() - tooltipWidth - 15);
    }
    if (tooltipPos.y() + tooltipHeight > height()) {
        tooltipPos.setY(m_cursorPos.y() - tooltipHeight - 15);
    }

    QRect tooltipRect(tooltipPos, QSize(tooltipWidth, tooltipHeight));

    // Draw tooltip background
    painter.fillRect(tooltipRect, QColor(0, 0, 0, 220));
    painter.setPen(QColor(255, 255, 0));
    painter.drawRect(tooltipRect);

    // Draw text
    painter.setPen(Qt::white);
    int y = tooltipRect.top() + fm.height();
    for (const QString& line : lines) {
        painter.drawText(tooltipRect.left() + 10, y, line);
        y += fm.height();
    }

    // Draw crosshair at cursor position
    painter.setPen(QPen(QColor(255, 255, 0), 1));
    painter.drawLine(m_cursorPos.x() - 10, m_cursorPos.y(), m_cursorPos.x() + 10, m_cursorPos.y());
    painter.drawLine(m_cursorPos.x(), m_cursorPos.y() - 10, m_cursorPos.x(), m_cursorPos.y() + 10);
}

QString VideoOutput::getBlockInfoAtPosition(const QPoint& pos, const QRect& videoRect) {
    Q_UNUSED(videoRect);

    if (!m_currentFrame) {
        return QString();
    }

    QString info;
    info += QString("Position: (%1, %2)\n").arg(pos.x()).arg(pos.y());

    // Get pixel value at position
    if (pos.x() >= 0 && pos.x() < m_currentFrame->width &&
        pos.y() >= 0 && pos.y() < m_currentFrame->height) {

        // Get Y value from current frame
        if (m_currentFrame->data[0]) {
            int yStride = m_currentFrame->linesize[0];
            uint8_t yValue = m_currentFrame->data[0][pos.y() * yStride + pos.x()];
            info += QString("Y: %1\n").arg(yValue);
        }

        // Get U/V values for YUV frames
        if (m_currentFrame->format == AV_PIX_FMT_YUV420P ||
            m_currentFrame->format == AV_PIX_FMT_YUVJ420P) {
            int uvX = pos.x() / 2;
            int uvY = pos.y() / 2;
            if (m_currentFrame->data[1] && m_currentFrame->data[2]) {
                int uvStride = m_currentFrame->linesize[1];
                uint8_t uValue = m_currentFrame->data[1][uvY * uvStride + uvX];
                uint8_t vValue = m_currentFrame->data[2][uvY * uvStride + uvX];
                info += QString("U: %1, V: %2\n").arg(uValue).arg(vValue);
            }
        }

        // Get RGB value from displayed image
        if (pos.x() < m_image.width() && pos.y() < m_image.height()) {
            QRgb pixel = m_image.pixel(pos.x(), pos.y());
            info += QString("RGB: (%1, %2, %3)\n")
                .arg(qRed(pixel))
                .arg(qGreen(pixel))
                .arg(qBlue(pixel));
        }
    }

    // Try to get motion vector data for detailed block info
    AVFrameSideData* sd = av_frame_get_side_data(m_currentFrame, AV_FRAME_DATA_MOTION_VECTORS);
    if (sd) {
        const AVMotionVector* mvs = reinterpret_cast<const AVMotionVector*>(sd->data);
        int mvCount = sd->size / sizeof(AVMotionVector);

        // Find the block that contains this position
        for (int i = 0; i < mvCount; ++i) {
            const AVMotionVector* mv = &mvs[i];

            // Check if position is inside this block
            if (pos.x() >= mv->src_x && pos.x() < mv->src_x + mv->w &&
                pos.y() >= mv->src_y && pos.y() < mv->src_y + mv->h) {

                info += "\n--- Block Info ---\n";
                info += QString("Block Position: (%1, %2)\n").arg(mv->src_x).arg(mv->src_y);
                info += QString("Block Size: %1x%2\n").arg(mv->w).arg(mv->h);

                // Macroblock index
                int mbSize = 16;
                int mbX = mv->src_x / mbSize;
                int mbY = mv->src_y / mbSize;
                info += QString("Macroblock: (%1, %2)\n").arg(mbX).arg(mbY);

                // Motion vector
                if (mv->motion_x != 0 || mv->motion_y != 0) {
                    // Motion vectors are in quarter-pixel units
                    double mvX = mv->motion_x / 4.0;
                    double mvY = mv->motion_y / 4.0;
                    double magnitude = std::sqrt(mvX * mvX + mvY * mvY);
                    info += QString("Motion Vector: (%.2f, %.2f)\n").arg(mvX).arg(mvY);
                    info += QString("MV Magnitude: %.2f px\n").arg(magnitude);
                } else {
                    info += "Motion Vector: (0, 0)\n";
                }

                // Prediction mode
                bool isIntra = (mv->source == -1);
                info += QString("Prediction: %1\n").arg(isIntra ? "Intra" : "Inter");

                if (!isIntra) {
                    info += QString("Reference: Frame %1\n").arg(mv->source);
                    info += QString("Destination: (%1, %2)\n").arg(mv->dst_x).arg(mv->dst_y);
                }

                break; // Found the block, no need to continue
            }
        }
    } else {
        // Fallback: show macroblock grid info
        int mbSize = 16;
        int mbX = pos.x() / mbSize;
        int mbY = pos.y() / mbSize;
        info += QString("\nMacroblock: (%1, %2)\n").arg(mbX).arg(mbY);
        info += QString("MB Index: %1\n").arg(mbY * (m_currentFrame->width / mbSize) + mbX);
    }

    return info;
}

void VideoOutput::drawQPHeatmap(QPainter& painter, const QRect& videoRect) {
    if (!m_currentFrame) {
        return;
    }

    // Try to get video encoding parameters from side data
    AVFrameSideData* sd = av_frame_get_side_data(m_currentFrame, AV_FRAME_DATA_VIDEO_ENC_PARAMS);

    if (!sd) {
        // Draw "No QP data" message
        painter.setPen(QPen(QColor(255, 255, 0, 200), 1));
        painter.setFont(QFont("Arial", 10));
        painter.drawText(videoRect.adjusted(10, 10, -10, -10),
                        Qt::AlignLeft | Qt::AlignTop,
                        "No QP data available\n\n"
                        "QP heatmap requires:\n"
                        "1. Video must be decoded (not just demuxed)\n"
                        "2. Codec must export encoding parameters\n"
                        "3. Use 'export_side_data=venc_params' flag");
        return;
    }

    // Parse AVVideoEncParams structure
    const AVVideoEncParams* encParams = reinterpret_cast<const AVVideoEncParams*>(sd->data);

    if (!encParams || encParams->nb_blocks == 0) {
        painter.setPen(QPen(QColor(255, 255, 0, 200), 1));
        painter.setFont(QFont("Arial", 10));
        painter.drawText(videoRect.adjusted(10, 10, -10, -10),
                        Qt::AlignLeft | Qt::AlignTop,
                        "No block-level QP data in this frame");
        return;
    }

    double scaleX = static_cast<double>(videoRect.width()) / m_currentFrame->width;
    double scaleY = static_cast<double>(videoRect.height()) / m_currentFrame->height;

    // Draw QP heatmap for each block
    int minQP = 51;
    int maxQP = 0;

    for (unsigned int i = 0; i < encParams->nb_blocks; ++i) {
        AVVideoBlockParams* block = av_video_enc_params_block(const_cast<AVVideoEncParams*>(encParams), i);

        if (!block) {
            continue;
        }

        // Get QP value - it might be in delta_qp or absolute
        int qp = encParams->qp;  // Base QP
        if (block->delta_qp) {
            qp += block->delta_qp;
        }

        // Clamp QP to valid range
        qp = qMax(0, qMin(51, qp));

        // Track min/max QP for statistics
        minQP = qMin(minQP, qp);
        maxQP = qMax(maxQP, qp);

        // Get block position and size
        int blockX = block->src_x;
        int blockY = block->src_y;
        int blockW = block->w;
        int blockH = block->h;

        // Calculate block rectangle in screen coordinates
        int x = videoRect.left() + static_cast<int>(blockX * scaleX);
        int y = videoRect.top() + static_cast<int>(blockY * scaleY);
        int w = static_cast<int>(blockW * scaleX);
        int h = static_cast<int>(blockH * scaleY);

        // Get color for this QP value
        QColor qpColor = getQPColor(qp);
        qpColor.setAlpha(160);  // Semi-transparent overlay

        // Draw filled rectangle
        painter.fillRect(x, y, w, h, qpColor);

        // Draw QP value text if zoom is sufficient
        if (m_zoomLevel >= 1.5 && w >= 24 && h >= 24) {
            painter.setFont(QFont("Arial", 8, QFont::Bold));
            painter.setPen(Qt::white);

            QString qpText = QString::number(qp);
            QFontMetrics fm(painter.font());
            QRect textRect = fm.boundingRect(qpText);
            int textX = x + w / 2 - textRect.width() / 2;
            int textY = y + h / 2 + textRect.height() / 2 - 2;

            // Draw text shadow for better visibility
            painter.setPen(Qt::black);
            painter.drawText(textX + 1, textY + 1, qpText);
            painter.setPen(Qt::white);
            painter.drawText(textX, textY, qpText);
        }
    }

    // Draw QP statistics and legend in bottom-right corner
    int legendX = videoRect.right() - 180;
    int legendY = videoRect.bottom() - 120;

    // Draw semi-transparent background
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 200));
    painter.drawRect(legendX, legendY, 170, 110);

    // Draw title
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.setPen(Qt::white);
    painter.drawText(legendX + 10, legendY + 15, "QP Heatmap");

    // Draw statistics
    painter.setFont(QFont("Arial", 9));
    painter.drawText(legendX + 10, legendY + 35, QString("Min QP: %1").arg(minQP));
    painter.drawText(legendX + 10, legendY + 50, QString("Max QP: %1").arg(maxQP));
    painter.drawText(legendX + 10, legendY + 65, QString("Range: %1").arg(maxQP - minQP));

    // Draw color scale legend
    painter.setFont(QFont("Arial", 8));
    int colorBarY = legendY + 80;
    int colorBarWidth = 150;
    int colorBarHeight = 20;
    int colorBarX = legendX + 10;

    // Draw gradient color bar
    for (int i = 0; i < colorBarWidth; ++i) {
        int qp = 51 * i / colorBarWidth;  // Map position to QP range 0-51
        QColor color = getQPColor(qp);
        painter.fillRect(colorBarX + i, colorBarY, 1, colorBarHeight, color);
    }

    // Draw color bar border
    painter.setPen(Qt::white);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(colorBarX, colorBarY, colorBarWidth, colorBarHeight);

    // Draw scale labels
    painter.drawText(colorBarX - 5, colorBarY + colorBarHeight + 12, "0");
    painter.drawText(colorBarX + colorBarWidth / 2 - 10, colorBarY + colorBarHeight + 12, "25");
    painter.drawText(colorBarX + colorBarWidth - 15, colorBarY + colorBarHeight + 12, "51");

    // Draw quality indicators
    painter.setFont(QFont("Arial", 7));
    painter.setPen(QColor(50, 200, 50));
    painter.drawText(colorBarX, colorBarY - 5, "High Quality");
    painter.setPen(QColor(255, 50, 50));
    painter.drawText(colorBarX + colorBarWidth - 50, colorBarY - 5, "Low Quality");
}

QColor VideoOutput::getQPColor(int qp) const {
    // QP range: 0-51 (H.264/H.265)
    // Lower QP = Higher quality = Green
    // Higher QP = Lower quality = Red

    // Clamp QP to valid range
    qp = qMax(0, qMin(51, qp));

    // Map QP to color gradient: Green (QP=0) → Yellow (QP=25) → Red (QP=51)
    double normalizedQP = qp / 51.0;

    int r, g, b;

    if (normalizedQP < 0.5) {
        // Green to Yellow (QP 0-25)
        double t = normalizedQP * 2.0;
        r = static_cast<int>(255 * t);        // 0 → 255
        g = 255;                               // 255 (constant)
        b = 0;                                 // 0 (constant)
    } else {
        // Yellow to Red (QP 26-51)
        double t = (normalizedQP - 0.5) * 2.0;
        r = 255;                               // 255 (constant)
        g = static_cast<int>(255 * (1.0 - t)); // 255 → 0
        b = 0;                                 // 0 (constant)
    }

    return QColor(r, g, b);
}

} // namespace VideoStudio
