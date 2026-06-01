#include "panels/timedynamicspanel.h"
#include "core/tsparser.h"
#include "core/timinganalyzer.h"
#include <QDebug>
#include <QPainterPath>
#include <QToolTip>
#include <cmath>

namespace VideoStudio {

// TimeDynamicsChart implementation

TimeDynamicsChart::TimeDynamicsChart(QWidget* parent)
    : QWidget(parent)
    , m_zoomLevel(1.0)
    , m_offsetX(0.0)
    , m_offsetY(0.0)
    , m_hoveredPointIndex(-1)
    , m_hasCursor(false)
{
    setMinimumHeight(300);
    setMouseTracking(true);
}

TimeDynamicsChart::~TimeDynamicsChart() {
}

void TimeDynamicsChart::setData(const QVector<TimePoint>& data) {
    m_data = data;
    m_zoomLevel = 1.0;
    m_offsetX = 0.0;
    m_offsetY = 0.0;
    update();
}

void TimeDynamicsChart::clear() {
    m_data.clear();
    update();
}

void TimeDynamicsChart::zoomIn() {
    m_zoomLevel *= 1.2;
    update();
}

void TimeDynamicsChart::zoomOut() {
    m_zoomLevel /= 1.2;
    if (m_zoomLevel < 0.1) m_zoomLevel = 0.1;
    update();
}

void TimeDynamicsChart::zoomFit() {
    m_zoomLevel = 1.0;
    m_offsetX = 0.0;
    m_offsetY = 0.0;
    update();
}

void TimeDynamicsChart::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(rect(), QColor(30, 30, 30));

    if (m_data.isEmpty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, "No data to display");
        return;
    }

    drawGrid(painter);
    drawAxes(painter);
    drawData(painter);

    if (m_hasCursor) {
        drawCursor(painter);
    }
}

void TimeDynamicsChart::drawAxes(QPainter& painter) {
    painter.setPen(QPen(Qt::white, 2));

    int margin = 50;
    int w = width() - margin * 2;
    int h = height() - margin * 2;

    // X axis
    painter.drawLine(margin, height() - margin, width() - margin, height() - margin);

    // Y axis
    painter.drawLine(margin, margin, margin, height() - margin);

    // Labels
    painter.setFont(QFont("Arial", 10));
    painter.drawText(width() / 2 - 50, height() - 10, "Offset (bytes)");

    painter.save();
    painter.translate(15, height() / 2 + 50);
    painter.rotate(-90);
    painter.drawText(0, 0, "Timestamp");
    painter.restore();
}

void TimeDynamicsChart::drawGrid(QPainter& painter) {
    painter.setPen(QPen(QColor(60, 60, 60), 1, Qt::DotLine));

    int margin = 50;
    int w = width() - margin * 2;
    int h = height() - margin * 2;

    // Vertical grid lines
    for (int i = 0; i <= 10; ++i) {
        int x = margin + (w * i) / 10;
        painter.drawLine(x, margin, x, height() - margin);
    }

    // Horizontal grid lines
    for (int i = 0; i <= 10; ++i) {
        int y = margin + (h * i) / 10;
        painter.drawLine(margin, y, width() - margin, y);
    }
}

void TimeDynamicsChart::drawData(QPainter& painter) {
    if (m_data.size() < 2) return;

    // Find min/max for scaling
    int64_t minOffset = m_data.first().offset;
    int64_t maxOffset = m_data.last().offset;
    int64_t minTimestamp = m_data.first().timestamp;
    int64_t maxTimestamp = m_data.first().timestamp;

    for (const TimePoint& point : m_data) {
        if (point.timestamp < minTimestamp) minTimestamp = point.timestamp;
        if (point.timestamp > maxTimestamp) maxTimestamp = point.timestamp;
    }

    int margin = 50;
    int w = width() - margin * 2;
    int h = height() - margin * 2;

    // Draw line
    painter.setPen(QPen(QColor(100, 200, 255), 2));
    QPainterPath path;

    for (int i = 0; i < m_data.size(); ++i) {
        const TimePoint& point = m_data[i];

        double xRatio = (maxOffset > minOffset) ?
            (double)(point.offset - minOffset) / (maxOffset - minOffset) : 0.5;
        double yRatio = (maxTimestamp > minTimestamp) ?
            (double)(point.timestamp - minTimestamp) / (maxTimestamp - minTimestamp) : 0.5;

        int x = margin + (int)(xRatio * w * m_zoomLevel + m_offsetX);
        int y = height() - margin - (int)(yRatio * h * m_zoomLevel + m_offsetY);

        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    painter.drawPath(path);

    // Draw points
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(100, 200, 255));

    for (int i = 0; i < m_data.size(); ++i) {
        const TimePoint& point = m_data[i];

        double xRatio = (maxOffset > minOffset) ?
            (double)(point.offset - minOffset) / (maxOffset - minOffset) : 0.5;
        double yRatio = (maxTimestamp > minTimestamp) ?
            (double)(point.timestamp - minTimestamp) / (maxTimestamp - minTimestamp) : 0.5;

        int x = margin + (int)(xRatio * w * m_zoomLevel + m_offsetX);
        int y = height() - margin - (int)(yRatio * h * m_zoomLevel + m_offsetY);

        int radius = (i == m_hoveredPointIndex) ? 6 : 3;
        painter.drawEllipse(QPoint(x, y), radius, radius);
    }
}

void TimeDynamicsChart::drawCursor(QPainter& painter) {
    painter.setPen(QPen(Qt::yellow, 1, Qt::DashLine));
    painter.drawLine(m_cursorPos.x(), 0, m_cursorPos.x(), height());
    painter.drawLine(0, m_cursorPos.y(), width(), m_cursorPos.y());
}

void TimeDynamicsChart::mouseMoveEvent(QMouseEvent* event) {
    m_cursorPos = event->pos();
    m_hasCursor = true;

    int nearestIndex = findNearestPoint(m_cursorPos);
    if (nearestIndex != m_hoveredPointIndex) {
        m_hoveredPointIndex = nearestIndex;

        if (nearestIndex >= 0 && nearestIndex < m_data.size()) {
            const TimePoint& point = m_data[nearestIndex];
            QString tooltip = QString("Packet #%1\nOffset: 0x%2\nTimestamp: %3")
                .arg(point.packetIndex)
                .arg(point.offset, 0, 16)
                .arg(point.timestamp);
            QToolTip::showText(event->globalPosition().toPoint(), tooltip, this);
        }
    }

    update();
}

void TimeDynamicsChart::mouseDoubleClickEvent(QMouseEvent* event) {
    int nearestIndex = findNearestPoint(event->pos());
    if (nearestIndex >= 0 && nearestIndex < m_data.size()) {
        emit pointClicked(m_data[nearestIndex].packetIndex);
    }
}

void TimeDynamicsChart::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0) {
        zoomIn();
    } else {
        zoomOut();
    }
}

int TimeDynamicsChart::findNearestPoint(const QPoint& pos) const {
    if (m_data.isEmpty()) return -1;

    int64_t minOffset = m_data.first().offset;
    int64_t maxOffset = m_data.last().offset;
    int64_t minTimestamp = m_data.first().timestamp;
    int64_t maxTimestamp = m_data.first().timestamp;

    for (const TimePoint& point : m_data) {
        if (point.timestamp < minTimestamp) minTimestamp = point.timestamp;
        if (point.timestamp > maxTimestamp) maxTimestamp = point.timestamp;
    }

    int margin = 50;
    int w = width() - margin * 2;
    int h = height() - margin * 2;

    int nearestIndex = -1;
    double minDistance = 20.0; // Threshold in pixels

    for (int i = 0; i < m_data.size(); ++i) {
        const TimePoint& point = m_data[i];

        double xRatio = (maxOffset > minOffset) ?
            (double)(point.offset - minOffset) / (maxOffset - minOffset) : 0.5;
        double yRatio = (maxTimestamp > minTimestamp) ?
            (double)(point.timestamp - minTimestamp) / (maxTimestamp - minTimestamp) : 0.5;

        int x = margin + (int)(xRatio * w * m_zoomLevel + m_offsetX);
        int y = height() - margin - (int)(yRatio * h * m_zoomLevel + m_offsetY);

        double distance = std::sqrt(std::pow(pos.x() - x, 2) + std::pow(pos.y() - y, 2));
        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

// TimeDynamicsPanel implementation

TimeDynamicsPanel::TimeDynamicsPanel(QWidget* parent)
    : QWidget(parent)
    , m_parser(nullptr)
    , m_analyzer(new TimingAnalyzer(this))
    , m_currentMode(TimeDynamicsMode::PTSDTSDynamics)
    , m_currentPID(0)
{
    createUI();
}

TimeDynamicsPanel::~TimeDynamicsPanel() {
}

void TimeDynamicsPanel::createUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Control bar
    QHBoxLayout* controlLayout = new QHBoxLayout();

    QLabel* modeLabel = new QLabel("Mode:", this);
    controlLayout->addWidget(modeLabel);

    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem("PTS/DTS Dynamics");
    m_modeCombo->addItem("PCR Dynamics");
    m_modeCombo->addItem("PCR/PTS Dynamics");
    m_modeCombo->addItem("Offset/PCR Dynamics");
    m_modeCombo->addItem("PCR Accuracy");
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TimeDynamicsPanel::onModeChanged);
    controlLayout->addWidget(m_modeCombo);

    QLabel* pidLabel = new QLabel("PID:", this);
    controlLayout->addWidget(pidLabel);

    m_pidCombo = new QComboBox(this);
    connect(m_pidCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TimeDynamicsPanel::onPIDChanged);
    controlLayout->addWidget(m_pidCombo);

    controlLayout->addStretch();

    // Zoom controls
    m_fitButton = new QPushButton("Fit", this);
    connect(m_fitButton, &QPushButton::clicked, this, &TimeDynamicsPanel::onZoomFit);
    controlLayout->addWidget(m_fitButton);

    m_zoomInButton = new QPushButton("+", this);
    m_zoomInButton->setMaximumWidth(30);
    connect(m_zoomInButton, &QPushButton::clicked, this, &TimeDynamicsPanel::onZoomIn);
    controlLayout->addWidget(m_zoomInButton);

    m_zoomOutButton = new QPushButton("-", this);
    m_zoomOutButton->setMaximumWidth(30);
    connect(m_zoomOutButton, &QPushButton::clicked, this, &TimeDynamicsPanel::onZoomOut);
    controlLayout->addWidget(m_zoomOutButton);

    mainLayout->addLayout(controlLayout);

    // Info label
    m_infoLabel = new QLabel("No data", this);
    m_infoLabel->setStyleSheet("color: #888; font-style: italic;");
    mainLayout->addWidget(m_infoLabel);

    // Chart
    m_chart = new TimeDynamicsChart(this);
    connect(m_chart, &TimeDynamicsChart::pointClicked,
            this, &TimeDynamicsPanel::onChartPointClicked);
    mainLayout->addWidget(m_chart);
}

void TimeDynamicsPanel::setTSParser(TSParser* parser) {
    m_parser = parser;
    m_analyzer->setTSParser(parser);
    updatePIDList();
    updateChart();
}

void TimeDynamicsPanel::clear() {
    m_parser = nullptr;
    m_pidCombo->clear();
    m_chart->clear();
    m_infoLabel->setText("No data");
}

void TimeDynamicsPanel::updatePIDList() {
    m_pidCombo->clear();

    if (!m_parser) {
        return;
    }

    const auto& pids = m_parser->getPIDs();

    // Analyze all PIDs to find which have timing data
    QSet<uint16_t> pidsWithTiming;

    for (auto it = pids.begin(); it != pids.end(); ++it) {
        uint16_t pid = it.key();

        // Quick check: analyze this PID
        m_analyzer->analyzePID(pid);

        // Check if it has any timing data
        if (!m_analyzer->getPTSData(pid).isEmpty() ||
            !m_analyzer->getDTSData(pid).isEmpty() ||
            !m_analyzer->getPCRData(pid).isEmpty()) {
            pidsWithTiming.insert(pid);
        }
    }

    // Add PIDs with timing data to combo box
    for (uint16_t pid : pidsWithTiming) {
        const PIDInfo& pidInfo = pids[pid];
        QString pidText = QString("0x%1 - %2")
            .arg(pid, 4, 16, QChar('0'))
            .arg(pidInfo.type.isEmpty() ? "Unknown" : pidInfo.type);

        m_pidCombo->addItem(pidText, pid);
    }

    if (m_pidCombo->count() > 0) {
        m_currentPID = m_pidCombo->itemData(0).toUInt();
    } else {
        m_currentPID = 0;
        m_infoLabel->setText("No PIDs with timing data found");
    }
}

void TimeDynamicsPanel::updateChart() {
    if (!m_parser || m_currentPID == 0) {
        m_chart->clear();
        m_infoLabel->setText("No data");
        return;
    }

    // Analyze the PID using TimingAnalyzer
    m_analyzer->analyzePID(m_currentPID);

    QVector<TimePoint> data;

    switch (m_currentMode) {
        case TimeDynamicsMode::PTSDTSDynamics: {
            // Get PTS data from analyzer
            const auto& ptsData = m_analyzer->getPTSData(m_currentPID);
            for (const auto& point : ptsData) {
                TimePoint tp;
                tp.offset = point.offset;
                tp.timestamp = point.timestamp;
                tp.packetIndex = point.packetIndex;
                data.append(tp);
            }
            m_infoLabel->setText(QString("PTS/DTS for PID 0x%1: %2 points")
                .arg(m_currentPID, 4, 16, QChar('0'))
                .arg(data.size()));
            break;
        }
        case TimeDynamicsMode::PCRDynamics: {
            // Get PCR data from analyzer
            const auto& pcrData = m_analyzer->getPCRData(m_currentPID);
            for (const auto& point : pcrData) {
                TimePoint tp;
                tp.offset = point.offset;
                tp.timestamp = point.timestamp;
                tp.packetIndex = point.packetIndex;
                data.append(tp);
            }
            m_infoLabel->setText(QString("PCR for PID 0x%1: %2 points")
                .arg(m_currentPID, 4, 16, QChar('0'))
                .arg(data.size()));
            break;
        }
        case TimeDynamicsMode::PCRPTSDynamics: {
            // Show PTS in relation to PCR
            const auto& ptsData = m_analyzer->getPTSData(m_currentPID);
            for (const auto& point : ptsData) {
                TimePoint tp;
                tp.offset = point.offset;
                tp.timestamp = point.timestamp;
                tp.packetIndex = point.packetIndex;
                data.append(tp);
            }
            m_infoLabel->setText(QString("PCR/PTS for PID 0x%1: %2 points")
                .arg(m_currentPID, 4, 16, QChar('0'))
                .arg(data.size()));
            break;
        }
        case TimeDynamicsMode::OffsetPCRDynamics: {
            // Show offset variation between PCR values
            const auto& pcrData = m_analyzer->getPCRData(m_currentPID);
            for (int i = 1; i < pcrData.size(); ++i) {
                TimePoint tp;
                tp.offset = pcrData[i].offset;
                tp.timestamp = pcrData[i].offset - pcrData[i-1].offset;
                tp.packetIndex = pcrData[i].packetIndex;
                data.append(tp);
            }
            m_infoLabel->setText(QString("Offset/PCR for PID 0x%1: %2 intervals")
                .arg(m_currentPID, 4, 16, QChar('0'))
                .arg(data.size()));
            break;
        }
        case TimeDynamicsMode::PCRAccuracy: {
            // Show PCR accuracy (±500ns)
            const auto& accuracyData = m_analyzer->getPCRAccuracyData(m_currentPID);
            for (const auto& acc : accuracyData) {
                TimePoint tp;
                tp.offset = acc.offset;
                tp.timestamp = static_cast<int64_t>(acc.differenceNs);
                tp.packetIndex = acc.packetIndex;
                data.append(tp);
            }
            m_infoLabel->setText(QString("PCR Accuracy for PID 0x%1: %2 points")
                .arg(m_currentPID, 4, 16, QChar('0'))
                .arg(data.size()));
            break;
        }
    }

    m_chart->setData(data);

    // Display statistics
    const TimingStats& stats = m_analyzer->getTimingStats(m_currentPID);
    QString statsInfo = QString(" | PTS: %1, DTS: %2, PCR: %3")
        .arg(stats.ptsCount)
        .arg(stats.dtsCount)
        .arg(stats.pcrCount);
    m_infoLabel->setText(m_infoLabel->text() + statsInfo);
}

void TimeDynamicsPanel::onModeChanged(int index) {
    m_currentMode = static_cast<TimeDynamicsMode>(index);
    updateChart();
}

void TimeDynamicsPanel::onPIDChanged(int index) {
    if (index >= 0) {
        m_currentPID = m_pidCombo->itemData(index).toUInt();
        updateChart();
    }
}

void TimeDynamicsPanel::onZoomIn() {
    m_chart->zoomIn();
}

void TimeDynamicsPanel::onZoomOut() {
    m_chart->zoomOut();
}

void TimeDynamicsPanel::onZoomFit() {
    m_chart->zoomFit();
}

void TimeDynamicsPanel::onChartPointClicked(int packetIndex) {
    emit packetClicked(packetIndex);
    qDebug() << "Time dynamics chart point clicked, packet:" << packetIndex;
}

} // namespace VideoStudio
