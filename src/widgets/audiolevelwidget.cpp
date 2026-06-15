#include "audiolevelwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <cmath>

namespace VideoStudio {

AudioLevelWidget::AudioLevelWidget(QWidget* parent)
    : QWidget(parent)
    , m_peakLevel(0.0f)
    , m_rmsLevel(0.0f)
    , m_dbFS(-std::numeric_limits<float>::infinity())
    , m_peakHold(0.0f)
    , m_peakHoldTime(0)
    , m_autoDecay(true)
    , m_decayRate(0.3f)
    , m_orientation(Qt::Horizontal)
    , m_showdBScale(true)
    , m_backgroundColor(QColor(30, 30, 30))
    , m_borderColor(QColor(100, 100, 100))
    , m_goodColor(QColor(0, 200, 0))
    , m_warningColor(QColor(255, 200, 0))
    , m_dangerColor(QColor(255, 0, 0))
    , m_peakColor(QColor(255, 255, 255))
{
    m_decayTimer = new QTimer(this);
    connect(m_decayTimer, &QTimer::timeout, this, &AudioLevelWidget::updateDecay);
    m_decayTimer->start(50); // Update every 50ms
}

AudioLevelWidget::~AudioLevelWidget() = default;

void AudioLevelWidget::setPeakLevel(float level) {
    m_peakLevel = qBound(0.0f, level, 1.0f);

    // Update peak hold
    if (m_peakLevel > m_peakHold) {
        m_peakHold = m_peakLevel;
        m_peakHoldTime = 1000; // Hold for 1 second
    }

    update();
}

void AudioLevelWidget::setRMSLevel(float level) {
    m_rmsLevel = qBound(0.0f, level, 1.0f);
    update();
}

void AudioLevelWidget::setdBFSLevel(float dbFS) {
    m_dbFS = dbFS;
    update();
}

void AudioLevelWidget::setDecayRate(float decayPerSecond) {
    m_decayRate = decayPerSecond;
}

void AudioLevelWidget::enableAutoDecay(bool enable) {
    m_autoDecay = enable;
}

void AudioLevelWidget::setOrientation(Qt::Orientation orientation) {
    m_orientation = orientation;
    updateGeometry();
    update();
}

void AudioLevelWidget::setShowdBScale(bool show) {
    m_showdBScale = show;
    update();
}

QSize AudioLevelWidget::sizeHint() const {
    if (m_orientation == Qt::Horizontal) {
        return QSize(200, 40);
    } else {
        return QSize(40, 200);
    }
}

QSize AudioLevelWidget::minimumSizeHint() const {
    if (m_orientation == Qt::Horizontal) {
        return QSize(100, 20);
    } else {
        return QSize(20, 100);
    }
}

void AudioLevelWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Fill background
    painter.fillRect(rect(), m_backgroundColor);

    // Draw border
    painter.setPen(m_borderColor);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    if (m_orientation == Qt::Horizontal) {
        drawHorizontalMeter(painter);
    } else {
        drawVerticalMeter(painter);
    }

    if (m_showdBScale) {
        drawdBScale(painter);
    }
}

void AudioLevelWidget::drawHorizontalMeter(QPainter& painter) {
    int w = width() - 4;
    int h = height() - 4;
    int x = 2;
    int y = 2;

    // Draw RMS level (main bar)
    int rmsWidth = static_cast<int>(w * m_rmsLevel);
    if (rmsWidth > 0) {
        QRect rmsRect(x, y + h/3, rmsWidth, h/3);
        painter.fillRect(rmsRect, getLevelColor(m_rmsLevel));
    }

    // Draw peak level (thin line at top)
    int peakWidth = static_cast<int>(w * m_peakLevel);
    if (peakWidth > 0) {
        painter.fillRect(x, y, peakWidth, h/6, getLevelColor(m_peakLevel));
    }

    // Draw peak hold indicator
    if (m_peakHold > 0.0f) {
        int peakHoldX = x + static_cast<int>(w * m_peakHold);
        painter.setPen(QPen(m_peakColor, 2));
        painter.drawLine(peakHoldX, y, peakHoldX, y + h);
    }

    // Draw dB text
    if (m_dbFS > -std::numeric_limits<float>::infinity()) {
        QString dbText = QString::number(m_dbFS, 'f', 1) + " dBFS";
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignRight | Qt::AlignVCenter, dbText);
    }
}

void AudioLevelWidget::drawVerticalMeter(QPainter& painter) {
    int w = width() - 4;
    int h = height() - 4;
    int x = 2;
    int y = 2;

    // Draw RMS level (main bar) - from bottom to top
    int rmsHeight = static_cast<int>(h * m_rmsLevel);
    if (rmsHeight > 0) {
        QRect rmsRect(x + w/3, y + h - rmsHeight, w/3, rmsHeight);
        painter.fillRect(rmsRect, getLevelColor(m_rmsLevel));
    }

    // Draw peak level (thin bar on right)
    int peakHeight = static_cast<int>(h * m_peakLevel);
    if (peakHeight > 0) {
        painter.fillRect(x + 2*w/3, y + h - peakHeight, w/6, peakHeight,
                        getLevelColor(m_peakLevel));
    }

    // Draw peak hold indicator
    if (m_peakHold > 0.0f) {
        int peakHoldY = y + h - static_cast<int>(h * m_peakHold);
        painter.setPen(QPen(m_peakColor, 2));
        painter.drawLine(x, peakHoldY, x + w, peakHoldY);
    }

    // Draw dB text
    if (m_dbFS > -std::numeric_limits<float>::infinity()) {
        QString dbText = QString::number(m_dbFS, 'f', 1);
        painter.setPen(Qt::white);
        painter.save();
        painter.translate(width()/2, height()/2);
        painter.rotate(-90);
        painter.drawText(-50, 0, 100, 20, Qt::AlignCenter, dbText);
        painter.restore();
    }
}

void AudioLevelWidget::drawdBScale(QPainter& painter) {
    painter.setPen(Qt::gray);
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    // Draw scale marks at -60, -40, -20, -10, -6, -3, 0 dB
    QVector<int> marks = {-60, -40, -20, -10, -6, -3, 0};

    for (int db : marks) {
        float level = dbToLinear(static_cast<float>(db));

        if (m_orientation == Qt::Horizontal) {
            int x = 2 + static_cast<int>((width() - 4) * level);
            painter.drawLine(x, height() - 10, x, height() - 5);
            QString label = QString::number(db);
            painter.drawText(x - 15, height() - 12, 30, 10, Qt::AlignCenter, label);
        } else {
            int y = height() - 2 - static_cast<int>((height() - 4) * level);
            painter.drawLine(width() - 10, y, width() - 5, y);
            QString label = QString::number(db);
            painter.drawText(5, y - 5, 30, 10, Qt::AlignLeft | Qt::AlignVCenter, label);
        }
    }
}

void AudioLevelWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
}

void AudioLevelWidget::updateDecay() {
    if (m_autoDecay) {
        // Decay peak level
        float decayAmount = m_decayRate * 0.05f; // 50ms intervals
        m_peakLevel = qMax(0.0f, m_peakLevel - decayAmount);
        m_rmsLevel = qMax(0.0f, m_rmsLevel - decayAmount);

        // Update dBFS
        if (m_peakLevel > 0.0f) {
            m_dbFS = linearToDb(m_peakLevel);
        }

        // Decay peak hold
        if (m_peakHoldTime > 0) {
            m_peakHoldTime -= 50;
            if (m_peakHoldTime <= 0) {
                m_peakHold = 0.0f;
            }
        }

        update();
    }
}

QColor AudioLevelWidget::getLevelColor(float level) const {
    float db = linearToDb(level);

    if (db >= -3.0f) {
        return m_dangerColor; // Red for clipping danger
    } else if (db >= -6.0f) {
        return m_warningColor; // Yellow for warning
    } else {
        return m_goodColor; // Green for good levels
    }
}

float AudioLevelWidget::dbToLinear(float db) const {
    // Convert dB to linear scale (0.0 to 1.0)
    // Assuming 0 dB = 1.0, -60 dB = 0.001 (near silence)
    if (db <= -60.0f) return 0.0f;
    if (db >= 0.0f) return 1.0f;

    return std::pow(10.0f, db / 20.0f);
}

float AudioLevelWidget::linearToDb(float linear) const {
    if (linear <= 0.0f) {
        return -std::numeric_limits<float>::infinity();
    }
    return 20.0f * std::log10(linear);
}

} // namespace VideoStudio
