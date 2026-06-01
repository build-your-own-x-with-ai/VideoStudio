#include "panels/bitratepanel.h"
#include "core/tsparser.h"
#include <QDebug>
#include <QPainterPath>
#include <QToolTip>
#include <cmath>

namespace VideoStudio {

// BitrateChart implementation

BitrateChart::BitrateChart(QWidget* parent)
    : QWidget(parent)
    , m_zoomLevel(1.0)
    , m_offsetX(0.0)
    , m_offsetY(0.0)
    , m_hoveredPointIndex(-1)
    , m_hasCursor(false)
    , m_minBitrate(0.0)
    , m_maxBitrate(0.0)
    , m_avgBitrate(0.0)
{
    setMinimumHeight(300);
    setMouseTracking(true);
}

BitrateChart::~BitrateChart() {
}

void BitrateChart::setData(const QVector<BitratePoint>& data) {
    m_data = data;
    m_zoomLevel = 1.0;
    m_offsetX = 0.0;
    m_offsetY = 0.0;

    // Calculate statistics
    if (!m_data.isEmpty()) {
        m_minBitrate = m_data.first().bitrate;
        m_maxBitrate = m_data.first().bitrate;
        double sum = 0.0;

        for (const BitratePoint& point : m_data) {
            if (point.bitrate < m_minBitrate) m_minBitrate = point.bitrate;
            if (point.bitrate > m_maxBitrate) m_maxBitrate = point.bitrate;
            sum += point.bitrate;
        }

        m_avgBitrate = sum / m_data.size();
    } else {
        m_minBitrate = 0.0;
        m_maxBitrate = 0.0;
        m_avgBitrate = 0.0;
    }

    update();
}

void BitrateChart::clear() {
    m_data.clear();
    m_minBitrate = 0.0;
    m_maxBitrate = 0.0;
    m_avgBitrate = 0.0;
    update();
}

void BitrateChart::zoomIn() {
    m_zoomLevel *= 1.2;
    update();
}

void BitrateChart::zoomOut() {
    m_zoomLevel /= 1.2;
    if (m_zoomLevel < 0.1) m_zoomLevel = 0.1;
    update();
}

void BitrateChart::zoomFit() {
    m_zoomLevel = 1.0;
    m_offsetX = 0.0;
    m_offsetY = 0.0;
    update();
}

QString BitrateChart::formatBitrate(double bps) const {
    if (bps >= 1000000.0) {
        return QString("%1 Mbps").arg(bps / 1000000.0, 0, 'f', 2);
    } else if (bps >= 1000.0) {
        return QString("%1 Kbps").arg(bps / 1000.0, 0, 'f', 2);
    } else {
        return QString("%1 bps").arg(bps, 0, 'f', 0);
    }
}

void BitrateChart::paintEvent(QPaintEvent* event) {
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
    drawStatistics(painter);

    if (m_hasCursor) {
        drawCursor(painter);
    }
}

void BitrateChart::drawAxes(QPainter& painter) {
    painter.setPen(QPen(Qt::white, 2));

    int margin = 60;

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
    painter.drawText(0, 0, "Bitrate");
    painter.restore();

    // Y axis scale labels
    int numLabels = 5;
    for (int i = 0; i <= numLabels; ++i) {
        double bitrate = m_minBitrate + (m_maxBitrate - m_minBitrate) * i / numLabels;
        int y = height() - margin - (height() - 2 * margin) * i / numLabels;
        painter.drawText(5, y + 5, formatBitrate(bitrate));
    }
}

void BitrateChart::drawGrid(QPainter& painter) {
    painter.setPen(QPen(QColor(60, 60, 60), 1, Qt::DotLine));

    int margin = 60;
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

void BitrateChart::drawData(QPainter& painter) {
    if (m_data.size() < 2) return;

    int64_t minOffset = m_data.first().offset;
    int64_t maxOffset = m_data.last().offset;

    int margin = 60;
    int w = width() - margin * 2;
    int h = height() - margin * 2;

    // Draw area fill
    painter.setPen(Qt::NoPen);
    QLinearGradient gradient(0, margin, 0, height() - margin);
    gradient.setColorAt(0, QColor(100, 200, 255, 100));
    gradient.setColorAt(1, QColor(100, 200, 255, 20));
    painter.setBrush(gradient);

    QPainterPath areaPath;
    bool firstPoint = true;

    for (int i = 0; i < m_data.size(); ++i) {
        const BitratePoint& point = m_data[i];

        double xRatio = (maxOffset > minOffset) ?
            (double)(point.offset - minOffset) / (maxOffset - minOffset) : 0.5;
        double yRatio = (m_maxBitrate > m_minBitrate) ?
            (point.bitrate - m_minBitrate) / (m_maxBitrate - m_minBitrate) : 0.5;

        int x = margin + (int)(xRatio * w * m_zoomLevel + m_offsetX);
        int y = height() - margin - (int)(yRatio * h * m_zoomLevel + m_offsetY);

        if (firstPoint) {
            areaPath.moveTo(x, height() - margin);
            areaPath.lineTo(x, y);
            firstPoint = false;
        } else {
            areaPath.lineTo(x, y);
        }
    }

    // Close the area path
    if (!m_data.isEmpty()) {
        const BitratePoint& lastPoint = m_data.last();
        double xRatio = (maxOffset > minOffset) ?
            (double)(lastPoint.offset - minOffset) / (maxOffset - minOffset) : 0.5;
        int x = margin + (int)(xRatio * w * m_zoomLevel + m_offsetX);
        areaPath.lineTo(x, height() - margin);
    }

    painter.drawPath(areaPath);

    // Draw line
    painter.setPen(QPen(QColor(100, 200, 255), 2));
    QPainterPath linePath;

    for (int i = 0; i < m_data.size(); ++i) {
        const BitratePoint& point = m_data[i];

        double xRatio = (maxOffset > minOffset) ?
            (double)(point.offset - minOffset) / (maxOffset - minOffset) : 0.5;
        double yRatio = (m_maxBitrate > m_minBitrate) ?
            (point.bitrate - m_minBitrate) / (m_maxBitrate - m_minBitrate) : 0.5;

        int x = margin + (int)(xRatio * w * m_zoomLevel + m_offsetX);
        int y = height() - margin - (int)(yRatio * h * m_zoomLevel + m_offsetY);

        if (i == 0) {
            linePath.moveTo(x, y);
        } else {
            linePath.lineTo(x, y);
        }
    }

    painter.drawPath(linePath);

    // Draw points
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(100, 200, 255));

    for (int i = 0; i < m_data.size(); ++i) {
        const BitratePoint& point = m_data[i];

        double xRatio = (maxOffset > minOffset) ?
            (double)(point.offset - minOffset) / (maxOffset - minOffset) : 0.5;
        double yRatio = (m_maxBitrate > m_minBitrate) ?
            (point.bitrate - m_minBitrate) / (m_maxBitrate - m_minBitrate) : 0.5;

        int x = margin + (int)(xRatio * w * m_zoomLevel + m_offsetX);
        int y = height() - margin - (int)(yRatio * h * m_zoomLevel + m_offsetY);

        int radius = (i == m_hoveredPointIndex) ? 6 : 3;
        painter.drawEllipse(QPoint(x, y), radius, radius);
    }

    // Draw average line
    if (m_maxBitrate > m_minBitrate) {
        double avgYRatio = (m_avgBitrate - m_minBitrate) / (m_maxBitrate - m_minBitrate);
        int avgY = height() - margin - (int)(avgYRatio * h);

        painter.setPen(QPen(QColor(255, 200, 100), 2, Qt::DashLine));
        painter.drawLine(margin, avgY, width() - margin, avgY);
    }
}

void BitrateChart::drawStatistics(QPainter& painter) {
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10));

    int x = width() - 200;
    int y = 20;

    painter.drawText(x, y, QString("Min: %1").arg(formatBitrate(m_minBitrate)));
    painter.drawText(x, y + 20, QString("Max: %1").arg(formatBitrate(m_maxBitrate)));

    painter.setPen(QColor(255, 200, 100));
    painter.drawText(x, y + 40, QString("Avg: %1").arg(formatBitrate(m_avgBitrate)));
}

void BitrateChart::drawCursor(QPainter& painter) {
    painter.setPen(QPen(Qt::yellow, 1, Qt::DashLine));
    painter.drawLine(m_cursorPos.x(), 0, m_cursorPos.x(), height());
    painter.drawLine(0, m_cursorPos.y(), width(), m_cursorPos.y());
}

void BitrateChart::mouseMoveEvent(QMouseEvent* event) {
    m_cursorPos = event->pos();
    m_hasCursor = true;

    int nearestIndex = findNearestPoint(m_cursorPos);
    if (nearestIndex != m_hoveredPointIndex) {
        m_hoveredPointIndex = nearestIndex;

        if (nearestIndex >= 0 && nearestIndex < m_data.size()) {
            const BitratePoint& point = m_data[nearestIndex];
            QString tooltip = QString("Packet #%1\nOffset: 0x%2\nBitrate: %3")
                .arg(point.packetIndex)
                .arg(point.offset, 0, 16)
                .arg(formatBitrate(point.bitrate));
            QToolTip::showText(event->globalPosition().toPoint(), tooltip, this);
        }
    }

    update();
}

void BitrateChart::mouseDoubleClickEvent(QMouseEvent* event) {
    int nearestIndex = findNearestPoint(event->pos());
    if (nearestIndex >= 0 && nearestIndex < m_data.size()) {
        emit pointClicked(m_data[nearestIndex].packetIndex);
    }
}

void BitrateChart::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0) {
        zoomIn();
    } else {
        zoomOut();
    }
}

int BitrateChart::findNearestPoint(const QPoint& pos) const {
    if (m_data.isEmpty()) return -1;

    int64_t minOffset = m_data.first().offset;
    int64_t maxOffset = m_data.last().offset;

    int margin = 60;
    int w = width() - margin * 2;
    int h = height() - margin * 2;

    int nearestIndex = -1;
    double minDistance = 20.0;

    for (int i = 0; i < m_data.size(); ++i) {
        const BitratePoint& point = m_data[i];

        double xRatio = (maxOffset > minOffset) ?
            (double)(point.offset - minOffset) / (maxOffset - minOffset) : 0.5;
        double yRatio = (m_maxBitrate > m_minBitrate) ?
            (point.bitrate - m_minBitrate) / (m_maxBitrate - m_minBitrate) : 0.5;

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

// BitratePanel implementation

BitratePanel::BitratePanel(QWidget* parent)
    : QWidget(parent)
    , m_parser(nullptr)
    , m_currentPID(0)
    , m_windowSize(100)
    , m_showAllPIDs(false)
{
    createUI();
}

BitratePanel::~BitratePanel() {
}

void BitratePanel::createUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Control bar
    QHBoxLayout* controlLayout = new QHBoxLayout();

    QLabel* modeLabel = new QLabel("Mode:", this);
    controlLayout->addWidget(modeLabel);

    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem("Overall Bitrate");
    m_modeCombo->addItem("Per-PID Bitrate");
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BitratePanel::onModeChanged);
    controlLayout->addWidget(m_modeCombo);

    QLabel* pidLabel = new QLabel("PID:", this);
    controlLayout->addWidget(pidLabel);

    m_pidCombo = new QComboBox(this);
    connect(m_pidCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BitratePanel::onPIDChanged);
    controlLayout->addWidget(m_pidCombo);

    QLabel* windowLabel = new QLabel("Window:", this);
    controlLayout->addWidget(windowLabel);

    m_windowCombo = new QComboBox(this);
    m_windowCombo->addItem("50 packets", 50);
    m_windowCombo->addItem("100 packets", 100);
    m_windowCombo->addItem("200 packets", 200);
    m_windowCombo->addItem("500 packets", 500);
    m_windowCombo->setCurrentIndex(1); // Default 100
    connect(m_windowCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BitratePanel::onWindowSizeChanged);
    controlLayout->addWidget(m_windowCombo);

    m_showAllPIDsCheckbox = new QCheckBox("Show All PIDs", this);
    connect(m_showAllPIDsCheckbox, &QCheckBox::toggled,
            this, &BitratePanel::onShowAllPIDsToggled);
    controlLayout->addWidget(m_showAllPIDsCheckbox);

    controlLayout->addStretch();

    // Zoom controls
    m_fitButton = new QPushButton("Fit", this);
    connect(m_fitButton, &QPushButton::clicked, this, &BitratePanel::onZoomFit);
    controlLayout->addWidget(m_fitButton);

    m_zoomInButton = new QPushButton("+", this);
    m_zoomInButton->setMaximumWidth(30);
    connect(m_zoomInButton, &QPushButton::clicked, this, &BitratePanel::onZoomIn);
    controlLayout->addWidget(m_zoomInButton);

    m_zoomOutButton = new QPushButton("-", this);
    m_zoomOutButton->setMaximumWidth(30);
    connect(m_zoomOutButton, &QPushButton::clicked, this, &BitratePanel::onZoomOut);
    controlLayout->addWidget(m_zoomOutButton);

    mainLayout->addLayout(controlLayout);

    // Info label
    m_infoLabel = new QLabel("No data", this);
    m_infoLabel->setStyleSheet("color: #888; font-style: italic;");
    mainLayout->addWidget(m_infoLabel);

    // Chart
    m_chart = new BitrateChart(this);
    connect(m_chart, &BitrateChart::pointClicked,
            this, &BitratePanel::onChartPointClicked);
    mainLayout->addWidget(m_chart);
}

void BitratePanel::setTSParser(TSParser* parser) {
    m_parser = parser;
    updatePIDList();
    updateChart();
}

void BitratePanel::clear() {
    m_parser = nullptr;
    m_pidCombo->clear();
    m_chart->clear();
    m_infoLabel->setText("No data");
}

void BitratePanel::updatePIDList() {
    m_pidCombo->clear();

    if (!m_parser) {
        return;
    }

    const auto& pids = m_parser->getPIDs();

    for (auto it = pids.begin(); it != pids.end(); ++it) {
        uint16_t pid = it.key();
        const PIDInfo& pidInfo = it.value();

        QString pidText = QString("0x%1 - %2")
            .arg(pid, 4, 16, QChar('0'))
            .arg(pidInfo.type.isEmpty() ? "Unknown" : pidInfo.type);

        m_pidCombo->addItem(pidText, pid);
    }

    if (m_pidCombo->count() > 0) {
        m_currentPID = m_pidCombo->itemData(0).toUInt();
    }
}

void BitratePanel::updateChart() {
    if (!m_parser) {
        m_chart->clear();
        m_infoLabel->setText("No data");
        return;
    }

    int mode = m_modeCombo->currentIndex();

    if (mode == 0) {
        // Overall bitrate
        analyzeOverallBitrate();
    } else {
        // Per-PID bitrate
        if (m_showAllPIDs) {
            analyzeAllPIDsBitrate();
        } else if (m_currentPID != 0) {
            analyzePIDBitrate(m_currentPID);
        }
    }
}

void BitratePanel::analyzeOverallBitrate() {
    const auto& packets = m_parser->getPackets();

    QVector<int> allPackets;
    for (int i = 0; i < packets.size(); ++i) {
        allPackets.append(i);
    }

    QVector<BitratePoint> data = calculateBitrate(allPackets, m_windowSize);
    m_chart->setData(data);
    m_infoLabel->setText(QString("Overall bitrate: %1 points (window: %2 packets)")
        .arg(data.size())
        .arg(m_windowSize));
}

void BitratePanel::analyzePIDBitrate(uint16_t pid) {
    const auto& packets = m_parser->getPackets();

    QVector<int> pidPackets;
    for (int i = 0; i < packets.size(); ++i) {
        if (packets[i].pid == pid) {
            pidPackets.append(i);
        }
    }

    QVector<BitratePoint> data = calculateBitrate(pidPackets, m_windowSize);
    m_chart->setData(data);
    m_infoLabel->setText(QString("Bitrate for PID 0x%1: %2 points (window: %3 packets)")
        .arg(pid, 4, 16, QChar('0'))
        .arg(data.size())
        .arg(m_windowSize));
}

void BitratePanel::analyzeAllPIDsBitrate() {
    // For now, just show overall bitrate when "Show All PIDs" is checked
    // In a full implementation, this would overlay multiple PID bitrates
    analyzeOverallBitrate();
}

QVector<BitratePoint> BitratePanel::calculateBitrate(const QVector<int>& packetIndices, int windowSize) {
    QVector<BitratePoint> result;

    if (packetIndices.size() < windowSize || !m_parser) {
        return result;
    }

    const auto& packets = m_parser->getPackets();

    for (int i = 0; i <= packetIndices.size() - windowSize; i += windowSize / 2) {
        int startIdx = packetIndices[i];
        int endIdx = packetIndices[qMin(i + windowSize - 1, packetIndices.size() - 1)];

        const TSPacket& startPacket = packets[startIdx];
        const TSPacket& endPacket = packets[endIdx];

        int64_t bytesDiff = endPacket.offset - startPacket.offset + 188;

        // Assume 27 MHz clock (standard for MPEG-TS)
        // Bitrate = bytes * 8 bits/byte
        double bitrate = bytesDiff * 8.0;

        BitratePoint point;
        point.offset = startPacket.offset;
        point.bitrate = bitrate;
        point.packetIndex = startIdx;

        result.append(point);
    }

    return result;
}

void BitratePanel::onModeChanged(int index) {
    Q_UNUSED(index);
    updateChart();
}

void BitratePanel::onPIDChanged(int index) {
    if (index >= 0) {
        m_currentPID = m_pidCombo->itemData(index).toUInt();
        updateChart();
    }
}

void BitratePanel::onWindowSizeChanged(int index) {
    if (index >= 0) {
        m_windowSize = m_windowCombo->itemData(index).toInt();
        updateChart();
    }
}

void BitratePanel::onZoomIn() {
    m_chart->zoomIn();
}

void BitratePanel::onZoomOut() {
    m_chart->zoomOut();
}

void BitratePanel::onZoomFit() {
    m_chart->zoomFit();
}

void BitratePanel::onChartPointClicked(int packetIndex) {
    emit packetClicked(packetIndex);
    qDebug() << "Bitrate chart point clicked, packet:" << packetIndex;
}

void BitratePanel::onShowAllPIDsToggled(bool checked) {
    m_showAllPIDs = checked;
    updateChart();
}

} // namespace VideoStudio
