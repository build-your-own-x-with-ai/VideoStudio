#ifndef AUDIOLEVELWIDGET_H
#define AUDIOLEVELWIDGET_H

#include <QWidget>
#include <QTimer>
#include "core/audioanalyzer.h"

namespace VideoStudio {

class AudioLevelWidget : public QWidget {
    Q_OBJECT

public:
    explicit AudioLevelWidget(QWidget* parent = nullptr);
    ~AudioLevelWidget() override;

    // Update level (0.0 to 1.0)
    void setPeakLevel(float level);
    void setRMSLevel(float level);
    void setdBFSLevel(float dbFS);

    // Auto-decay settings
    void setDecayRate(float decayPerSecond);
    void enableAutoDecay(bool enable);

    // Display settings
    void setOrientation(Qt::Orientation orientation);
    void setShowdBScale(bool show);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void updateDecay();

private:
    void drawHorizontalMeter(QPainter& painter);
    void drawVerticalMeter(QPainter& painter);
    void drawdBScale(QPainter& painter);

    QColor getLevelColor(float level) const;
    float dbToLinear(float db) const;
    float linearToDb(float linear) const;

    // Levels
    float m_peakLevel;
    float m_rmsLevel;
    float m_dbFS;

    // Display peak hold
    float m_peakHold;
    int m_peakHoldTime;

    // Auto-decay
    bool m_autoDecay;
    float m_decayRate;
    QTimer* m_decayTimer;

    // Settings
    Qt::Orientation m_orientation;
    bool m_showdBScale;

    // Colors
    QColor m_backgroundColor;
    QColor m_borderColor;
    QColor m_goodColor;      // -20 to 0 dB
    QColor m_warningColor;   // -6 to 0 dB
    QColor m_dangerColor;    // -3 to 0 dB
    QColor m_peakColor;
};

} // namespace VideoStudio

#endif // AUDIOLEVELWIDGET_H
