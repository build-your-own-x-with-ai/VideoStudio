#include "widgets/barchart.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QToolTip>
#include <cmath>

namespace VideoStudio {

BarChart::BarChart(QWidget* parent)
    : QWidget(parent)
    , m_frameIndex(nullptr)
    , m_currentFrame(0)
    , m_maxFrameSize(0)
    , m_viewStartFrame(0)
    , m_viewEndFrame(0)
    , m_zoomLevel(1.0)
{
    setMinimumHeight(100);
    setMaximumHeight(200);
    setMouseTracking(true);
}

BarChart::~BarChart() {
}

void BarChart::setFrameIndex(const FrameIndex* frameIndex) {
    m_frameIndex = frameIndex;
    if (m_frameIndex) {
        m_maxFrameSize = m_frameIndex->getMaxFrameSize();
        m_viewStartFrame = 0;
        m_viewEndFrame = m_frameIndex->frameCount() - 1;
    }
    update();
}

void BarChart::setCurrentFrame(int frameNumber) {
    m_currentFrame = frameNumber;
    update();
}

void BarChart::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    if (!m_frameIndex || m_frameIndex->frameCount() == 0) {
        return;
    }

    int visibleFrameCount = m_viewEndFrame - m_viewStartFrame + 1;
    double barWidth = static_cast<double>(width()) / visibleFrameCount;

    // Draw bars for each frame in view
    for (int i = m_viewStartFrame; i <= m_viewEndFrame; ++i) {
        const FrameInfo* frame = m_frameIndex->getFrame(i);
        if (!frame) continue;

        // Calculate bar height based on frame size
        int barHeight = 0;
        if (m_maxFrameSize > 0) {
            barHeight = static_cast<int>((static_cast<double>(frame->size) / m_maxFrameSize) * (height() - 20));
        }

        // Get color based on frame type
        QColor color = getFrameTypeColor(frame->frameType);

        // Highlight current frame
        if (i == m_currentFrame) {
            color = color.lighter(150);
        }

        // Draw bar
        int x = static_cast<int>((i - m_viewStartFrame) * barWidth);
        int y = height() - barHeight - 10;
        int w = std::max(1, static_cast<int>(std::ceil(barWidth)));

        painter.fillRect(x, y, w, barHeight, color);

        // Draw key frame indicator
        if (frame->isKeyFrame) {
            painter.setPen(Qt::yellow);
            painter.drawLine(x, y, x + w, y);
        }
    }

    // Draw current frame marker
    if (m_currentFrame >= m_viewStartFrame && m_currentFrame <= m_viewEndFrame) {
        int x = static_cast<int>((m_currentFrame - m_viewStartFrame) * barWidth);
        painter.setPen(QPen(Qt::red, 2));
        painter.drawLine(x, 0, x, height());
    }

    // Draw baseline
    painter.setPen(Qt::black);
    painter.drawLine(0, height() - 10, width(), height() - 10);
}

void BarChart::mousePressEvent(QMouseEvent* event) {
    if (!m_frameIndex || m_frameIndex->frameCount() == 0) {
        return;
    }

    int frameNumber = frameNumberAtPosition(event->pos().x());
    if (frameNumber >= 0 && frameNumber < m_frameIndex->frameCount()) {
        emit frameClicked(frameNumber);
    }
}

void BarChart::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

QColor BarChart::getFrameTypeColor(AVPictureType type) const {
    switch (type) {
        case AV_PICTURE_TYPE_I:
            return QColor(255, 100, 100); // Red for I-frames
        case AV_PICTURE_TYPE_P:
            return QColor(100, 100, 255); // Blue for P-frames
        case AV_PICTURE_TYPE_B:
            return QColor(100, 255, 100); // Green for B-frames
        default:
            return QColor(200, 200, 200); // Gray for others
    }
}

int BarChart::frameNumberAtPosition(int x) const {
    if (!m_frameIndex || m_frameIndex->frameCount() == 0) {
        return -1;
    }

    int visibleFrameCount = m_viewEndFrame - m_viewStartFrame + 1;
    double barWidth = static_cast<double>(width()) / visibleFrameCount;
    int relativeFrame = static_cast<int>(x / barWidth);
    return m_viewStartFrame + relativeFrame;
}

void BarChart::zoomIn() {
    if (!m_frameIndex) return;

    int centerFrame = (m_viewStartFrame + m_viewEndFrame) / 2;
    int currentRange = m_viewEndFrame - m_viewStartFrame + 1;
    int newRange = currentRange / 2;

    if (newRange < 10) newRange = 10;

    m_viewStartFrame = centerFrame - newRange / 2;
    m_viewEndFrame = centerFrame + newRange / 2;

    // Clamp to valid range
    if (m_viewStartFrame < 0) {
        m_viewStartFrame = 0;
        m_viewEndFrame = newRange - 1;
    }
    if (m_viewEndFrame >= m_frameIndex->frameCount()) {
        m_viewEndFrame = m_frameIndex->frameCount() - 1;
        m_viewStartFrame = m_viewEndFrame - newRange + 1;
        if (m_viewStartFrame < 0) m_viewStartFrame = 0;
    }

    update();
}

void BarChart::zoomOut() {
    if (!m_frameIndex) return;

    int centerFrame = (m_viewStartFrame + m_viewEndFrame) / 2;
    int currentRange = m_viewEndFrame - m_viewStartFrame + 1;
    int newRange = currentRange * 2;

    if (newRange > m_frameIndex->frameCount()) {
        newRange = m_frameIndex->frameCount();
    }

    m_viewStartFrame = centerFrame - newRange / 2;
    m_viewEndFrame = centerFrame + newRange / 2;

    // Clamp to valid range
    if (m_viewStartFrame < 0) {
        m_viewStartFrame = 0;
        m_viewEndFrame = newRange - 1;
    }
    if (m_viewEndFrame >= m_frameIndex->frameCount()) {
        m_viewEndFrame = m_frameIndex->frameCount() - 1;
        m_viewStartFrame = m_viewEndFrame - newRange + 1;
        if (m_viewStartFrame < 0) m_viewStartFrame = 0;
    }

    update();
}

void BarChart::zoomFit() {
    if (!m_frameIndex) return;

    m_viewStartFrame = 0;
    m_viewEndFrame = m_frameIndex->frameCount() - 1;
    update();
}

void BarChart::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0) {
        zoomIn();
    } else {
        zoomOut();
    }
}

} // namespace VideoStudio
