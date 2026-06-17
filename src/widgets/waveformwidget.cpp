#include "waveformwidget.h"
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>

namespace VideoStudio {

WaveformWidget::WaveformWidget(QWidget* parent)
    : QWidget(parent)
    , m_analyzer(std::make_unique<AudioAnalyzer>())
    , m_streamIndex(-1)
    , m_startTime(0.0)
    , m_endTime(10.0)
    , m_duration(0.0)
    , m_zoomLevel(1.0)
    , m_cursorPosition(-1)
    , m_isDragging(false)
    , m_dragStartX(0)
    , m_dragStartTime(0.0)
    , m_playbackCursorTime(-1.0)
    , m_showPlaybackCursor(false)
    , m_backgroundColor(QColor(30, 30, 30))
    , m_waveformColor(QColor(100, 200, 255))
    , m_centerLineColor(QColor(80, 80, 80))
    , m_gridColor(QColor(50, 50, 50))
    , m_cursorColor(QColor(255, 100, 100))
{
    setMouseTracking(true);
    setMinimumHeight(80);
}

WaveformWidget::~WaveformWidget() = default;

void WaveformWidget::setAudioFile(const QString& filename, int streamIndex) {
    m_filename = filename;
    m_streamIndex = streamIndex;

    if (filename.isEmpty()) {
        clear();
        return;
    }

    // Open audio file
    if (!m_analyzer->openFile(filename, streamIndex)) {
        qWarning() << "WaveformWidget: Failed to open audio file:" << filename;
        clear();
        return;
    }

    // Get audio info
    AudioStreamInfo info = m_analyzer->getStreamInfo();
    m_duration = info.duration / 1000000.0; // Convert to seconds

    // Set initial time range to show first 10 seconds or entire file
    m_startTime = 0.0;
    m_endTime = qMin(10.0, m_duration);

    // Generate waveform data
    generateWaveformData();

    update();
}

void WaveformWidget::setTimeRange(double startTime, double endTime) {
    m_startTime = qMax(0.0, startTime);
    m_endTime = qMin(m_duration, endTime);

    generateWaveformData();
    update();
}

void WaveformWidget::setZoomLevel(double zoom) {
    m_zoomLevel = qMax(0.1, qMin(10.0, zoom));
    generateWaveformData();
    update();
}

void WaveformWidget::setPlaybackCursor(double timeInSeconds) {
    m_playbackCursorTime = timeInSeconds;
    m_showPlaybackCursor = (timeInSeconds >= 0.0);

    // Auto-scroll to follow playback cursor
    if (m_showPlaybackCursor) {
        double viewDuration = m_endTime - m_startTime;

        // If cursor is outside visible range, scroll to center it
        if (timeInSeconds < m_startTime || timeInSeconds > m_endTime) {
            m_startTime = timeInSeconds - viewDuration / 2.0;
            m_endTime = timeInSeconds + viewDuration / 2.0;

            // Clamp to valid range
            if (m_startTime < 0.0) {
                m_startTime = 0.0;
                m_endTime = viewDuration;
            }
            if (m_endTime > m_duration) {
                m_endTime = m_duration;
                m_startTime = m_duration - viewDuration;
                if (m_startTime < 0.0) m_startTime = 0.0;
            }
        }
        // If cursor is near the end of visible range, scroll forward
        else if (timeInSeconds > m_startTime + viewDuration * 0.8) {
            double shift = viewDuration * 0.2;
            m_startTime += shift;
            m_endTime += shift;

            if (m_endTime > m_duration) {
                m_endTime = m_duration;
                m_startTime = m_duration - viewDuration;
                if (m_startTime < 0.0) m_startTime = 0.0;
            }
        }
    }

    update();
}

void WaveformWidget::clear() {
    m_filename.clear();
    m_waveformData.clear();
    m_duration = 0.0;
    m_startTime = 0.0;
    m_endTime = 10.0;
    m_cursorPosition = -1;
    m_playbackCursorTime = -1.0;
    m_showPlaybackCursor = false;
    update();
}

void WaveformWidget::generateWaveformData() {
    if (m_filename.isEmpty() || !m_analyzer->isOpen()) {
        m_waveformData.clear();
        return;
    }

    int numSamples = width();
    if (numSamples <= 0) {
        numSamples = 800; // Default width
    }

    // Get downsampled waveform data
    m_waveformData = m_analyzer->getWaveformData(m_startTime, m_endTime, numSamples);
}

void WaveformWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Fill background
    painter.fillRect(rect(), m_backgroundColor);

    if (m_waveformData.isEmpty()) {
        // Draw "No audio data" message
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No audio data"));
        return;
    }

    // Draw waveform
    drawWaveform(painter);

    // Draw time axis
    drawTimeAxis(painter);

    // Draw playback cursor (if active)
    if (m_showPlaybackCursor && m_playbackCursorTime >= m_startTime && m_playbackCursorTime <= m_endTime) {
        int playbackX = timeToPixel(m_playbackCursorTime);
        painter.setPen(QPen(QColor(255, 0, 0), 3));  // Red, thicker line
        painter.drawLine(playbackX, 0, playbackX, height() - 20);

        // Draw time label
        QString timeStr = QString::number(m_playbackCursorTime, 'f', 2) + "s";
        painter.fillRect(playbackX - 35, 5, 70, 20, QColor(255, 0, 0, 200));
        painter.setPen(Qt::white);
        painter.drawText(QRect(playbackX - 35, 5, 70, 20), Qt::AlignCenter, timeStr);
    }

    // Draw cursor
    if (m_cursorPosition >= 0) {
        drawCursor(painter);
    }
}

void WaveformWidget::drawWaveform(QPainter& painter) {
    int w = width();
    int h = height();
    int centerY = h / 2;

    // Draw center line
    painter.setPen(m_centerLineColor);
    painter.drawLine(0, centerY, w, centerY);

    // Draw grid lines (every 20 pixels vertically)
    painter.setPen(m_gridColor);
    for (int y = 20; y < h; y += 20) {
        painter.drawLine(0, y, w, y);
    }

    // Draw waveform
    painter.setPen(QPen(m_waveformColor, 1));

    int numSamples = m_waveformData.size();
    if (numSamples == 0) return;

    float scale = (h / 2) * 0.9f; // Leave 10% margin

    for (int x = 0; x < w && x < numSamples; ++x) {
        float sample = m_waveformData[x];
        int y1 = centerY - static_cast<int>(sample * scale);
        int y2 = centerY + static_cast<int>(sample * scale);

        painter.drawLine(x, y1, x, y2);
    }
}

void WaveformWidget::drawTimeAxis(QPainter& painter) {
    int w = width();
    int h = height();

    painter.setPen(Qt::white);

    // Draw time labels
    double timeRange = m_endTime - m_startTime;
    int numLabels = w / 100; // One label every 100 pixels
    if (numLabels < 2) numLabels = 2;

    for (int i = 0; i <= numLabels; ++i) {
        double time = m_startTime + (timeRange * i / numLabels);
        int x = static_cast<int>(w * i / numLabels);

        // Draw tick
        painter.drawLine(x, h - 15, x, h - 10);

        // Draw time label
        QString timeStr = QString::number(time, 'f', 2) + "s";
        QRect textRect(x - 30, h - 15, 60, 15);
        painter.drawText(textRect, Qt::AlignCenter, timeStr);
    }
}

void WaveformWidget::drawCursor(QPainter& painter) {
    painter.setPen(QPen(m_cursorColor, 2));
    painter.drawLine(m_cursorPosition, 0, m_cursorPosition, height() - 20);

    // Draw time at cursor
    double time = pixelToTime(m_cursorPosition);
    QString timeStr = QString::number(time, 'f', 3) + "s";

    painter.setPen(Qt::white);
    painter.fillRect(m_cursorPosition - 30, 5, 60, 20, QColor(0, 0, 0, 180));
    painter.drawText(QRect(m_cursorPosition - 30, 5, 60, 20), Qt::AlignCenter, timeStr);
}

void WaveformWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    generateWaveformData();
}

void WaveformWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_cursorPosition = event->pos().x();
        m_isDragging = true;
        m_dragStartX = event->pos().x();
        m_dragStartTime = m_startTime;

        double time = pixelToTime(m_cursorPosition);
        emit timePositionClicked(time);

        update();
    }
}

void WaveformWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging) {
        // Pan the waveform
        int dx = event->pos().x() - m_dragStartX;
        double timeRange = m_endTime - m_startTime;
        double timeDelta = -(dx / static_cast<double>(width())) * timeRange;

        double newStartTime = m_dragStartTime + timeDelta;
        double newEndTime = newStartTime + timeRange;

        // Clamp to valid range
        if (newStartTime < 0.0) {
            newStartTime = 0.0;
            newEndTime = timeRange;
        }
        if (newEndTime > m_duration) {
            newEndTime = m_duration;
            newStartTime = m_duration - timeRange;
        }

        setTimeRange(newStartTime, newEndTime);
        emit timeRangeChanged(m_startTime, m_endTime);
    } else {
        // Update cursor position
        m_cursorPosition = event->pos().x();
        update();
    }
}

void WaveformWidget::wheelEvent(QWheelEvent* event) {
    // Zoom in/out
    double zoomFactor = event->angleDelta().y() > 0 ? 0.9 : 1.1;

    double timeRange = m_endTime - m_startTime;
    double newTimeRange = timeRange * zoomFactor;

    // Clamp time range
    if (newTimeRange < 0.1) newTimeRange = 0.1;
    if (newTimeRange > m_duration) newTimeRange = m_duration;

    // Zoom around cursor position
    QPointF pos = event->position();
    double cursorTime = pixelToTime(pos.x());
    double ratio = (cursorTime - m_startTime) / timeRange;

    double newStartTime = cursorTime - newTimeRange * ratio;
    double newEndTime = newStartTime + newTimeRange;

    // Clamp to valid range
    if (newStartTime < 0.0) {
        newStartTime = 0.0;
        newEndTime = newTimeRange;
    }
    if (newEndTime > m_duration) {
        newEndTime = m_duration;
        newStartTime = m_duration - newTimeRange;
    }

    setTimeRange(newStartTime, newEndTime);
    emit timeRangeChanged(m_startTime, m_endTime);

    event->accept();
}

double WaveformWidget::pixelToTime(int x) const {
    double timeRange = m_endTime - m_startTime;
    return m_startTime + (x / static_cast<double>(width())) * timeRange;
}

int WaveformWidget::timeToPixel(double time) const {
    double timeRange = m_endTime - m_startTime;
    return static_cast<int>((time - m_startTime) / timeRange * width());
}

} // namespace VideoStudio
