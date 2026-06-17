#include "phasemeterwidget.h"
#include <QPainter>
#include <QVBoxLayout>
#include <cmath>
#include <algorithm>

namespace VideoStudio {

PhaseMeterWidget::PhaseMeterWidget(QWidget* parent)
    : QWidget(parent)
    , m_correlation(0.0)
    , m_pointAlpha(1.0f)
    , m_backgroundColor(QColor(20, 20, 20))
    , m_gridColor(QColor(50, 50, 50))
    , m_centerColor(QColor(100, 100, 100))
    , m_pointColor(QColor(0, 255, 128))
    , m_monoLineColor(QColor(255, 100, 100))
    , m_stereoLineColor(QColor(100, 100, 255))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Correlation value label
    m_correlationLabel = new QLabel(tr("Correlation: 0.00"), this);
    m_correlationLabel->setAlignment(Qt::AlignCenter);
    m_correlationLabel->setStyleSheet("QLabel { font-size: 12px; padding: 5px; }");
    layout->addWidget(m_correlationLabel);

    setMinimumSize(300, 300);
}

void PhaseMeterWidget::updatePhase(const std::vector<float>& leftChannel,
                                   const std::vector<float>& rightChannel) {
    if (leftChannel.empty() || rightChannel.empty()) return;

    size_t numSamples = std::min(leftChannel.size(), rightChannel.size());

    // Calculate correlation
    m_correlation = calculateCorrelation(leftChannel, rightChannel);

    // Update correlation label with color coding
    QString corrText = QString("Correlation: %1").arg(m_correlation, 0, 'f', 2);
    m_correlationLabel->setText(corrText);

    // Color code based on correlation
    if (m_correlation < -0.5) {
        // Out of phase - RED warning
        m_correlationLabel->setStyleSheet("QLabel { color: #ff4444; font-weight: bold; font-size: 12px; padding: 5px; }");
    } else if (m_correlation < 0.5) {
        // Wide stereo - YELLOW
        m_correlationLabel->setStyleSheet("QLabel { color: #ffaa00; font-size: 12px; padding: 5px; }");
    } else {
        // Good correlation - GREEN
        m_correlationLabel->setStyleSheet("QLabel { color: #44ff44; font-size: 12px; padding: 5px; }");
    }

    // Add new points to buffer (M/S encoding)
    for (size_t i = 0; i < numSamples; i += 4) {  // Changed from 16 to 4 for more points
        float left = leftChannel[i];
        float right = rightChannel[i];

        PhasePoint point;
        point.mid = (left + right) / 2.0f;   // Mid (mono sum)
        point.side = (left - right) / 2.0f;  // Side (stereo difference)

        m_phasePoints.push_back(point);
    }

    // Maintain buffer size
    while (m_phasePoints.size() > MAX_POINTS) {
        m_phasePoints.pop_front();
    }

    update();
}

double PhaseMeterWidget::calculateCorrelation(const std::vector<float>& left,
                                              const std::vector<float>& right) {
    if (left.empty() || right.empty()) return 0.0;

    size_t n = std::min(left.size(), right.size());

    double sumL = 0.0, sumR = 0.0;
    double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;

    for (size_t i = 0; i < n; ++i) {
        sumL += left[i];
        sumR += right[i];
        sumLL += left[i] * left[i];
        sumRR += right[i] * right[i];
        sumLR += left[i] * right[i];
    }

    double meanL = sumL / n;
    double meanR = sumR / n;

    double varL = sumLL / n - meanL * meanL;
    double varR = sumRR / n - meanR * meanR;
    double covar = sumLR / n - meanL * meanR;

    double denominator = std::sqrt(varL * varR);
    if (denominator < 1e-10) return 0.0;

    double correlation = covar / denominator;
    return std::clamp(correlation, -1.0, 1.0);
}

void PhaseMeterWidget::reset() {
    m_correlation = 0.0;
    m_phasePoints.clear();
    m_correlationLabel->setText(tr("Correlation: 0.00"));
    m_correlationLabel->setStyleSheet("QLabel { font-size: 12px; padding: 5px; }");
    update();
}

void PhaseMeterWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Get drawing area (leave space for label)
    int labelHeight = m_correlationLabel->height();
    QRect drawRect(0, labelHeight, width(), height() - labelHeight);

    // Fill background
    painter.fillRect(drawRect, m_backgroundColor);

    int centerX = drawRect.center().x();
    int centerY = drawRect.center().y();
    int radius = std::min(drawRect.width(), drawRect.height()) / 2 - 20;

    // Draw circular grid
    painter.setPen(QPen(m_gridColor, 1));
    painter.drawEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);
    painter.drawEllipse(centerX - radius/2, centerY - radius/2, radius, radius);

    // Draw crosshair (Mid horizontal, Side vertical)
    painter.drawLine(centerX - radius, centerY, centerX + radius, centerY);
    painter.drawLine(centerX, centerY - radius, centerX, centerY + radius);

    // Draw diagonal lines for mono reference
    painter.setPen(QPen(m_monoLineColor, 1, Qt::DashLine));
    painter.drawLine(centerX - radius * 0.7, centerY - radius * 0.7,
                    centerX + radius * 0.7, centerY + radius * 0.7);

    // Draw stereo reference (horizontal spread)
    painter.setPen(QPen(m_stereoLineColor, 1, Qt::DashLine));
    painter.drawLine(centerX - radius, centerY, centerX + radius, centerY);

    // Draw labels
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 9));
    painter.drawText(centerX + radius + 5, centerY + 5, "L");
    painter.drawText(centerX - radius - 15, centerY + 5, "R");
    painter.drawText(centerX + 5, centerY - radius - 5, "M");
    painter.drawText(centerX + 5, centerY + radius + 15, "S");

    // Draw phase points (Lissajous curve)
    if (!m_phasePoints.empty()) {
        painter.setPen(Qt::NoPen);

        size_t numPoints = m_phasePoints.size();
        for (size_t i = 0; i < numPoints; ++i) {
            const PhasePoint& point = m_phasePoints[i];

            // Map M/S to screen coordinates
            int x = centerX + static_cast<int>(point.mid * radius);
            int y = centerY - static_cast<int>(point.side * radius);

            // Fade older points
            float alpha = static_cast<float>(i) / numPoints;
            alpha = 0.3f + alpha * 0.7f;  // Min alpha 0.3 instead of 0.0
            QColor color = m_pointColor;
            color.setAlphaF(alpha * m_pointAlpha);

            painter.setBrush(color);
            painter.drawEllipse(QPointF(x, y), 3, 3);  // Increased from 2 to 3
        }
    }

    // Draw center dot
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_centerColor);
    painter.drawEllipse(QPointF(centerX, centerY), 3, 3);
}

} // namespace VideoStudio
