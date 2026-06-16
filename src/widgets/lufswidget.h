#ifndef VIDEOSTUDIO_LUFSWIDGET_H
#define VIDEOSTUDIO_LUFSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <vector>

namespace VideoStudio {

// ITU-R BS.1770-4 compliant loudness measurement
class LUFSWidget : public QWidget {
    Q_OBJECT

public:
    explicit LUFSWidget(QWidget* parent = nullptr);

    // Update with new audio samples
    void updateLoudness(const std::vector<float>& leftChannel,
                       const std::vector<float>& rightChannel,
                       int sampleRate);

    // Reset measurements
    void reset();

    // Get current values
    double getIntegratedLUFS() const { return m_integratedLUFS; }
    double getShortTermLUFS() const { return m_shortTermLUFS; }
    double getMomentaryLUFS() const { return m_momentaryLUFS; }
    double getLoudnessRange() const { return m_loudnessRange; }
    double getTruePeak() const { return m_truePeak; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    // K-weighting filter coefficients (ITU-R BS.1770)
    void initFilters();

    // Apply K-weighting filter (high-shelf + high-pass)
    double applyKWeighting(double sample, int channel);

    // Calculate mean square for loudness
    double calculateMeanSquare(const std::vector<double>& samples);

    // Convert power to LUFS
    double powerToLUFS(double power);

    // Labels for display
    QLabel* m_integratedLabel;
    QLabel* m_shortTermLabel;
    QLabel* m_momentaryLabel;
    QLabel* m_rangeLabel;
    QLabel* m_truePeakLabel;

    // Current measurements
    double m_integratedLUFS;    // Integrated loudness over entire program
    double m_shortTermLUFS;     // 3-second sliding window
    double m_momentaryLUFS;     // 400ms sliding window
    double m_loudnessRange;     // LRA (statistical range)
    double m_truePeak;          // True peak level in dBTP

    // Filter state for K-weighting (BS.1770-4)
    struct FilterState {
        double x1, x2;  // Input history
        double y1, y2;  // Output history
    };

    std::vector<FilterState> m_highShelfState;  // Per-channel state
    std::vector<FilterState> m_highPassState;   // Per-channel state

    // Filter coefficients
    double m_hsfB0, m_hsfB1, m_hsfB2;  // High-shelf filter
    double m_hsfA1, m_hsfA2;
    double m_hpfB0, m_hpfB1, m_hpfB2;  // High-pass filter
    double m_hpfA1, m_hpfA2;

    // Sliding window buffers
    std::vector<double> m_shortTermBuffer;   // 3 seconds
    std::vector<double> m_momentaryBuffer;   // 400ms
    std::vector<double> m_integratedBuffer;  // Full duration

    int m_sampleRate;
    size_t m_shortTermSize;   // 3s * sample_rate
    size_t m_momentarySize;   // 0.4s * sample_rate
};

} // namespace VideoStudio

#endif // VIDEOSTUDIO_LUFSWIDGET_H
