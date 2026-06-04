#include "widgets/gopviewer.h"
#include "core/framedata.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <cmath>

namespace VideoStudio {

GOPViewer::GOPViewer(QWidget* parent)
    : QWidget(parent)
    , m_frameIndex(nullptr)
    , m_currentFrame(0)
    , m_viewStartFrame(0)
    , m_viewEndFrame(100)
    , m_framesPerRow(20)
    , m_frameWidth(40)
    , m_frameHeight(30)
    , m_horizontalSpacing(10)
    , m_verticalSpacing(40)
    , m_leftMargin(20)
    , m_topMargin(20)
    , m_autoScroll(true)
{
    setMinimumHeight(200);
    setMouseTracking(true);
}

GOPViewer::~GOPViewer() {
}

void GOPViewer::setFrameIndex(const FrameIndex* frameIndex) {
    m_frameIndex = frameIndex;
    if (m_frameIndex) {
        // Reset view to start from frame 0
        m_viewStartFrame = 0;
        m_viewEndFrame = qMin(100, m_frameIndex->frameCount() - 1);
        analyzeGOPStructure();
    }
    update();
}

void GOPViewer::setCurrentFrame(int frameNumber) {
    m_currentFrame = frameNumber;

    // Auto-scroll to show current frame if it's outside the visible range
    // Only auto-scroll if the flag is enabled (not during user interaction)
    if (m_autoScroll && m_frameIndex && frameNumber >= 0) {
        if (frameNumber < m_viewStartFrame || frameNumber > m_viewEndFrame) {
            // Center the view around the current frame
            int visibleFrames = m_viewEndFrame - m_viewStartFrame + 1;
            m_viewStartFrame = qMax(0, frameNumber - visibleFrames / 2);
            m_viewEndFrame = qMin(m_frameIndex->frameCount() - 1, m_viewStartFrame + visibleFrames - 1);

            // Adjust if we hit the end
            if (m_viewEndFrame == m_frameIndex->frameCount() - 1) {
                m_viewStartFrame = qMax(0, m_viewEndFrame - visibleFrames + 1);
            }
        }
    }

    update();
}

void GOPViewer::setDuplicateFrames(const QSet<int>& duplicateFrames) {
    m_duplicateFrames = duplicateFrames;
    update();
}

void GOPViewer::clear() {
    m_frameIndex = nullptr;
    m_gops.clear();
    m_currentFrame = 0;
    m_viewStartFrame = 0;
    m_viewEndFrame = 100;
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
    // Calculate frames per row based on widget width
    int availableWidth = width() - 2 * m_leftMargin;
    m_framesPerRow = qMax(10, availableWidth / (m_frameWidth + m_horizontalSpacing));

    // Draw frames
    for (int i = m_viewStartFrame; i <= m_viewEndFrame && i < m_frameIndex->frameCount(); ++i) {
        const FrameInfo* frame = m_frameIndex->getFrame(i);
        if (!frame) continue;

        int x = frameToX(i);
        int y = frameToY(i);

        drawFrame(painter, i, x, y, m_frameWidth, m_frameHeight);
    }

    // Draw dependency arrows
    painter.setOpacity(0.5);
    for (int i = m_viewStartFrame; i <= m_viewEndFrame && i < m_frameIndex->frameCount(); ++i) {
        const FrameInfo* frame = m_frameIndex->getFrame(i);
        if (!frame) continue;

        if (frame->frameType == AV_PICTURE_TYPE_P) {
            // P-frame depends on previous I or P frame
            for (int j = i - 1; j >= m_viewStartFrame; --j) {
                const FrameInfo* refFrame = m_frameIndex->getFrame(j);
                if (refFrame && (refFrame->frameType == AV_PICTURE_TYPE_I ||
                                 refFrame->frameType == AV_PICTURE_TYPE_P)) {
                    int fromX = frameToX(j) + m_frameWidth / 2;
                    int fromY = frameToY(j) + m_frameHeight;
                    int toX = frameToX(i) + m_frameWidth / 2;
                    int toY = frameToY(i);
                    drawDependencyArrow(painter, fromX, fromY, toX, toY);
                    break;
                }
            }
        } else if (frame->frameType == AV_PICTURE_TYPE_B) {
            // B-frame depends on surrounding I/P frames
            // Find previous reference
            for (int j = i - 1; j >= m_viewStartFrame; --j) {
                const FrameInfo* refFrame = m_frameIndex->getFrame(j);
                if (refFrame && (refFrame->frameType == AV_PICTURE_TYPE_I ||
                                 refFrame->frameType == AV_PICTURE_TYPE_P)) {
                    int fromX = frameToX(j) + m_frameWidth / 2;
                    int fromY = frameToY(j) + m_frameHeight;
                    int toX = frameToX(i) + m_frameWidth / 2;
                    int toY = frameToY(i);
                    drawDependencyArrow(painter, fromX, fromY, toX, toY);
                    break;
                }
            }
            // Find next reference
            for (int j = i + 1; j <= m_viewEndFrame && j < m_frameIndex->frameCount(); ++j) {
                const FrameInfo* refFrame = m_frameIndex->getFrame(j);
                if (refFrame && (refFrame->frameType == AV_PICTURE_TYPE_I ||
                                 refFrame->frameType == AV_PICTURE_TYPE_P)) {
                    int fromX = frameToX(j) + m_frameWidth / 2;
                    int fromY = frameToY(j) + m_frameHeight;
                    int toX = frameToX(i) + m_frameWidth / 2;
                    int toY = frameToY(i);
                    drawDependencyArrow(painter, fromX, fromY, toX, toY);
                    break;
                }
            }
        }
    }
    painter.setOpacity(1.0);

    // Draw scroll indicator at the bottom
    if (m_frameIndex && m_frameIndex->frameCount() > 0) {
        QString scrollInfo = QString("Showing frames %1-%2 of %3 (Use mouse wheel to scroll)")
            .arg(m_viewStartFrame)
            .arg(qMin(m_viewEndFrame, m_frameIndex->frameCount() - 1))
            .arg(m_frameIndex->frameCount());

        painter.setPen(QColor(150, 150, 150));
        painter.setFont(QFont("Arial", 9));
        painter.drawText(QRect(10, height() - 20, width() - 20, 15), Qt::AlignCenter, scrollInfo);
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

    // Draw duplicate marker overlay if this is a duplicate frame
    if (m_duplicateFrames.contains(frameNumber) && frameNumber != m_currentFrame) {
        // Draw a small "D" badge in the corner
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 165, 0));
        painter.drawEllipse(x + w - 12, y + 2, 10, 10);
        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 7, QFont::Bold));
        painter.drawText(QRect(x + w - 12, y + 2, 10, 10), Qt::AlignCenter, "D");
    }

    // Draw frame type label
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(QRect(x, y, w, h), Qt::AlignCenter, frameLabel);

    // Draw frame number below
    painter.setFont(QFont("Arial", 8));
    painter.drawText(QRect(x, y + h + 2, w, 15), Qt::AlignCenter, QString::number(frameNumber));
}

void GOPViewer::drawDependencyArrow(QPainter& painter, int fromX, int fromY, int toX, int toY) {
    painter.setPen(QPen(QColor(150, 150, 150, 150), 1));

    // Draw curved arrow
    QPainterPath path;
    path.moveTo(fromX, fromY);

    // Calculate control point for bezier curve
    int midY = (fromY + toY) / 2;
    path.quadTo(fromX, midY, toX, toY);

    painter.drawPath(path);

    // Draw arrowhead
    double angle = std::atan2(toY - midY, toX - fromX);
    int arrowSize = 5;
    QPointF p1(toX - arrowSize * std::cos(angle - M_PI / 6),
               toY - arrowSize * std::sin(angle - M_PI / 6));
    QPointF p2(toX - arrowSize * std::cos(angle + M_PI / 6),
               toY - arrowSize * std::sin(angle + M_PI / 6));
    painter.drawLine(QPointF(toX, toY), p1);
    painter.drawLine(QPointF(toX, toY), p2);
}

int GOPViewer::frameToX(int frameNumber) const {
    int col = (frameNumber - m_viewStartFrame) % m_framesPerRow;
    return m_leftMargin + col * (m_frameWidth + m_horizontalSpacing);
}

int GOPViewer::frameToY(int frameNumber) const {
    int row = (frameNumber - m_viewStartFrame) / m_framesPerRow;
    return m_topMargin + row * (m_frameHeight + m_verticalSpacing);
}

int GOPViewer::pixelToFrame(int x, int y) const {
    int col = (x - m_leftMargin) / (m_frameWidth + m_horizontalSpacing);
    int row = (y - m_topMargin) / (m_frameHeight + m_verticalSpacing);

    // Check if click is within frame bounds
    int frameX = m_leftMargin + col * (m_frameWidth + m_horizontalSpacing);
    int frameY = m_topMargin + row * (m_frameHeight + m_verticalSpacing);

    if (x < frameX || x > frameX + m_frameWidth ||
        y < frameY || y > frameY + m_frameHeight) {
        return -1; // Click was in spacing area
    }

    int frameNumber = m_viewStartFrame + row * m_framesPerRow + col;
    return frameNumber;
}

void GOPViewer::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int frameNumber = pixelToFrame(event->pos().x(), event->pos().y());
        if (frameNumber >= m_viewStartFrame && frameNumber <= m_viewEndFrame && frameNumber >= 0) {
            if (m_frameIndex && frameNumber < m_frameIndex->frameCount()) {
                // Temporarily disable auto-scroll when user clicks a frame
                m_autoScroll = false;
                emit frameClicked(frameNumber);
                // Re-enable auto-scroll after a short delay
                QTimer::singleShot(100, this, [this]() {
                    m_autoScroll = true;
                });
            }
        }
    }
}

void GOPViewer::wheelEvent(QWheelEvent* event) {
    if (!m_frameIndex) {
        return;
    }

    int delta = event->angleDelta().y();
    int scrollAmount = 5;

    if (delta > 0) {
        // Scroll up (show earlier frames)
        m_viewStartFrame = qMax(0, m_viewStartFrame - scrollAmount);
        m_viewEndFrame = qMax(m_viewStartFrame + 100, m_viewEndFrame - scrollAmount);
    } else {
        // Scroll down (show later frames)
        m_viewStartFrame = qMin(m_frameIndex->frameCount() - 1, m_viewStartFrame + scrollAmount);
        m_viewEndFrame = qMin(m_frameIndex->frameCount() - 1, m_viewEndFrame + scrollAmount);
    }

    update();
}

void GOPViewer::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

} // namespace VideoStudio
