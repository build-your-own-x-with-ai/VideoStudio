#include "widgets/areachart.h"
#include "core/framedata.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>

namespace VideoStudio {

AreaChart::AreaChart(QWidget* parent)
    : QWidget(parent)
    , m_frameIndex(nullptr)
    , m_minFrame(0)
    , m_maxFrame(100)
    , m_minValue(0.0)
    , m_maxValue(100.0)
    , m_autoScale(true)
    , m_isDragging(false)
    , m_leftMargin(60)
    , m_rightMargin(20)
    , m_topMargin(20)
    , m_bottomMargin(40)
{
    setMinimumHeight(200);
    setMouseTracking(true);
}

AreaChart::~AreaChart() {
}

void AreaChart::setFrameIndex(const FrameIndex* frameIndex) {
    m_frameIndex = frameIndex;

    if (m_frameIndex) {
        m_maxFrame = m_frameIndex->frameCount() - 1;

        // Add default series for frame types
        addSeries("I-frames", QColor(255, 100, 100));
        addSeries("P-frames", QColor(100, 100, 255));
        addSeries("B-frames", QColor(100, 255, 100));

        updateData();
    }
}

void AreaChart::addSeries(const QString& name, const QColor& color) {
    DataSeries series;
    series.name = name;
    series.color = color;
    series.visible = true;
    m_series[name] = series;
}

void AreaChart::setSeriesVisible(const QString& name, bool visible) {
    if (m_series.contains(name)) {
        m_series[name].visible = visible;
        update();
    }
}

void AreaChart::clear() {
    m_series.clear();
    m_frameIndex = nullptr;
    update();
}

void AreaChart::setXRange(int minFrame, int maxFrame) {
    m_minFrame = qMax(0, minFrame);
    m_maxFrame = qMin(m_frameIndex ? m_frameIndex->frameCount() - 1 : 100, maxFrame);
    update();
    emit rangeChanged(m_minFrame, m_maxFrame);
}

void AreaChart::setYRange(double minValue, double maxValue) {
    m_minValue = minValue;
    m_maxValue = maxValue;
    m_autoScale = false;
    update();
}

void AreaChart::setAutoScale(bool enabled) {
    m_autoScale = enabled;
    if (enabled) {
        updateData();
    }
}

void AreaChart::updateData() {
    if (!m_frameIndex) {
        return;
    }

    // Clear existing data
    for (auto& series : m_series) {
        series.values.clear();
    }

    // Populate series data
    int frameCount = m_frameIndex->frameCount();
    double maxVal = 0.0;

    for (int i = 0; i < frameCount; ++i) {
        const FrameInfo* frame = m_frameIndex->getFrame(i);
        if (!frame) continue;

        double iValue = 0.0, pValue = 0.0, bValue = 0.0;

        switch (frame->frameType) {
            case AV_PICTURE_TYPE_I:
                iValue = frame->size;
                break;
            case AV_PICTURE_TYPE_P:
                pValue = frame->size;
                break;
            case AV_PICTURE_TYPE_B:
                bValue = frame->size;
                break;
            default:
                break;
        }

        if (m_series.contains("I-frames")) {
            m_series["I-frames"].values.append(iValue);
        }
        if (m_series.contains("P-frames")) {
            m_series["P-frames"].values.append(pValue);
        }
        if (m_series.contains("B-frames")) {
            m_series["B-frames"].values.append(bValue);
        }

        maxVal = qMax(maxVal, qMax(iValue, qMax(pValue, bValue)));
    }

    // Auto-scale Y axis
    if (m_autoScale && maxVal > 0) {
        m_minValue = 0.0;
        m_maxValue = maxVal * 1.1; // Add 10% padding
    }

    update();
}

void AreaChart::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Fill background
    painter.fillRect(rect(), QColor(40, 40, 40));

    if (!m_frameIndex || m_series.isEmpty()) {
        return;
    }

    drawGrid(painter);
    drawAxes(painter);
    drawSeries(painter);
    drawLegend(painter);
}

void AreaChart::drawGrid(QPainter& painter) {
    painter.setPen(QPen(QColor(60, 60, 60), 1));

    int chartWidth = width() - m_leftMargin - m_rightMargin;
    int chartHeight = height() - m_topMargin - m_bottomMargin;

    // Horizontal grid lines
    for (int i = 0; i <= 5; ++i) {
        int y = m_topMargin + (chartHeight * i) / 5;
        painter.drawLine(m_leftMargin, y, m_leftMargin + chartWidth, y);
    }

    // Vertical grid lines
    for (int i = 0; i <= 10; ++i) {
        int x = m_leftMargin + (chartWidth * i) / 10;
        painter.drawLine(x, m_topMargin, x, m_topMargin + chartHeight);
    }
}

void AreaChart::drawAxes(QPainter& painter) {
    painter.setPen(QPen(Qt::white, 1));
    painter.setFont(QFont("Arial", 9));

    int chartWidth = width() - m_leftMargin - m_rightMargin;
    int chartHeight = height() - m_topMargin - m_bottomMargin;

    // Y-axis labels
    for (int i = 0; i <= 5; ++i) {
        int y = m_topMargin + (chartHeight * i) / 5;
        double value = m_maxValue - (m_maxValue - m_minValue) * i / 5.0;

        QString label;
        if (value >= 1000000) {
            label = QString::number(value / 1000000.0, 'f', 1) + "M";
        } else if (value >= 1000) {
            label = QString::number(value / 1000.0, 'f', 1) + "K";
        } else {
            label = QString::number(value, 'f', 0);
        }

        painter.drawText(QRect(5, y - 10, m_leftMargin - 10, 20),
                        Qt::AlignRight | Qt::AlignVCenter, label);
    }

    // X-axis labels
    for (int i = 0; i <= 10; ++i) {
        int x = m_leftMargin + (chartWidth * i) / 10;
        int frame = m_minFrame + (m_maxFrame - m_minFrame) * i / 10;
        painter.drawText(QRect(x - 30, height() - m_bottomMargin + 5, 60, 20),
                        Qt::AlignCenter, QString::number(frame));
    }

    // Axis labels
    painter.drawText(QRect(0, height() / 2 - 50, m_leftMargin - 5, 100),
                    Qt::AlignRight | Qt::AlignVCenter, "Size (bytes)");
    painter.drawText(QRect(width() / 2 - 50, height() - 20, 100, 20),
                    Qt::AlignCenter, "Frame");
}

void AreaChart::drawSeries(QPainter& painter) {
    int chartWidth = width() - m_leftMargin - m_rightMargin;
    int chartHeight = height() - m_topMargin - m_bottomMargin;

    painter.setClipRect(m_leftMargin, m_topMargin, chartWidth, chartHeight);

    // Draw each series as filled area
    for (const auto& series : m_series) {
        if (!series.visible || series.values.isEmpty()) {
            continue;
        }

        QPolygonF polygon;

        // Add bottom-left corner
        polygon << QPointF(m_leftMargin, m_topMargin + chartHeight);

        // Add data points
        int visibleFrames = m_maxFrame - m_minFrame + 1;
        for (int i = m_minFrame; i <= m_maxFrame && i < series.values.size(); ++i) {
            QPoint pt = frameToPixel(i, series.values[i]);
            polygon << QPointF(pt.x(), pt.y());
        }

        // Add bottom-right corner
        polygon << QPointF(m_leftMargin + chartWidth, m_topMargin + chartHeight);

        // Fill area
        QColor fillColor = series.color;
        fillColor.setAlpha(100);
        painter.setBrush(fillColor);
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(polygon);

        // Draw line
        painter.setPen(QPen(series.color, 2));
        painter.setBrush(Qt::NoBrush);
        for (int i = 1; i < polygon.size() - 1; ++i) {
            painter.drawLine(polygon[i - 1], polygon[i]);
        }
    }

    painter.setClipping(false);
}

void AreaChart::drawLegend(QPainter& painter) {
    int x = width() - m_rightMargin - 150;
    int y = m_topMargin + 10;

    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 9));

    for (const auto& series : m_series) {
        if (!series.visible) {
            continue;
        }

        // Draw color box
        painter.fillRect(x, y, 15, 15, series.color);
        painter.drawRect(x, y, 15, 15);

        // Draw name
        painter.drawText(x + 20, y + 12, series.name);

        y += 20;
    }
}

QPoint AreaChart::frameToPixel(int frame, double value) const {
    int chartWidth = width() - m_leftMargin - m_rightMargin;
    int chartHeight = height() - m_topMargin - m_bottomMargin;

    int x = m_leftMargin + (frame - m_minFrame) * chartWidth / (m_maxFrame - m_minFrame + 1);

    double normalizedValue = (value - m_minValue) / (m_maxValue - m_minValue);
    int y = m_topMargin + chartHeight - static_cast<int>(normalizedValue * chartHeight);

    return QPoint(x, y);
}

int AreaChart::pixelToFrame(int x) const {
    int chartWidth = width() - m_leftMargin - m_rightMargin;
    int relativeX = x - m_leftMargin;
    int frame = m_minFrame + (relativeX * (m_maxFrame - m_minFrame + 1)) / chartWidth;
    return qBound(m_minFrame, frame, m_maxFrame);
}

void AreaChart::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragStart = event->pos();
        m_dragStartMinFrame = m_minFrame;
        m_dragStartMaxFrame = m_maxFrame;
    }
}

void AreaChart::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging) {
        int dx = event->pos().x() - m_dragStart.x();
        int chartWidth = width() - m_leftMargin - m_rightMargin;
        int frameDelta = -(dx * (m_maxFrame - m_minFrame + 1)) / chartWidth;

        int newMin = m_dragStartMinFrame + frameDelta;
        int newMax = m_dragStartMaxFrame + frameDelta;

        if (newMin >= 0 && newMax < (m_frameIndex ? m_frameIndex->frameCount() : 100)) {
            setXRange(newMin, newMax);
        }
    }
}

void AreaChart::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
}

void AreaChart::wheelEvent(QWheelEvent* event) {
    if (!m_frameIndex) {
        return;
    }

    int delta = event->angleDelta().y();
    double zoomFactor = delta > 0 ? 0.9 : 1.1;

    int range = m_maxFrame - m_minFrame + 1;
    int newRange = static_cast<int>(range * zoomFactor);
    newRange = qMax(10, qMin(newRange, m_frameIndex->frameCount()));

    int center = (m_minFrame + m_maxFrame) / 2;
    int newMin = center - newRange / 2;
    int newMax = center + newRange / 2;

    if (newMin < 0) {
        newMin = 0;
        newMax = newRange - 1;
    }
    if (newMax >= m_frameIndex->frameCount()) {
        newMax = m_frameIndex->frameCount() - 1;
        newMin = newMax - newRange + 1;
    }

    setXRange(newMin, newMax);
}

void AreaChart::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

void AreaChart::zoomIn() {
    if (!m_frameIndex) return;

    int range = m_maxFrame - m_minFrame + 1;
    int newRange = range / 2;
    if (newRange < 10) newRange = 10;

    int center = (m_minFrame + m_maxFrame) / 2;
    int newMin = center - newRange / 2;
    int newMax = center + newRange / 2;

    if (newMin < 0) {
        newMin = 0;
        newMax = newRange - 1;
    }
    if (newMax >= m_frameIndex->frameCount()) {
        newMax = m_frameIndex->frameCount() - 1;
        newMin = newMax - newRange + 1;
        if (newMin < 0) newMin = 0;
    }

    setXRange(newMin, newMax);
}

void AreaChart::zoomOut() {
    if (!m_frameIndex) return;

    int range = m_maxFrame - m_minFrame + 1;
    int newRange = range * 2;
    if (newRange > m_frameIndex->frameCount()) {
        newRange = m_frameIndex->frameCount();
    }

    int center = (m_minFrame + m_maxFrame) / 2;
    int newMin = center - newRange / 2;
    int newMax = center + newRange / 2;

    if (newMin < 0) {
        newMin = 0;
        newMax = newRange - 1;
    }
    if (newMax >= m_frameIndex->frameCount()) {
        newMax = m_frameIndex->frameCount() - 1;
        newMin = newMax - newRange + 1;
        if (newMin < 0) newMin = 0;
    }

    setXRange(newMin, newMax);
}

void AreaChart::zoomFit() {
    if (!m_frameIndex) return;
    setXRange(0, m_frameIndex->frameCount() - 1);
}

} // namespace VideoStudio
