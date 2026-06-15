#include "spectrumwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <cmath>
#include <algorithm>

namespace VideoStudio {

SpectrumWidget::SpectrumWidget(QWidget* parent)
    : QWidget(parent)
    , m_analyzer(std::make_unique<AudioAnalyzer>())
    , m_streamIndex(-1)
    , m_fftSize(2048)
    , m_sampleRate(44100.0f)
    , m_minFreq(20.0f)
    , m_maxFreq(20000.0f)
    , m_showPeaks(true)
    , m_displayMode(Bars)
    , m_logarithmicScale(true)
    , m_numBands(64)
    , m_isAnalyzing(false)
    , m_backgroundColor(QColor(20, 20, 20))
    , m_spectrumColor(QColor(0, 200, 255))
    , m_gridColor(QColor(50, 50, 50))
    , m_peakColor(QColor(255, 100, 100))
    , m_axisColor(QColor(150, 150, 150))
{
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &SpectrumWidget::updateSpectrum);

    setMinimumHeight(100);

    // Initialize magnitude spectrum
    m_magnitudeSpectrum.resize(m_fftSize / 2);
    m_magnitudeSpectrum.fill(0.0f);
}

SpectrumWidget::~SpectrumWidget() {
    stopAnalysis();
}

void SpectrumWidget::setAudioFile(const QString& filename, int streamIndex) {
    stopAnalysis();

    m_filename = filename;
    m_streamIndex = streamIndex;

    if (filename.isEmpty()) {
        clear();
        return;
    }

    // Open audio file
    if (!m_analyzer->openFile(filename, streamIndex)) {
        qWarning() << "SpectrumWidget: Failed to open audio file:" << filename;
        clear();
        return;
    }

    // Get audio info
    AudioStreamInfo info = m_analyzer->getStreamInfo();
    m_sampleRate = static_cast<float>(info.sampleRate);

    // Adjust max frequency based on Nyquist limit
    m_maxFreq = qMin(20000.0f, m_sampleRate / 2.0f);

    update();
}

void SpectrumWidget::setFFTSize(int size) {
    // Ensure power of 2
    int validSize = 256;
    while (validSize < size && validSize < 8192) {
        validSize *= 2;
    }

    m_fftSize = validSize;
    m_magnitudeSpectrum.resize(m_fftSize / 2);
    m_magnitudeSpectrum.fill(0.0f);

    update();
}

void SpectrumWidget::setFrequencyRange(float minFreq, float maxFreq) {
    m_minFreq = qMax(1.0f, minFreq);
    m_maxFreq = qMin(m_sampleRate / 2.0f, maxFreq);
    update();
}

void SpectrumWidget::setDisplayMode(DisplayMode mode) {
    m_displayMode = mode;

    // Initialize waterfall data if needed
    if (mode == Waterfall && m_waterfallData.isEmpty()) {
        m_waterfallData.resize(100);  // Keep last 100 frames
        for (auto& row : m_waterfallData) {
            row.resize(m_numBands);
            row.fill(0.0f);
        }
    }

    update();
}

void SpectrumWidget::setLogarithmicScale(bool enable) {
    m_logarithmicScale = enable;
    update();
}

void SpectrumWidget::setShowPeaks(bool show) {
    m_showPeaks = show;
    update();
}

void SpectrumWidget::startAnalysis() {
    if (m_filename.isEmpty()) {
        return;
    }

    m_isAnalyzing = true;
    m_updateTimer->start(50);  // Update every 50ms
}

void SpectrumWidget::stopAnalysis() {
    m_isAnalyzing = false;
    m_updateTimer->stop();
}

void SpectrumWidget::clear() {
    m_filename.clear();
    m_magnitudeSpectrum.fill(0.0f);
    m_waterfallData.clear();
    m_peakIndices.clear();
    update();
}

void SpectrumWidget::updateSpectrum() {
    if (!m_isAnalyzing || !m_analyzer->isOpen()) {
        return;
    }

    analyzeAudio();
    update();
}

void SpectrumWidget::analyzeAudio() {
    // Decode a frame of audio
    AudioFrameData frameData;
    if (!m_analyzer->decodeNextFrame(frameData)) {
        // Reached end of file, loop back
        m_analyzer->seekToTime(0.0);
        return;
    }

    // Get enough samples for FFT
    if (frameData.samples.size() < m_fftSize) {
        return;
    }

    // Take first m_fftSize samples
    QVector<float> samples(m_fftSize);
    for (int i = 0; i < m_fftSize; ++i) {
        samples[i] = frameData.samples[i];
    }

    // Compute FFT
    computeFFT(samples);
}

void SpectrumWidget::computeFFT(const QVector<float>& samples) {
    // Prepare real and imaginary parts
    QVector<float> real = samples;
    QVector<float> imag(m_fftSize, 0.0f);

    // Apply Hanning window to reduce spectral leakage
    for (int i = 0; i < m_fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (m_fftSize - 1)));
        real[i] *= window;
    }

    // Perform FFT
    fft(real, imag);

    // Calculate magnitude spectrum
    calculateMagnitudeSpectrum(real, imag);

    // Detect peaks
    if (m_showPeaks) {
        m_peakIndices.clear();

        for (int i = 2; i < m_magnitudeSpectrum.size() - 2; ++i) {
            if (m_magnitudeSpectrum[i] > m_magnitudeSpectrum[i-1] &&
                m_magnitudeSpectrum[i] > m_magnitudeSpectrum[i+1] &&
                m_magnitudeSpectrum[i] > m_magnitudeSpectrum[i-2] &&
                m_magnitudeSpectrum[i] > m_magnitudeSpectrum[i+2] &&
                m_magnitudeSpectrum[i] > -40.0f) {  // Above -40 dB threshold
                m_peakIndices.append(i);
            }
        }

        // Emit strongest peak
        if (!m_peakIndices.isEmpty()) {
            int strongestPeak = m_peakIndices[0];
            float maxMag = m_magnitudeSpectrum[strongestPeak];

            for (int idx : m_peakIndices) {
                if (m_magnitudeSpectrum[idx] > maxMag) {
                    maxMag = m_magnitudeSpectrum[idx];
                    strongestPeak = idx;
                }
            }

            float freq = (strongestPeak * m_sampleRate) / m_fftSize;
            emit peakFrequencyDetected(freq, maxMag);
        }
    }

    // Update waterfall data
    if (m_displayMode == Waterfall) {
        // Shift waterfall data down
        m_waterfallData.pop_back();

        // Add new spectrum at top
        QVector<float> newRow(m_numBands);
        for (int i = 0; i < m_numBands; ++i) {
            int spectrumIndex = (i * m_magnitudeSpectrum.size()) / m_numBands;
            newRow[i] = m_magnitudeSpectrum[spectrumIndex];
        }
        m_waterfallData.prepend(newRow);
    }
}

// FFT implementation (Cooley-Tukey algorithm)
void SpectrumWidget::fft(QVector<float>& real, QVector<float>& imag) {
    int n = real.size();

    // Bit-reversal permutation
    int j = 0;
    for (int i = 0; i < n - 1; ++i) {
        if (i < j) {
            std::swap(real[i], real[j]);
            std::swap(imag[i], imag[j]);
        }

        int k = n / 2;
        while (k <= j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }

    // FFT computation
    for (int len = 2; len <= n; len *= 2) {
        float angle = -2.0f * M_PI / len;
        float wlenReal = std::cos(angle);
        float wlenImag = std::sin(angle);

        for (int i = 0; i < n; i += len) {
            float wReal = 1.0f;
            float wImag = 0.0f;

            for (int j = 0; j < len / 2; ++j) {
                float uReal = real[i + j];
                float uImag = imag[i + j];

                float tReal = wReal * real[i + j + len/2] - wImag * imag[i + j + len/2];
                float tImag = wReal * imag[i + j + len/2] + wImag * real[i + j + len/2];

                real[i + j] = uReal + tReal;
                imag[i + j] = uImag + tImag;

                real[i + j + len/2] = uReal - tReal;
                imag[i + j + len/2] = uImag - tImag;

                float wRealTemp = wReal * wlenReal - wImag * wlenImag;
                wImag = wReal * wlenImag + wImag * wlenReal;
                wReal = wRealTemp;
            }
        }
    }
}

void SpectrumWidget::calculateMagnitudeSpectrum(const QVector<float>& real,
                                                const QVector<float>& imag) {
    // Calculate magnitude in dB
    for (int i = 0; i < m_fftSize / 2; ++i) {
        float magnitude = std::sqrt(real[i] * real[i] + imag[i] * imag[i]);

        // Convert to dB scale (with floor to avoid log(0))
        float magnitudeDB = 20.0f * std::log10(qMax(magnitude, 1e-10f));

        // Normalize to 0 dB max
        m_magnitudeSpectrum[i] = magnitudeDB;
    }
}

void SpectrumWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Fill background
    painter.fillRect(rect(), m_backgroundColor);

    if (m_magnitudeSpectrum.isEmpty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No spectrum data"));
        return;
    }

    // Draw grid and axes
    drawFrequencyAxis(painter);
    drawMagnitudeAxis(painter);

    // Draw spectrum based on display mode
    switch (m_displayMode) {
    case Bars:
        drawBars(painter);
        break;
    case Line:
        drawLine(painter);
        break;
    case Filled:
        drawFilled(painter);
        break;
    case Waterfall:
        drawWaterfall(painter);
        break;
    }

    // Draw peaks
    if (m_showPeaks && m_displayMode != Waterfall) {
        drawPeaks(painter);
    }
}

void SpectrumWidget::drawBars(QPainter& painter) {
    int w = width() - 60;  // Leave space for axis
    int h = height() - 40;
    int xOffset = 50;
    int yOffset = 10;

    int barWidth = qMax(1, w / m_numBands);

    for (int i = 0; i < m_numBands; ++i) {
        // Calculate frequency range for this band
        float freqStart = m_logarithmicScale ?
            m_minFreq * std::pow(m_maxFreq / m_minFreq, static_cast<float>(i) / m_numBands) :
            m_minFreq + (m_maxFreq - m_minFreq) * i / m_numBands;

        float freqEnd = m_logarithmicScale ?
            m_minFreq * std::pow(m_maxFreq / m_minFreq, static_cast<float>(i + 1) / m_numBands) :
            m_minFreq + (m_maxFreq - m_minFreq) * (i + 1) / m_numBands;

        // Find magnitude in this frequency range
        int binStart = static_cast<int>((freqStart / m_sampleRate) * m_fftSize);
        int binEnd = static_cast<int>((freqEnd / m_sampleRate) * m_fftSize);

        binStart = qBound(0, binStart, m_magnitudeSpectrum.size() - 1);
        binEnd = qBound(0, binEnd, m_magnitudeSpectrum.size() - 1);

        // Average magnitude in this range
        float avgMagnitude = 0.0f;
        int count = 0;
        for (int j = binStart; j <= binEnd; ++j) {
            avgMagnitude += m_magnitudeSpectrum[j];
            count++;
        }
        if (count > 0) {
            avgMagnitude /= count;
        }

        // Convert magnitude to bar height (-80 dB to 0 dB range)
        float normalizedMag = (avgMagnitude + 80.0f) / 80.0f;
        normalizedMag = qBound(0.0f, normalizedMag, 1.0f);

        int barHeight = static_cast<int>(h * normalizedMag);
        int x = xOffset + (i * w) / m_numBands;
        int y = yOffset + h - barHeight;

        // Color gradient based on magnitude
        QColor barColor = m_spectrumColor;
        if (avgMagnitude > -10.0f) {
            barColor = QColor(255, 100, 100);  // Red for high levels
        } else if (avgMagnitude > -30.0f) {
            barColor = QColor(255, 200, 0);    // Yellow for medium levels
        }

        painter.fillRect(x, y, barWidth - 1, barHeight, barColor);
    }
}

void SpectrumWidget::drawLine(QPainter& painter) {
    int w = width() - 60;
    int h = height() - 40;
    int xOffset = 50;
    int yOffset = 10;

    painter.setPen(QPen(m_spectrumColor, 2));

    QVector<QPointF> points;

    for (int i = 0; i < m_numBands; ++i) {
        float freq = m_logarithmicScale ?
            m_minFreq * std::pow(m_maxFreq / m_minFreq, static_cast<float>(i) / m_numBands) :
            m_minFreq + (m_maxFreq - m_minFreq) * i / m_numBands;

        int bin = static_cast<int>((freq / m_sampleRate) * m_fftSize);
        bin = qBound(0, bin, m_magnitudeSpectrum.size() - 1);

        float magnitude = m_magnitudeSpectrum[bin];
        float normalizedMag = (magnitude + 80.0f) / 80.0f;
        normalizedMag = qBound(0.0f, normalizedMag, 1.0f);

        int x = xOffset + (i * w) / m_numBands;
        int y = yOffset + h - static_cast<int>(h * normalizedMag);

        points.append(QPointF(x, y));
    }

    if (!points.isEmpty()) {
        painter.drawPolyline(points.data(), points.size());
    }
}

void SpectrumWidget::drawFilled(QPainter& painter) {
    int w = width() - 60;
    int h = height() - 40;
    int xOffset = 50;
    int yOffset = 10;

    QPolygonF polygon;
    polygon << QPointF(xOffset, yOffset + h);  // Start at bottom left

    for (int i = 0; i < m_numBands; ++i) {
        float freq = m_logarithmicScale ?
            m_minFreq * std::pow(m_maxFreq / m_minFreq, static_cast<float>(i) / m_numBands) :
            m_minFreq + (m_maxFreq - m_minFreq) * i / m_numBands;

        int bin = static_cast<int>((freq / m_sampleRate) * m_fftSize);
        bin = qBound(0, bin, m_magnitudeSpectrum.size() - 1);

        float magnitude = m_magnitudeSpectrum[bin];
        float normalizedMag = (magnitude + 80.0f) / 80.0f;
        normalizedMag = qBound(0.0f, normalizedMag, 1.0f);

        int x = xOffset + (i * w) / m_numBands;
        int y = yOffset + h - static_cast<int>(h * normalizedMag);

        polygon << QPointF(x, y);
    }

    polygon << QPointF(xOffset + w, yOffset + h);  // End at bottom right

    QLinearGradient gradient(0, yOffset, 0, yOffset + h);
    gradient.setColorAt(0, m_spectrumColor);
    gradient.setColorAt(1, m_backgroundColor);

    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(polygon);

    // Draw outline
    painter.setPen(QPen(m_spectrumColor, 2));
    painter.setBrush(Qt::NoBrush);
    for (int i = 1; i < polygon.size() - 1; ++i) {
        painter.drawLine(polygon[i-1], polygon[i]);
    }
}

void SpectrumWidget::drawWaterfall(QPainter& painter) {
    if (m_waterfallData.isEmpty()) {
        return;
    }

    int w = width() - 60;
    int h = height() - 40;
    int xOffset = 50;
    int yOffset = 10;

    int rowHeight = qMax(1, h / m_waterfallData.size());

    for (int row = 0; row < m_waterfallData.size(); ++row) {
        const QVector<float>& rowData = m_waterfallData[row];

        for (int col = 0; col < rowData.size(); ++col) {
            float magnitude = rowData[col];
            float normalizedMag = (magnitude + 80.0f) / 80.0f;
            normalizedMag = qBound(0.0f, normalizedMag, 1.0f);

            // Color map: black -> blue -> cyan -> yellow -> red
            int r, g, b;
            if (normalizedMag < 0.25f) {
                r = 0;
                g = 0;
                b = static_cast<int>(255 * normalizedMag * 4);
            } else if (normalizedMag < 0.5f) {
                r = 0;
                g = static_cast<int>(255 * (normalizedMag - 0.25f) * 4);
                b = 255;
            } else if (normalizedMag < 0.75f) {
                r = static_cast<int>(255 * (normalizedMag - 0.5f) * 4);
                g = 255;
                b = 255 - r;
            } else {
                r = 255;
                g = 255 - static_cast<int>(255 * (normalizedMag - 0.75f) * 4);
                b = 0;
            }

            int x = xOffset + (col * w) / rowData.size();
            int y = yOffset + row * rowHeight;
            int cellWidth = qMax(1, w / rowData.size());

            painter.fillRect(x, y, cellWidth, rowHeight, QColor(r, g, b));
        }
    }
}

void SpectrumWidget::drawFrequencyAxis(QPainter& painter) {
    int w = width() - 60;
    int h = height() - 40;
    int xOffset = 50;
    int yOffset = 10;

    painter.setPen(m_axisColor);

    // Draw axis line
    painter.drawLine(xOffset, yOffset + h, xOffset + w, yOffset + h);

    // Draw frequency labels
    QVector<float> frequencies;
    if (m_logarithmicScale) {
        frequencies = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    } else {
        for (int i = 0; i <= 10; ++i) {
            frequencies.append(m_minFreq + (m_maxFreq - m_minFreq) * i / 10);
        }
    }

    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    for (float freq : frequencies) {
        if (freq < m_minFreq || freq > m_maxFreq) {
            continue;
        }

        float x = frequencyToX(freq);

        // Draw tick
        painter.drawLine(static_cast<int>(x), yOffset + h,
                        static_cast<int>(x), yOffset + h + 5);

        // Draw label
        QString label;
        if (freq >= 1000) {
            label = QString::number(freq / 1000, 'f', freq >= 10000 ? 0 : 1) + "k";
        } else {
            label = QString::number(static_cast<int>(freq));
        }

        QRect textRect(static_cast<int>(x) - 30, yOffset + h + 8, 60, 20);
        painter.drawText(textRect, Qt::AlignCenter, label);
    }

    // Draw "Hz" label
    painter.drawText(xOffset + w - 20, yOffset + h + 25, "Hz");
}

void SpectrumWidget::drawMagnitudeAxis(QPainter& painter) {
    int h = height() - 40;
    int xOffset = 50;
    int yOffset = 10;

    painter.setPen(m_axisColor);

    // Draw axis line
    painter.drawLine(xOffset, yOffset, xOffset, yOffset + h);

    // Draw grid lines and labels
    QVector<int> dbLevels = {0, -10, -20, -30, -40, -50, -60, -70, -80};

    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    for (int db : dbLevels) {
        float normalizedMag = (db + 80.0f) / 80.0f;
        int y = yOffset + h - static_cast<int>(h * normalizedMag);

        // Draw grid line
        painter.setPen(QPen(m_gridColor, 1, Qt::DotLine));
        painter.drawLine(xOffset, y, width() - 10, y);

        // Draw tick and label
        painter.setPen(m_axisColor);
        painter.drawLine(xOffset - 5, y, xOffset, y);

        QString label = QString::number(db);
        painter.drawText(5, y - 10, 40, 20, Qt::AlignRight | Qt::AlignVCenter, label);
    }

    // Draw "dB" label
    painter.drawText(5, yOffset - 5, 40, 20, Qt::AlignRight, "dB");
}

void SpectrumWidget::drawPeaks(QPainter& painter) {
    if (m_peakIndices.isEmpty()) {
        return;
    }

    int h = height() - 40;
    int yOffset = 10;

    painter.setPen(QPen(m_peakColor, 2));

    for (int idx : m_peakIndices) {
        float freq = (idx * m_sampleRate) / m_fftSize;

        if (freq < m_minFreq || freq > m_maxFreq) {
            continue;
        }

        float x = frequencyToX(freq);
        float magnitude = m_magnitudeSpectrum[idx];
        float normalizedMag = (magnitude + 80.0f) / 80.0f;
        normalizedMag = qBound(0.0f, normalizedMag, 1.0f);

        int y = yOffset + h - static_cast<int>(h * normalizedMag);

        // Draw peak marker
        painter.drawEllipse(QPointF(x, y), 4, 4);

        // Draw frequency label for top 3 peaks
        if (m_peakIndices.indexOf(idx) < 3) {
            QString label = QString::number(static_cast<int>(freq)) + " Hz";
            painter.drawText(static_cast<int>(x) - 30, y - 15, 60, 15,
                           Qt::AlignCenter, label);
        }
    }
}

void SpectrumWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
}

float SpectrumWidget::frequencyToX(float freq) const {
    int w = width() - 60;
    int xOffset = 50;

    if (m_logarithmicScale) {
        if (freq <= m_minFreq) return xOffset;
        if (freq >= m_maxFreq) return xOffset + w;

        float logMin = std::log10(m_minFreq);
        float logMax = std::log10(m_maxFreq);
        float logFreq = std::log10(freq);

        return xOffset + w * (logFreq - logMin) / (logMax - logMin);
    } else {
        return xOffset + w * (freq - m_minFreq) / (m_maxFreq - m_minFreq);
    }
}

float SpectrumWidget::xToFrequency(int x) const {
    int w = width() - 60;
    int xOffset = 50;

    float ratio = static_cast<float>(x - xOffset) / w;

    if (m_logarithmicScale) {
        float logMin = std::log10(m_minFreq);
        float logMax = std::log10(m_maxFreq);
        float logFreq = logMin + ratio * (logMax - logMin);
        return std::pow(10.0f, logFreq);
    } else {
        return m_minFreq + ratio * (m_maxFreq - m_minFreq);
    }
}

int SpectrumWidget::magnitudeToY(float magnitude) const {
    int h = height() - 40;
    int yOffset = 10;

    float normalizedMag = (magnitude + 80.0f) / 80.0f;
    normalizedMag = qBound(0.0f, normalizedMag, 1.0f);

    return yOffset + h - static_cast<int>(h * normalizedMag);
}

} // namespace VideoStudio
