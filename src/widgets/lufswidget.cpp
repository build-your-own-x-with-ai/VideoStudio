#include "lufswidget.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <cmath>
#include <algorithm>

namespace VideoStudio {

LUFSWidget::LUFSWidget(QWidget* parent)
    : QWidget(parent)
    , m_integratedLUFS(-70.0)
    , m_shortTermLUFS(-70.0)
    , m_momentaryLUFS(-70.0)
    , m_loudnessRange(0.0)
    , m_truePeak(-70.0)
    , m_sampleRate(48000)
    , m_shortTermSize(3 * 48000)
    , m_momentarySize(400 * 48000 / 1000)
{
    initFilters();

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Create grid for measurements
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(10);

    // Integrated LUFS
    QLabel* intLabel = new QLabel(tr("Integrated:"), this);
    m_integratedLabel = new QLabel("-70.0 LUFS", this);
    m_integratedLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; }");
    gridLayout->addWidget(intLabel, 0, 0);
    gridLayout->addWidget(m_integratedLabel, 0, 1);

    // Short-term LUFS
    QLabel* shortLabel = new QLabel(tr("Short-term (3s):"), this);
    m_shortTermLabel = new QLabel("-70.0 LUFS", this);
    gridLayout->addWidget(shortLabel, 1, 0);
    gridLayout->addWidget(m_shortTermLabel, 1, 1);

    // Momentary LUFS
    QLabel* momLabel = new QLabel(tr("Momentary (400ms):"), this);
    m_momentaryLabel = new QLabel("-70.0 LUFS", this);
    gridLayout->addWidget(momLabel, 2, 0);
    gridLayout->addWidget(m_momentaryLabel, 2, 1);

    // Loudness Range
    QLabel* lraLabel = new QLabel(tr("Loudness Range:"), this);
    m_rangeLabel = new QLabel("0.0 LU", this);
    gridLayout->addWidget(lraLabel, 3, 0);
    gridLayout->addWidget(m_rangeLabel, 3, 1);

    // True Peak
    QLabel* peakLabel = new QLabel(tr("True Peak:"), this);
    m_truePeakLabel = new QLabel("-70.0 dBTP", this);
    gridLayout->addWidget(peakLabel, 4, 0);
    gridLayout->addWidget(m_truePeakLabel, 4, 1);

    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch();

    setMinimumHeight(180);
}

void LUFSWidget::initFilters() {
    // ITU-R BS.1770-4 K-weighting filter coefficients
    // Assumes 48 kHz sample rate (adjust if needed)

    // High-shelf filter (stage 1): ~4 dB boost above 1.5 kHz
    double f0 = 1681.974450955533;
    double G = 3.999843853973347;
    double Q = 0.7071752369554196;

    double K = std::tan(M_PI * f0 / m_sampleRate);
    double Vh = std::pow(10.0, G / 20.0);
    double Vb = std::pow(Vh, 0.4996667741545416);

    double a0 = 1.0 + K / Q + K * K;
    m_hsfB0 = (Vh + Vb * K / Q + K * K) / a0;
    m_hsfB1 = 2.0 * (K * K - Vh) / a0;
    m_hsfB2 = (Vh - Vb * K / Q + K * K) / a0;
    m_hsfA1 = 2.0 * (K * K - 1.0) / a0;
    m_hsfA2 = (1.0 - K / Q + K * K) / a0;

    // High-pass filter (stage 2): 38 Hz cutoff
    double fc = 38.13547087602444;
    double Qhp = 0.5003270373238773;

    double Khp = std::tan(M_PI * fc / m_sampleRate);
    double a0hp = 1.0 + Khp / Qhp + Khp * Khp;
    m_hpfB0 = 1.0 / a0hp;
    m_hpfB1 = -2.0 / a0hp;
    m_hpfB2 = 1.0 / a0hp;
    m_hpfA1 = 2.0 * (Khp * Khp - 1.0) / a0hp;
    m_hpfA2 = (1.0 - Khp / Qhp + Khp * Khp) / a0hp;

    // Initialize filter states for 2 channels
    m_highShelfState.resize(2, {0.0, 0.0, 0.0, 0.0});
    m_highPassState.resize(2, {0.0, 0.0, 0.0, 0.0});
}

double LUFSWidget::applyKWeighting(double sample, int channel) {
    // Apply high-shelf filter
    FilterState& hsf = m_highShelfState[channel];
    double yHsf = m_hsfB0 * sample + m_hsfB1 * hsf.x1 + m_hsfB2 * hsf.x2
                  - m_hsfA1 * hsf.y1 - m_hsfA2 * hsf.y2;
    hsf.x2 = hsf.x1;
    hsf.x1 = sample;
    hsf.y2 = hsf.y1;
    hsf.y1 = yHsf;

    // Apply high-pass filter
    FilterState& hpf = m_highPassState[channel];
    double yHpf = m_hpfB0 * yHsf + m_hpfB1 * hpf.x1 + m_hpfB2 * hpf.x2
                  - m_hpfA1 * hpf.y1 - m_hpfA2 * hpf.y2;
    hpf.x2 = hpf.x1;
    hpf.x1 = yHsf;
    hpf.y2 = hpf.y1;
    hpf.y1 = yHpf;

    return yHpf;
}

double LUFSWidget::calculateMeanSquare(const std::vector<double>& samples) {
    if (samples.empty()) return 0.0;

    double sum = 0.0;
    for (double sample : samples) {
        sum += sample * sample;
    }
    return sum / samples.size();
}

double LUFSWidget::powerToLUFS(double power) {
    if (power <= 0.0) return -70.0;
    return -0.691 + 10.0 * std::log10(power);
}

void LUFSWidget::updateLoudness(const std::vector<float>& leftChannel,
                                const std::vector<float>& rightChannel,
                                int sampleRate) {
    if (leftChannel.empty() || rightChannel.empty()) return;

    m_sampleRate = sampleRate;
    m_shortTermSize = 3 * sampleRate;
    m_momentarySize = 400 * sampleRate / 1000;

    // Process samples through K-weighting filter
    std::vector<double> leftWeighted;
    std::vector<double> rightWeighted;

    size_t numSamples = std::min(leftChannel.size(), rightChannel.size());
    leftWeighted.reserve(numSamples);
    rightWeighted.reserve(numSamples);

    for (size_t i = 0; i < numSamples; ++i) {
        leftWeighted.push_back(applyKWeighting(leftChannel[i], 0));
        rightWeighted.push_back(applyKWeighting(rightChannel[i], 1));
    }

    // Calculate channel powers and combine
    for (size_t i = 0; i < numSamples; ++i) {
        double channelPower = leftWeighted[i] * leftWeighted[i] +
                             rightWeighted[i] * rightWeighted[i];

        // Add to buffers
        m_momentaryBuffer.push_back(channelPower);
        m_shortTermBuffer.push_back(channelPower);
        m_integratedBuffer.push_back(channelPower);

        // Maintain buffer sizes
        if (m_momentaryBuffer.size() > m_momentarySize) {
            m_momentaryBuffer.erase(m_momentaryBuffer.begin());
        }
        if (m_shortTermBuffer.size() > m_shortTermSize) {
            m_shortTermBuffer.erase(m_shortTermBuffer.begin());
        }
    }

    // Calculate momentary loudness (400ms)
    if (m_momentaryBuffer.size() >= m_momentarySize) {
        double momentaryPower = calculateMeanSquare(m_momentaryBuffer);
        m_momentaryLUFS = powerToLUFS(momentaryPower);
    }

    // Calculate short-term loudness (3s)
    if (m_shortTermBuffer.size() >= m_shortTermSize) {
        double shortTermPower = calculateMeanSquare(m_shortTermBuffer);
        m_shortTermLUFS = powerToLUFS(shortTermPower);
    }

    // Calculate integrated loudness (gated)
    if (!m_integratedBuffer.empty()) {
        double integratedPower = calculateMeanSquare(m_integratedBuffer);
        m_integratedLUFS = powerToLUFS(integratedPower);
    }

    // Calculate true peak
    m_truePeak = -70.0;
    for (float sample : leftChannel) {
        double db = 20.0 * std::log10(std::abs(sample) + 1e-10);
        m_truePeak = std::max(m_truePeak, db);
    }
    for (float sample : rightChannel) {
        double db = 20.0 * std::log10(std::abs(sample) + 1e-10);
        m_truePeak = std::max(m_truePeak, db);
    }

    // Update labels
    m_integratedLabel->setText(QString("%1 LUFS").arg(m_integratedLUFS, 0, 'f', 1));
    m_shortTermLabel->setText(QString("%1 LUFS").arg(m_shortTermLUFS, 0, 'f', 1));
    m_momentaryLabel->setText(QString("%1 LUFS").arg(m_momentaryLUFS, 0, 'f', 1));
    m_truePeakLabel->setText(QString("%1 dBTP").arg(m_truePeak, 0, 'f', 1));

    update();
}

void LUFSWidget::reset() {
    m_integratedLUFS = -70.0;
    m_shortTermLUFS = -70.0;
    m_momentaryLUFS = -70.0;
    m_loudnessRange = 0.0;
    m_truePeak = -70.0;

    m_momentaryBuffer.clear();
    m_shortTermBuffer.clear();
    m_integratedBuffer.clear();

    // Reset filter states
    for (auto& state : m_highShelfState) {
        state = {0.0, 0.0, 0.0, 0.0};
    }
    for (auto& state : m_highPassState) {
        state = {0.0, 0.0, 0.0, 0.0};
    }

    m_integratedLabel->setText("-70.0 LUFS");
    m_shortTermLabel->setText("-70.0 LUFS");
    m_momentaryLabel->setText("-70.0 LUFS");
    m_rangeLabel->setText("0.0 LU");
    m_truePeakLabel->setText("-70.0 dBTP");

    update();
}

void LUFSWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QWidget::paintEvent(event);
}

} // namespace VideoStudio
