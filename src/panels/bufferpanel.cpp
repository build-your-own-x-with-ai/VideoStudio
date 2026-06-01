#include "panels/bufferpanel.h"
#include "core/videodecoder.h"
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>
#include <cmath>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace VideoStudio {

BufferPanel::BufferPanel(QWidget* parent)
    : QWidget(parent)
    , m_decoder(nullptr)
    , m_maxBufferSize(0)
    , m_maxOccupancy(0)
    , m_scaleMode(ScaleMode::Defined)
    , m_horizontalScale(1.0)
    , m_mouseInChart(false)
    , m_hoveredFrame(-1)
{
    setMinimumHeight(200);
    setMouseTracking(true);
}

BufferPanel::~BufferPanel() = default;

void BufferPanel::setDecoder(VideoDecoder* decoder) {
    m_decoder = decoder;
    updateBufferData();
}

void BufferPanel::clear() {
    m_bufferData.clear();
    m_maxBufferSize = 0;
    m_maxOccupancy = 0;
    update();
}

void BufferPanel::setVerticalScaleMode(ScaleMode mode) {
    m_scaleMode = mode;
    update();
}

void BufferPanel::setHorizontalScale(double scale) {
    m_horizontalScale = qBound(0.1, scale, 10.0);
    update();
}

void BufferPanel::updateBufferData() {
    m_bufferData.clear();

    if (!m_decoder || !m_decoder->isOpen()) {
        update();
        return;
    }

    // Estimate buffer size from codec parameters
    m_maxBufferSize = estimateBufferSize();

    // Calculate buffer occupancy for each frame
    int frameCount = m_decoder->getFrameCount();
    m_maxOccupancy = 0;

    int64_t currentOccupancy = 0;
    for (int i = 0; i < frameCount; i++) {
        // Get frame size
        int64_t frameSize = m_decoder->getFrameSize(i);

        // Add frame to buffer
        currentOccupancy += frameSize * 8; // Convert to bits

        // Remove decoded data (simplified model: constant bitrate removal)
        double fps = m_decoder->getFrameRate();
        int64_t bitrate = m_decoder->getBitrate();
        if (fps > 0 && bitrate > 0) {
            int64_t bitsPerFrame = bitrate / fps;
            currentOccupancy -= bitsPerFrame;
        }

        // Clamp to valid range
        currentOccupancy = qMax(0LL, currentOccupancy);

        m_bufferData.append(qMakePair(i, currentOccupancy));
        m_maxOccupancy = qMax(m_maxOccupancy, currentOccupancy);
    }

    update();
}

int64_t BufferPanel::estimateBufferSize() const {
    if (!m_decoder || !m_decoder->isOpen()) {
        return 0;
    }

    // Try to get CPB size from codec context
    AVCodecContext* codecCtx = m_decoder->getCodecContext();
    if (codecCtx && codecCtx->rc_buffer_size > 0) {
        return codecCtx->rc_buffer_size * 8; // Convert to bits
    }

    // Estimate from bitrate and level
    int64_t bitrate = m_decoder->getBitrate();
    if (bitrate > 0) {
        // Use 2 seconds of video as buffer size (typical for streaming)
        return bitrate * 2;
    }

    // Default fallback
    return 10 * 1024 * 1024 * 8; // 10 MB in bits
}

void BufferPanel::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(rect(), QColor(30, 30, 30));

    if (m_bufferData.isEmpty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No buffer data available"));
        return;
    }

    drawChart(painter);

    if (m_mouseInChart && m_hoveredFrame >= 0) {
        drawTooltip(painter);
    }
}

void BufferPanel::drawChart(QPainter& painter) {
    const int margin = 50;
    QRect chartRect = rect().adjusted(margin, margin, -margin, -margin);

    // Draw grid
    drawGrid(painter, chartRect);

    // Draw buffer curve
    drawBufferCurve(painter, chartRect);

    // Draw axes labels
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    // Y-axis label
    painter.save();
    painter.translate(10, chartRect.center().y());
    painter.rotate(-90);
    painter.drawText(0, 0, tr("Buffer Occupancy (Mbits)"));
    painter.restore();

    // X-axis label
    painter.drawText(chartRect.center().x() - 50, height() - 10, tr("Frame Number"));

    // Draw scale values
    int64_t maxScale = (m_scaleMode == ScaleMode::Defined) ? m_maxBufferSize : m_maxOccupancy;
    if (maxScale > 0) {
        for (int i = 0; i <= 4; i++) {
            int64_t value = (maxScale * i) / 4;
            double mbits = value / (1024.0 * 1024.0);
            int y = chartRect.bottom() - (chartRect.height() * i) / 4;
            painter.drawText(5, y + 5, QString::number(mbits, 'f', 1));
        }
    }
}

void BufferPanel::drawGrid(QPainter& painter, const QRect& chartRect) {
    painter.setPen(QPen(QColor(60, 60, 60), 1));

    // Horizontal grid lines
    for (int i = 0; i <= 4; i++) {
        int y = chartRect.bottom() - (chartRect.height() * i) / 4;
        painter.drawLine(chartRect.left(), y, chartRect.right(), y);
    }

    // Vertical grid lines
    int frameCount = m_bufferData.size();
    int step = qMax(1, frameCount / 10);
    for (int i = 0; i <= frameCount; i += step) {
        int x = chartRect.left() + (chartRect.width() * i * m_horizontalScale) / frameCount;
        if (x <= chartRect.right()) {
            painter.drawLine(x, chartRect.top(), x, chartRect.bottom());
        }
    }
}

void BufferPanel::drawBufferCurve(QPainter& painter, const QRect& chartRect) {
    if (m_bufferData.isEmpty()) return;

    int64_t maxScale = (m_scaleMode == ScaleMode::Defined) ? m_maxBufferSize : m_maxOccupancy;
    if (maxScale == 0) return;

    // Draw buffer size limit line (if in Defined mode)
    if (m_scaleMode == ScaleMode::Defined && m_maxBufferSize > 0) {
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        int y = chartRect.bottom() - (chartRect.height() * m_maxBufferSize) / maxScale;
        painter.drawLine(chartRect.left(), y, chartRect.right(), y);
    }

    // Draw buffer occupancy curve
    painter.setPen(QPen(QColor(100, 200, 255), 2));

    QVector<QPointF> points;
    for (int i = 0; i < m_bufferData.size(); i++) {
        int frameIndex = m_bufferData[i].first;
        int64_t occupancy = m_bufferData[i].second;

        double x = chartRect.left() + (chartRect.width() * frameIndex * m_horizontalScale) / m_bufferData.size();
        double y = chartRect.bottom() - (chartRect.height() * occupancy) / maxScale;

        if (x <= chartRect.right()) {
            points.append(QPointF(x, y));
        }
    }

    if (points.size() > 1) {
        painter.drawPolyline(points.data(), points.size());
    }

    // Fill area under curve
    if (points.size() > 1) {
        QPolygonF polygon;
        polygon << QPointF(points.first().x(), chartRect.bottom());
        polygon << points;
        polygon << QPointF(points.last().x(), chartRect.bottom());

        painter.setBrush(QColor(100, 200, 255, 50));
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(polygon);
    }
}

void BufferPanel::drawTooltip(QPainter& painter) {
    if (m_hoveredFrame < 0 || m_hoveredFrame >= m_bufferData.size()) return;

    int64_t occupancy = m_bufferData[m_hoveredFrame].second;
    double mbits = occupancy / (1024.0 * 1024.0);
    double percent = (m_maxBufferSize > 0) ? (100.0 * occupancy / m_maxBufferSize) : 0.0;

    QString text = tr("Frame: %1\nOccupancy: %2 Mbits (%3%)")
        .arg(m_hoveredFrame)
        .arg(mbits, 0, 'f', 2)
        .arg(percent, 0, 'f', 1);

    QFontMetrics fm(font());
    QRect textRect = fm.boundingRect(QRect(), Qt::AlignLeft, text);
    textRect.adjust(-5, -5, 5, 5);
    textRect.moveTopLeft(m_mousePos + QPoint(10, 10));

    // Keep tooltip in widget bounds
    if (textRect.right() > width()) {
        textRect.moveRight(width() - 5);
    }
    if (textRect.bottom() > height()) {
        textRect.moveBottom(height() - 5);
    }

    painter.setBrush(QColor(50, 50, 50, 230));
    painter.setPen(Qt::white);
    painter.drawRect(textRect);
    painter.drawText(textRect, Qt::AlignCenter, text);
}

void BufferPanel::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

void BufferPanel::mouseMoveEvent(QMouseEvent* event) {
    m_mousePos = event->pos();

    const int margin = 50;
    QRect chartRect = rect().adjusted(margin, margin, -margin, -margin);

    m_mouseInChart = chartRect.contains(m_mousePos);

    if (m_mouseInChart && !m_bufferData.isEmpty()) {
        // Calculate hovered frame
        int relativeX = m_mousePos.x() - chartRect.left();
        int frameCount = m_bufferData.size();
        m_hoveredFrame = (relativeX * frameCount) / (chartRect.width() * m_horizontalScale);
        m_hoveredFrame = qBound(0, m_hoveredFrame, frameCount - 1);
    } else {
        m_hoveredFrame = -1;
    }

    update();
}

} // namespace VideoStudio
