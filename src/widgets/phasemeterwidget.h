#ifndef VIDEOSTUDIO_PHASEMETERWIDGET_H
#define VIDEOSTUDIO_PHASEMETERWIDGET_H

#include <QWidget>
#include <QLabel>
#include <vector>
#include <deque>

namespace VideoStudio {

// Stereo phase correlation meter (Goniometer / Lissajous display)
class PhaseMeterWidget : public QWidget {
    Q_OBJECT

public:
    explicit PhaseMeterWidget(QWidget* parent = nullptr);

    // Update with new stereo samples
    void updatePhase(const std::vector<float>& leftChannel,
                     const std::vector<float>& rightChannel);

    // Reset display
    void reset();

    // Get current correlation
    double getCorrelation() const { return m_correlation; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    // Calculate stereo correlation coefficient (-1 to +1)
    double calculateCorrelation(const std::vector<float>& left,
                               const std::vector<float>& right);

    // Correlation display label
    QLabel* m_correlationLabel;

    // Current correlation value (-1 to +1)
    double m_correlation;

    // Point buffer for Lissajous display (M/S visualization)
    struct PhasePoint {
        float mid;   // (L + R) / 2
        float side;  // (L - R) / 2
    };

    std::deque<PhasePoint> m_phasePoints;
    static const size_t MAX_POINTS = 2000;  // History buffer size

    // Display colors
    QColor m_backgroundColor;
    QColor m_gridColor;
    QColor m_centerColor;
    QColor m_pointColor;
    QColor m_monoLineColor;
    QColor m_stereoLineColor;

    // Decay/fade for trail effect
    float m_pointAlpha;
};

} // namespace VideoStudio

#endif // VIDEOSTUDIO_PHASEMETERWIDGET_H
