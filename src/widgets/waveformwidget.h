#ifndef WAVEFORMWIDGET_H
#define WAVEFORMWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPainter>
#include <memory>
#include "core/audioanalyzer.h"

namespace VideoStudio {

class WaveformWidget : public QWidget {
    Q_OBJECT

public:
    explicit WaveformWidget(QWidget* parent = nullptr);
    ~WaveformWidget() override;

    // Set audio file to display
    void setAudioFile(const QString& filename, int streamIndex = -1);

    // Set time range to display
    void setTimeRange(double startTime, double endTime);

    // Set zoom level (samples per pixel)
    void setZoomLevel(double zoom);

    // Set playback cursor position (for sync with player)
    void setPlaybackCursor(double timeInSeconds);

    // Clear waveform
    void clear();

    QSize sizeHint() const override { return QSize(800, 200); }
    QSize minimumSizeHint() const override { return QSize(400, 100); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

signals:
    void timePositionClicked(double time);
    void timeRangeChanged(double startTime, double endTime);

private:
    void generateWaveformData();
    void drawWaveform(QPainter& painter);
    void drawTimeAxis(QPainter& painter);
    void drawCursor(QPainter& painter);

    double pixelToTime(int x) const;
    int timeToPixel(double time) const;

    std::unique_ptr<AudioAnalyzer> m_analyzer;
    QString m_filename;
    int m_streamIndex;

    // Waveform data (downsampled for display)
    QVector<float> m_waveformData;

    // Display settings
    double m_startTime;      // Start time in seconds
    double m_endTime;        // End time in seconds
    double m_duration;       // Total duration
    double m_zoomLevel;      // Samples per pixel

    // Interaction
    int m_cursorPosition;    // Pixel position of cursor
    bool m_isDragging;
    int m_dragStartX;
    double m_dragStartTime;

    // Playback cursor
    double m_playbackCursorTime;  // Time in seconds for playback cursor
    bool m_showPlaybackCursor;

    // Colors
    QColor m_backgroundColor;
    QColor m_waveformColor;
    QColor m_centerLineColor;
    QColor m_gridColor;
    QColor m_cursorColor;
};

} // namespace VideoStudio

#endif // WAVEFORMWIDGET_H
