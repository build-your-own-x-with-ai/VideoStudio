#include "panels/graphicspanel.h"
#include "core/tsparser.h"
#include <QDebug>
#include <QPainterPath>
#include <QToolTip>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QDialog>
#include <QListWidget>
#include <cmath>

namespace VideoStudio {

// GraphicsChart implementation

GraphicsChart::GraphicsChart(QWidget* parent)
    : QWidget(parent)
    , m_mode(GraphicsMode::Line)
    , m_rangingMode(RangingMode::Offset)
    , m_zoomLevel(1.0)
    , m_offsetX(0.0)
    , m_offsetY(0.0)
    , m_hoveredPointIndex(-1)
    , m_hasCursor(false)
{
    setMinimumHeight(400);
    setMouseTracking(true);
}

GraphicsChart::~GraphicsChart() {
}

void GraphicsChart::addParameter(const GraphicsParameter& param) {
    // Check if parameter already exists
    for (int i = 0; i < m_parameters.size(); ++i) {
        if (m_parameters[i].name == param.name) {
            m_parameters[i] = param;
            update();
            return;
        }
    }

    m_parameters.append(param);
    update();
}

void GraphicsChart::removeParameter(const QString& name) {
    for (int i = 0; i < m_parameters.size(); ++i) {
        if (m_parameters[i].name == name) {
            m_parameters.removeAt(i);
            break;
        }
    }
    update();
}

void GraphicsChart::clearParameters() {
    m_parameters.clear();
    update();
}

void GraphicsChart::setMode(GraphicsMode mode) {
    m_mode = mode;
    update();
}

void GraphicsChart::setRangingMode(RangingMode mode) {
    m_rangingMode = mode;
    update();
}

void GraphicsChart::zoomIn() {
    m_zoomLevel *= 1.2;
    qDebug() << "GraphicsChart::zoomIn() - new zoom level:" << m_zoomLevel;
    update();
}

void GraphicsChart::zoomOut() {
    m_zoomLevel /= 1.2;
    if (m_zoomLevel < 0.1) m_zoomLevel = 0.1;
    qDebug() << "GraphicsChart::zoomOut() - new zoom level:" << m_zoomLevel;
    update();
}

void GraphicsChart::zoomFit() {
    m_zoomLevel = 1.0;
    m_offsetX = 0.0;
    m_offsetY = 0.0;
    update();
}

void GraphicsChart::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(rect(), QColor(30, 30, 30));

    if (m_parameters.isEmpty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, "No parameters to display\n\nAdd parameters from Property Panel");
        return;
    }

    drawGrid(painter);
    drawAxes(painter);

    switch (m_mode) {
        case GraphicsMode::Line:
            drawDataLine(painter);
            break;
        case GraphicsMode::Bar:
            drawDataBar(painter);
            break;
        case GraphicsMode::Transition:
            drawDataTransition(painter);
            break;
    }

    drawLegend(painter);

    if (m_hasCursor) {
        drawCursor(painter);
    }
}

void GraphicsChart::drawAxes(QPainter& painter) {
    painter.setPen(QPen(Qt::white, 2));

    int margin = 60;

    // X axis
    painter.drawLine(margin, height() - margin, width() - margin, height() - margin);

    // Y axis
    painter.drawLine(margin, margin, margin, height() - margin);

    // Labels
    painter.setFont(QFont("Arial", 10));

    if (m_rangingMode == RangingMode::Offset) {
        painter.drawText(width() / 2 - 50, height() - 10, "Offset (bytes)");
    } else {
        painter.drawText(width() / 2 - 50, height() - 10, "Count");
    }

    painter.save();
    painter.translate(15, height() / 2 + 50);
    painter.rotate(-90);
    painter.drawText(0, 0, "Value");
    painter.restore();
}

void GraphicsChart::drawGrid(QPainter& painter) {
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

void GraphicsChart::drawDataLine(QPainter& painter) {
    int margin = 60;
    int w = width() - margin * 2;
    int h = height() - margin * 2;

    // Find global min/max for scaling
    double minValue = 0.0;
    double maxValue = 1.0;
    int64_t minOffset = 0;
    int64_t maxOffset = 1;

    for (const GraphicsParameter& param : m_parameters) {
        if (!param.visible || param.values.isEmpty()) continue;

        for (double val : param.values) {
            if (val < minValue) minValue = val;
            if (val > maxValue) maxValue = val;
        }

        if (!param.offsets.isEmpty()) {
            if (param.offsets.first() < minOffset) minOffset = param.offsets.first();
            if (param.offsets.last() > maxOffset) maxOffset = param.offsets.last();
        }
    }

    // Calculate zoomed dimensions
    int zoomedW = (int)(w * m_zoomLevel);
    int zoomedH = (int)(h * m_zoomLevel);

    // Draw each parameter
    for (const GraphicsParameter& param : m_parameters) {
        if (!param.visible || param.values.size() < 2) continue;

        painter.setPen(QPen(param.color, 2));
        QPainterPath path;

        for (int i = 0; i < param.values.size(); ++i) {
            double xRatio;
            if (m_rangingMode == RangingMode::Offset && !param.offsets.isEmpty()) {
                xRatio = (maxOffset > minOffset) ?
                    (double)(param.offsets[i] - minOffset) / (maxOffset - minOffset) : 0.5;
            } else {
                xRatio = (double)i / (param.values.size() - 1);
            }

            double yRatio = (maxValue > minValue) ?
                (param.values[i] - minValue) / (maxValue - minValue) : 0.5;

            // Apply zoom: scale coordinates from top-left
            int x = margin + (int)(xRatio * zoomedW) + (int)m_offsetX;
            int y = height() - margin - (int)(yRatio * zoomedH) - (int)m_offsetY;

            if (i == 0) {
                path.moveTo(x, y);
            } else {
                path.lineTo(x, y);
            }
        }

        painter.drawPath(path);

        // Draw points
        painter.setPen(Qt::NoPen);
        painter.setBrush(param.color);

        for (int i = 0; i < param.values.size(); ++i) {
            double xRatio;
            if (m_rangingMode == RangingMode::Offset && !param.offsets.isEmpty()) {
                xRatio = (maxOffset > minOffset) ?
                    (double)(param.offsets[i] - minOffset) / (maxOffset - minOffset) : 0.5;
            } else {
                xRatio = (double)i / (param.values.size() - 1);
            }

            double yRatio = (maxValue > minValue) ?
                (param.values[i] - minValue) / (maxValue - minValue) : 0.5;

            // Apply zoom: scale coordinates from top-left
            int x = margin + (int)(xRatio * zoomedW) + (int)m_offsetX;
            int y = height() - margin - (int)(yRatio * zoomedH) - (int)m_offsetY;

            painter.drawEllipse(QPoint(x, y), 3, 3);
        }
    }
}

void GraphicsChart::drawDataBar(QPainter& painter) {
    int margin = 60;
    int w = width() - margin * 2;
    int h = height() - margin * 2;

    // Find global min/max for scaling
    double minValue = 0.0;
    double maxValue = 1.0;

    for (const GraphicsParameter& param : m_parameters) {
        if (!param.visible || param.values.isEmpty()) continue;

        for (double val : param.values) {
            if (val < minValue) minValue = val;
            if (val > maxValue) maxValue = val;
        }
    }

    // Calculate bar width
    int totalBars = 0;
    for (const GraphicsParameter& param : m_parameters) {
        if (param.visible && !param.values.isEmpty()) {
            totalBars += param.values.size();
        }
    }

    if (totalBars == 0) return;

    int barWidth = qMax(2, w / totalBars);
    int currentX = margin;

    // Draw bars for each parameter
    for (const GraphicsParameter& param : m_parameters) {
        if (!param.visible || param.values.isEmpty()) continue;

        painter.setPen(Qt::NoPen);
        painter.setBrush(param.color);

        for (double val : param.values) {
            double yRatio = (maxValue > minValue) ?
                (val - minValue) / (maxValue - minValue) : 0.5;

            int barHeight = (int)(yRatio * h);
            int y = height() - margin - barHeight;

            painter.drawRect(currentX, y, barWidth - 1, barHeight);
            currentX += barWidth;
        }
    }
}

void GraphicsChart::drawDataTransition(QPainter& painter) {
    // Transition mode shows state changes
    int margin = 60;
    int w = width() - margin * 2;
    int h = height() - margin * 2;

    for (const GraphicsParameter& param : m_parameters) {
        if (!param.visible || param.values.size() < 2) continue;

        painter.setPen(QPen(param.color, 3));

        for (int i = 0; i < param.values.size() - 1; ++i) {
            double xRatio1 = (double)i / (param.values.size() - 1);
            double xRatio2 = (double)(i + 1) / (param.values.size() - 1);

            int x1 = margin + (int)(xRatio1 * w);
            int x2 = margin + (int)(xRatio2 * w);
            int y = height() - margin - (int)((param.values[i] / 255.0) * h);

            // Draw horizontal line
            painter.drawLine(x1, y, x2, y);

            // Draw vertical transition
            if (param.values[i] != param.values[i + 1]) {
                int y2 = height() - margin - (int)((param.values[i + 1] / 255.0) * h);
                painter.drawLine(x2, y, x2, y2);
            }
        }
    }
}

void GraphicsChart::drawLegend(QPainter& painter) {
    int x = width() - 200;
    int y = 20;

    painter.setFont(QFont("Arial", 10));

    for (const GraphicsParameter& param : m_parameters) {
        if (!param.visible) continue;

        // Draw color box
        painter.setPen(Qt::NoPen);
        painter.setBrush(param.color);
        painter.drawRect(x, y, 15, 15);

        // Draw parameter name
        painter.setPen(Qt::white);
        painter.drawText(x + 20, y + 12, param.displayName);

        y += 20;
    }
}

void GraphicsChart::drawCursor(QPainter& painter) {
    painter.setPen(QPen(Qt::yellow, 1, Qt::DashLine));
    painter.drawLine(m_cursorPos.x(), 0, m_cursorPos.x(), height());
    painter.drawLine(0, m_cursorPos.y(), width(), m_cursorPos.y());
}

void GraphicsChart::mouseMoveEvent(QMouseEvent* event) {
    m_cursorPos = event->pos();
    m_hasCursor = true;
    update();
}

void GraphicsChart::mouseDoubleClickEvent(QMouseEvent* event) {
    int nearestIndex = findNearestPoint(event->pos());
    if (nearestIndex >= 0) {
        // Find which parameter and emit offset
        for (const GraphicsParameter& param : m_parameters) {
            if (param.visible && nearestIndex < param.offsets.size()) {
                emit pointClicked(param.offsets[nearestIndex]);
                break;
            }
        }
    }
}

void GraphicsChart::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0) {
        zoomIn();
    } else {
        zoomOut();
    }
}

int GraphicsChart::findNearestPoint(const QPoint& pos) const {
    // Simplified nearest point finding
    return -1;
}

// GraphicsPanel implementation

GraphicsPanel::GraphicsPanel(QWidget* parent)
    : QWidget(parent)
    , m_parser(nullptr)
    , m_colorIndex(0)
{
    // Initialize color palette
    m_colors << QColor(100, 200, 255)  // Blue
             << QColor(255, 100, 100)  // Red
             << QColor(100, 255, 100)  // Green
             << QColor(255, 200, 100)  // Orange
             << QColor(200, 100, 255)  // Purple
             << QColor(255, 255, 100)  // Yellow
             << QColor(100, 255, 255)  // Cyan
             << QColor(255, 100, 255); // Magenta

    createUI();
}

GraphicsPanel::~GraphicsPanel() {
}

void GraphicsPanel::createUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Control bar
    QHBoxLayout* controlLayout = new QHBoxLayout();

    QLabel* modeLabel = new QLabel("Mode:", this);
    controlLayout->addWidget(modeLabel);

    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem("Line");
    m_modeCombo->addItem("Bar");
    m_modeCombo->addItem("Transition");
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GraphicsPanel::onModeChanged);
    controlLayout->addWidget(m_modeCombo);

    QLabel* rangingLabel = new QLabel("Ranging:", this);
    controlLayout->addWidget(rangingLabel);

    m_rangingCombo = new QComboBox(this);
    m_rangingCombo->addItem("Offset");
    m_rangingCombo->addItem("Count");
    connect(m_rangingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GraphicsPanel::onRangingModeChanged);
    controlLayout->addWidget(m_rangingCombo);

    controlLayout->addStretch();

    // Remove parameter button
    m_removeButton = new QPushButton("Remove", this);
    connect(m_removeButton, &QPushButton::clicked, this, &GraphicsPanel::onRemoveParameter);
    controlLayout->addWidget(m_removeButton);

    // Export button
    m_exportButton = new QPushButton("Export", this);
    connect(m_exportButton, &QPushButton::clicked, this, &GraphicsPanel::onExport);
    controlLayout->addWidget(m_exportButton);

    // Zoom controls
    m_fitButton = new QPushButton("Fit", this);
    connect(m_fitButton, &QPushButton::clicked, this, &GraphicsPanel::onZoomFit);
    controlLayout->addWidget(m_fitButton);

    m_zoomInButton = new QPushButton("+", this);
    m_zoomInButton->setMaximumWidth(30);
    connect(m_zoomInButton, &QPushButton::clicked, this, &GraphicsPanel::onZoomIn);
    controlLayout->addWidget(m_zoomInButton);

    m_zoomOutButton = new QPushButton("-", this);
    m_zoomOutButton->setMaximumWidth(30);
    connect(m_zoomOutButton, &QPushButton::clicked, this, &GraphicsPanel::onZoomOut);
    controlLayout->addWidget(m_zoomOutButton);

    mainLayout->addLayout(controlLayout);

    // Info label
    m_infoLabel = new QLabel("No parameters added", this);
    m_infoLabel->setStyleSheet("color: #888; font-style: italic;");
    mainLayout->addWidget(m_infoLabel);

    // Chart
    m_chart = new GraphicsChart(this);
    connect(m_chart, &GraphicsChart::pointClicked,
            this, &GraphicsPanel::onChartPointClicked);
    mainLayout->addWidget(m_chart);
}

void GraphicsPanel::setTSParser(TSParser* parser) {
    m_parser = parser;
}

void GraphicsPanel::clear() {
    m_parser = nullptr;
    m_chart->clearParameters();
    m_infoLabel->setText("No parameters added");
}

void GraphicsPanel::addParameter(const QString& name, const QVector<double>& values, const QVector<int64_t>& offsets) {
    GraphicsParameter param;
    param.name = name;
    param.displayName = name;
    param.color = getNextColor();
    param.values = values;
    param.offsets = offsets;
    param.visible = true;

    m_chart->addParameter(param);

    // Update parameter list display
    updateParameterList();
}

QColor GraphicsPanel::getNextColor() {
    QColor color = m_colors[m_colorIndex % m_colors.size()];
    m_colorIndex++;
    return color;
}

void GraphicsPanel::onModeChanged(int index) {
    m_chart->setMode(static_cast<GraphicsMode>(index));
}

void GraphicsPanel::onRangingModeChanged(int index) {
    m_chart->setRangingMode(static_cast<RangingMode>(index));
}

void GraphicsPanel::onZoomIn() {
    qDebug() << "GraphicsPanel::onZoomIn() called";
    m_chart->zoomIn();
}

void GraphicsPanel::onZoomOut() {
    qDebug() << "GraphicsPanel::onZoomOut() called";
    m_chart->zoomOut();
}

void GraphicsPanel::onZoomFit() {
    qDebug() << "GraphicsPanel::onZoomFit() called";
    m_chart->zoomFit();
}

void GraphicsPanel::onChartPointClicked(int64_t offset) {
    emit offsetClicked(offset);
    qDebug() << "Graphics chart point clicked, offset:" << offset;
}

void GraphicsPanel::onRemoveParameter() {
    const QVector<GraphicsParameter>& parameters = m_chart->getParameters();

    if (parameters.isEmpty()) {
        QMessageBox::information(this, tr("Remove Parameter"),
            tr("No parameters to remove."));
        return;
    }

    // Create selection dialog
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Remove Parameter"));
    dialog.setMinimumWidth(300);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QLabel* label = new QLabel(tr("Select parameter to remove:"), &dialog);
    layout->addWidget(label);

    QListWidget* listWidget = new QListWidget(&dialog);
    for (const GraphicsParameter& param : parameters) {
        QListWidgetItem* item = new QListWidgetItem(param.displayName);
        item->setData(Qt::UserRole, param.name);
        item->setForeground(QBrush(param.color));
        listWidget->addItem(item);
    }
    layout->addWidget(listWidget);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* removeButton = new QPushButton(tr("Remove"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &dialog);
    buttonLayout->addStretch();
    buttonLayout->addWidget(removeButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(removeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(listWidget, &QListWidget::itemDoubleClicked, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted) {
        QListWidgetItem* currentItem = listWidget->currentItem();
        if (currentItem) {
            QString paramName = currentItem->data(Qt::UserRole).toString();
            m_chart->removeParameter(paramName);
            updateParameterList();
        }
    }
}

void GraphicsPanel::onExport() {
    const QVector<GraphicsParameter>& parameters = m_chart->getParameters();

    if (parameters.isEmpty()) {
        QMessageBox::information(this, tr("Export Data"),
            tr("No parameters to export."));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Graphics Data"), QString(),
        tr("CSV Files (*.csv);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export Failed"),
            tr("Failed to open file for writing:\n%1").arg(fileName));
        return;
    }

    QTextStream out(&file);

    // Write header
    out << "Index,Offset";
    for (const GraphicsParameter& param : parameters) {
        out << "," << param.displayName;
    }
    out << "\n";

    // Find maximum data length
    int maxLength = 0;
    for (const GraphicsParameter& param : parameters) {
        if (param.values.size() > maxLength) {
            maxLength = param.values.size();
        }
    }

    // Write data rows
    for (int i = 0; i < maxLength; ++i) {
        out << i;

        // Write offset (use first parameter's offset if available)
        if (!parameters.isEmpty() && i < parameters[0].offsets.size()) {
            out << "," << parameters[0].offsets[i];
        } else {
            out << ",";
        }

        // Write values for each parameter
        for (const GraphicsParameter& param : parameters) {
            if (i < param.values.size()) {
                out << "," << param.values[i];
            } else {
                out << ",";
            }
        }
        out << "\n";
    }

    file.close();

    QMessageBox::information(this, tr("Export Complete"),
        tr("Exported %1 data points to:\n%2").arg(maxLength).arg(fileName));
}

void GraphicsPanel::updateParameterList() {
    const QVector<GraphicsParameter>& parameters = m_chart->getParameters();

    if (parameters.isEmpty()) {
        m_infoLabel->setText("No parameters added");
    } else if (parameters.size() == 1) {
        m_infoLabel->setText(QString("1 parameter: %1").arg(parameters[0].displayName));
    } else {
        QStringList names;
        for (const GraphicsParameter& param : parameters) {
            names.append(param.displayName);
        }
        m_infoLabel->setText(QString("%1 parameters: %2")
            .arg(parameters.size())
            .arg(names.join(", ")));
    }
}

} // namespace VideoStudio
