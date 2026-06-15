#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H

#include <QWidget>
#include <QVector>
#include <QTimer>
#include <memory>
#include "core/audioanalyzer.h"

namespace VideoStudio {

class SpectrumWidget : public QWidget {
    Q_OBJECT

public:
    enum DisplayMode {
        Bars,           // Bar chart display
        Line,           // Line graph display
        Filled,         // Filled area display
        Waterfall       // Waterfall/spectrogram display
    };

    explicit SpectrumWidget(QWidget* parent = nullptr);
    ~SpectrumWidget() override;

    // Set audio file and stream
    void setAudioFile(const QString& filename, int streamIndex = -1);

    // Set FFT size (must be power of 2: 256, 512, 1024, 2048, 4096)
    void setFFTSize(int size);

    // Set frequency range to display
    void setFrequencyRange(float minFreq, float maxFreq);

    // Set display mode
    void setDisplayMode(DisplayMode mode);

    // Enable/disable logarithmic frequency scale
    void setLogarithmicScale(bool enable);

    // Enable/disable peak frequency detection
    void setShowPeaks(bool show);

    // Start/stop real-time updates
    void startAnalysis();
    void stopAnalysis();

    // Clear display
    void clear();

    // Update spectrum from external source (e.g., AudioMonitor)
    void updateSpectrumData(const QVector<float>& magnitudeSpectrum);

    QSize sizeHint() const override { return QSize(600, 300); }
    QSize minimumSizeHint() const override { return QSize(300, 150); }

signals:
    void peakFrequencyDetected(float frequency, float magnitude);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void updateSpectrum();

private:
    void analyzeAudio();
    void computeFFT(const QVector<float>& samples);
    void drawBars(QPainter& painter);
    void drawLine(QPainter& painter);
    void drawFilled(QPainter& painter);
    void drawWaterfall(QPainter& painter);
    void drawFrequencyAxis(QPainter& painter);
    void drawMagnitudeAxis(QPainter& painter);
    void drawPeaks(QPainter& painter);

    float frequencyToX(float freq) const;
    float xToFrequency(int x) const;
    int magnitudeToY(float magnitude) const;

    // FFT implementation
    void fft(QVector<float>& real, QVector<float>& imag);
    void calculateMagnitudeSpectrum(const QVector<float>& real,
                                   const QVector<float>& imag);

    std::unique_ptr<AudioAnalyzer> m_analyzer;
    QString m_filename;
    int m_streamIndex;

    // FFT parameters
    int m_fftSize;
    float m_sampleRate;

    // Frequency range
    float m_minFreq;
    float m_maxFreq;

    // Spectrum data
    QVector<float> m_magnitudeSpectrum;  // Current spectrum
    QVector<QVector<float>> m_waterfallData;  // History for waterfall

    // Peak detection
    QVector<int> m_peakIndices;
    bool m_showPeaks;

    // Display settings
    DisplayMode m_displayMode;
    bool m_logarithmicScale;
    int m_numBands;  // Number of frequency bands to display

    // Update timer
    QTimer* m_updateTimer;
    bool m_isAnalyzing;

    // Colors
    QColor m_backgroundColor;
    QColor m_spectrumColor;
    QColor m_gridColor;
    QColor m_peakColor;
    QColor m_axisColor;
};

} // namespace VideoStudio

#endif // SPECTRUMWIDGET_H
