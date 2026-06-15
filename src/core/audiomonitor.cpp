#include "audiomonitor.h"
#include "widgets/audiolevelwidget.h"
#include "widgets/spectrumwidget.h"
#include "widgets/waveformwidget.h"
#include <QDebug>
#include <cmath>

namespace VideoStudio {

AudioMonitor::AudioMonitor(QObject* parent)
    : QObject(parent)
    , m_analyzer(std::make_unique<AudioAnalyzer>())
    , m_streamIndex(-1)
    , m_levelMeterLeft(nullptr)
    , m_levelMeterRight(nullptr)
    , m_spectrumWidget(nullptr)
    , m_waveformWidget(nullptr)
    , m_isMonitoring(false)
    , m_isPaused(false)
    , m_currentPosition(0.0)
    , m_playbackRate(1.0)
    , m_updateInterval(50)
    , m_fftSize(2048)
{
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &AudioMonitor::processAudioFrame);
}

AudioMonitor::~AudioMonitor() {
    stop();
}

bool AudioMonitor::setAudioFile(const QString& filename, int streamIndex) {
    stop();

    m_filename = filename;
    m_streamIndex = streamIndex;

    if (filename.isEmpty()) {
        return false;
    }

    if (!m_analyzer->openFile(filename, streamIndex)) {
        emit error(tr("Failed to open audio file: %1").arg(filename));
        return false;
    }

    m_currentPosition = 0.0;
    return true;
}

void AudioMonitor::setLevelMeterLeft(AudioLevelWidget* widget) {
    m_levelMeterLeft = widget;
}

void AudioMonitor::setLevelMeterRight(AudioLevelWidget* widget) {
    m_levelMeterRight = widget;
}

void AudioMonitor::setSpectrumWidget(SpectrumWidget* widget) {
    m_spectrumWidget = widget;
}

void AudioMonitor::setWaveformWidget(WaveformWidget* widget) {
    m_waveformWidget = widget;
}

void AudioMonitor::start() {
    if (m_filename.isEmpty() || !m_analyzer->isOpen()) {
        qWarning() << "AudioMonitor: Cannot start - no file loaded";
        return;
    }

    m_isMonitoring = true;
    m_isPaused = false;
    m_updateTimer->start(m_updateInterval);

    qDebug() << "AudioMonitor: Started monitoring";
}

void AudioMonitor::stop() {
    m_isMonitoring = false;
    m_isPaused = false;
    m_updateTimer->stop();
    m_currentPosition = 0.0;

    // Reset all widgets
    if (m_levelMeterLeft) {
        m_levelMeterLeft->setPeakLevel(0.0f);
        m_levelMeterLeft->setRMSLevel(0.0f);
    }
    if (m_levelMeterRight) {
        m_levelMeterRight->setPeakLevel(0.0f);
        m_levelMeterRight->setRMSLevel(0.0f);
    }

    qDebug() << "AudioMonitor: Stopped monitoring";
}

void AudioMonitor::pause() {
    m_isPaused = true;
    m_updateTimer->stop();
}

void AudioMonitor::resume() {
    if (m_isMonitoring) {
        m_isPaused = false;
        m_updateTimer->start(m_updateInterval);
    }
}

void AudioMonitor::setPlaybackPosition(double timeInSeconds) {
    m_currentPosition = timeInSeconds;

    // Seek analyzer to this position
    if (m_analyzer->isOpen()) {
        m_analyzer->seekToTime(timeInSeconds);
    }

    // Update waveform cursor
    updateWaveformCursor();

    emit positionUpdated(timeInSeconds);
}

void AudioMonitor::setPlaybackRate(double rate) {
    m_playbackRate = rate;
}

void AudioMonitor::setUpdateInterval(int milliseconds) {
    m_updateInterval = qMax(10, milliseconds);
    if (m_updateTimer->isActive()) {
        m_updateTimer->setInterval(m_updateInterval);
    }
}

void AudioMonitor::setFFTSize(int size) {
    m_fftSize = size;
}

void AudioMonitor::processAudioFrame() {
    if (m_isPaused || !m_analyzer->isOpen()) {
        return;
    }

    // Decode next audio frame
    AudioFrameData frameData;
    if (!m_analyzer->decodeNextFrame(frameData)) {
        // End of audio stream - loop back or stop
        m_analyzer->seekToTime(0.0);
        m_currentPosition = 0.0;
        return;
    }

    // Update current position
    m_currentPosition = frameData.timestamp;

    // Update visualizations
    updateLevelMeters(frameData);
    updateSpectrum(frameData);
    updateWaveformCursor();

    emit positionUpdated(m_currentPosition);
}

void AudioMonitor::updateLevelMeters(const AudioFrameData& frameData) {
    if (frameData.samples.isEmpty()) {
        return;
    }

    AudioStreamInfo info = m_analyzer->getStreamInfo();
    int channels = info.channels;

    if (channels == 1) {
        // Mono audio
        AudioLevelInfo level = m_analyzer->calculateLevel(frameData.samples);

        if (m_levelMeterLeft) {
            m_levelMeterLeft->setPeakLevel(level.peakLevel);
            m_levelMeterLeft->setRMSLevel(level.rmsLevel);
            m_levelMeterLeft->setdBFSLevel(level.dbFS);
        }

        emit levelUpdated(level.peakLevel, 0.0f, level.rmsLevel, 0.0f);
    } else if (channels == 2) {
        // Stereo audio - separate left and right
        QVector<float> leftSamples = extractLeftChannel(frameData.samples, channels);
        QVector<float> rightSamples = extractRightChannel(frameData.samples, channels);

        AudioLevelInfo leftLevel = m_analyzer->calculateLevel(leftSamples);
        AudioLevelInfo rightLevel = m_analyzer->calculateLevel(rightSamples);

        if (m_levelMeterLeft) {
            m_levelMeterLeft->setPeakLevel(leftLevel.peakLevel);
            m_levelMeterLeft->setRMSLevel(leftLevel.rmsLevel);
            m_levelMeterLeft->setdBFSLevel(leftLevel.dbFS);
        }

        if (m_levelMeterRight) {
            m_levelMeterRight->setPeakLevel(rightLevel.peakLevel);
            m_levelMeterRight->setRMSLevel(rightLevel.rmsLevel);
            m_levelMeterRight->setdBFSLevel(rightLevel.dbFS);
        }

        emit levelUpdated(leftLevel.peakLevel, rightLevel.peakLevel,
                         leftLevel.rmsLevel, rightLevel.rmsLevel);
    } else {
        // Multi-channel - use first channel for left, second for right
        QVector<float> leftSamples = extractLeftChannel(frameData.samples, channels);
        QVector<float> rightSamples = extractRightChannel(frameData.samples, channels);

        AudioLevelInfo leftLevel = m_analyzer->calculateLevel(leftSamples);
        AudioLevelInfo rightLevel = m_analyzer->calculateLevel(rightSamples);

        if (m_levelMeterLeft) {
            m_levelMeterLeft->setPeakLevel(leftLevel.peakLevel);
            m_levelMeterLeft->setRMSLevel(leftLevel.rmsLevel);
            m_levelMeterLeft->setdBFSLevel(leftLevel.dbFS);
        }

        if (m_levelMeterRight) {
            m_levelMeterRight->setPeakLevel(rightLevel.peakLevel);
            m_levelMeterRight->setRMSLevel(rightLevel.rmsLevel);
            m_levelMeterRight->setdBFSLevel(rightLevel.dbFS);
        }

        emit levelUpdated(leftLevel.peakLevel, rightLevel.peakLevel,
                         leftLevel.rmsLevel, rightLevel.rmsLevel);
    }
}

void AudioMonitor::updateSpectrum(const AudioFrameData& frameData) {
    if (!m_spectrumWidget || frameData.samples.isEmpty()) {
        return;
    }

    // Use first channel or mono for spectrum
    AudioStreamInfo info = m_analyzer->getStreamInfo();
    int channels = info.channels;

    QVector<float> samples;
    if (channels == 1) {
        samples = frameData.samples;
    } else {
        samples = extractLeftChannel(frameData.samples, channels);
    }

    // Calculate spectrum
    QVector<float> spectrum = m_analyzer->calculateSpectrum(samples, m_fftSize);

    emit spectrumUpdated(spectrum);
}

void AudioMonitor::updateWaveformCursor() {
    if (m_waveformWidget) {
        m_waveformWidget->setPlaybackCursor(m_currentPosition);
    }
}

QVector<float> AudioMonitor::extractLeftChannel(const QVector<float>& interleavedSamples, int channels) {
    if (channels <= 0) {
        return QVector<float>();
    }

    int numFrames = interleavedSamples.size() / channels;
    QVector<float> leftChannel(numFrames);

    for (int i = 0; i < numFrames; ++i) {
        leftChannel[i] = interleavedSamples[i * channels];
    }

    return leftChannel;
}

QVector<float> AudioMonitor::extractRightChannel(const QVector<float>& interleavedSamples, int channels) {
    if (channels < 2) {
        return QVector<float>();
    }

    int numFrames = interleavedSamples.size() / channels;
    QVector<float> rightChannel(numFrames);

    for (int i = 0; i < numFrames; ++i) {
        rightChannel[i] = interleavedSamples[i * channels + 1];
    }

    return rightChannel;
}

} // namespace VideoStudio
