#include "widgets/barchart.h"
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>
#include <cmath>

namespace VideoStudio {

BarChart::BarChart(QWidget* parent)
    : QWidget(parent)
    , m_frameIndex(nullptr)
    , m_currentFrame(0)
    , m_maxFrameSize(0)
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

    int frameCount = m_frameIndex->frameCount();
    double barWidth = static_cast<double>(width()) / frameCount;

    // Draw bars for each frame
    for (int i = 0; i < frameCount; ++i) {
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
        int x = static_cast<int>(i * barWidth);
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
    if (m_currentFrame >= 0 && m_currentFrame < frameCount) {
        int x = static_cast<int>(m_currentFrame * barWidth);
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

    int frameCount = m_frameIndex->frameCount();
    double barWidth = static_cast<double>(width()) / frameCount;
    return static_cast<int>(x / barWidth);
}

} // namespace VideoStudio
