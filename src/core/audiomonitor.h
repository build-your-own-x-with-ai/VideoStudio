#ifndef AUDIOMONITOR_H
#define AUDIOMONITOR_H

#include <QObject>
#include <QTimer>
#include <memory>
#include "audioanalyzer.h"

namespace VideoStudio {

// Forward declarations
class AudioLevelWidget;
class SpectrumWidget;
class WaveformWidget;

/**
 * AudioMonitor - Real-time audio monitoring during playback
 *
 * This class bridges the video playback system with audio visualization widgets.
 * It processes audio frames in real-time and updates level meters, spectrum analyzer,
 * and waveform displays.
 */
class AudioMonitor : public QObject {
    Q_OBJECT

public:
    explicit AudioMonitor(QObject* parent = nullptr);
    ~AudioMonitor() override;

    // Set audio file to monitor
    bool setAudioFile(const QString& filename, int streamIndex = -1);

    // Connect visualization widgets
    void setLevelMeterLeft(AudioLevelWidget* widget);
    void setLevelMeterRight(AudioLevelWidget* widget);
    void setSpectrumWidget(SpectrumWidget* widget);
    void setWaveformWidget(WaveformWidget* widget);

    // Control monitoring
    void start();
    void stop();
    void pause();
    void resume();

    // Sync with playback position
    void setPlaybackPosition(double timeInSeconds);
    void setPlaybackRate(double rate);

    // Settings
    void setUpdateInterval(int milliseconds);
    void setFFTSize(int size);

    bool isMonitoring() const { return m_isMonitoring; }

signals:
    void levelUpdated(float peakLeft, float peakRight, float rmsLeft, float rmsRight);
    void spectrumUpdated(const QVector<float>& spectrum);
    void positionUpdated(double timeInSeconds);
    void error(const QString& message);

private slots:
    void processAudioFrame();

private:
    void updateLevelMeters(const AudioFrameData& frameData);
    void updateSpectrum(const AudioFrameData& frameData);
    void updateWaveformCursor();

    std::unique_ptr<AudioAnalyzer> m_analyzer;
    QString m_filename;
    int m_streamIndex;

    // Connected widgets
    AudioLevelWidget* m_levelMeterLeft;
    AudioLevelWidget* m_levelMeterRight;
    SpectrumWidget* m_spectrumWidget;
    WaveformWidget* m_waveformWidget;

    // Monitoring state
    bool m_isMonitoring;
    bool m_isPaused;
    double m_currentPosition;
    double m_playbackRate;

    // Update timer
    QTimer* m_updateTimer;
    int m_updateInterval;

    // FFT settings
    int m_fftSize;

    // Channel separation
    QVector<float> extractLeftChannel(const QVector<float>& interleavedSamples, int channels);
    QVector<float> extractRightChannel(const QVector<float>& interleavedSamples, int channels);
};

} // namespace VideoStudio

#endif // AUDIOMONITOR_H
