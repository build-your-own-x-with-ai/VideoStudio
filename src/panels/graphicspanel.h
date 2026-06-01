#ifndef GRAPHICSPANEL_H
#define GRAPHICSPANEL_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVector>
#include <QMap>
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>

namespace VideoStudio {

class TSParser;

struct GraphicsParameter {
    QString name;           // Parameter name (e.g., "PID", "Continuity Counter")
    QString displayName;    // Display name for legend
    QColor color;           // Line/bar color
    QVector<double> values; // Parameter values
    QVector<int64_t> offsets; // Corresponding offsets
    bool visible;           // Show/hide this parameter
};

enum class GraphicsMode {
    Line,
    Bar,
    Transition
};

enum class RangingMode {
    Offset,
    Count
};

class GraphicsChart : public QWidget {
    Q_OBJECT

public:
    explicit GraphicsChart(QWidget* parent = nullptr);
    ~GraphicsChart();

    void addParameter(const GraphicsParameter& param);
    void removeParameter(const QString& name);
    void clearParameters();
    void setMode(GraphicsMode mode);
    void setRangingMode(RangingMode mode);
    void zoomIn();
    void zoomOut();
    void zoomFit();

signals:
    void pointClicked(int64_t offset);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void drawAxes(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawDataLine(QPainter& painter);
    void drawDataBar(QPainter& painter);
    void drawDataTransition(QPainter& painter);
    void drawLegend(QPainter& painter);
    void drawCursor(QPainter& painter);
    int findNearestPoint(const QPoint& pos) const;

    QVector<GraphicsParameter> m_parameters;
    GraphicsMode m_mode;
    RangingMode m_rangingMode;
    double m_zoomLevel;
    double m_offsetX;
    double m_offsetY;
    QPoint m_cursorPos;
    int m_hoveredPointIndex;
    bool m_hasCursor;
};

class GraphicsPanel : public QWidget {
    Q_OBJECT

public:
    explicit GraphicsPanel(QWidget* parent = nullptr);
    ~GraphicsPanel();

    void setTSParser(TSParser* parser);
    void clear();

    // Add parameter from Property Panel
    void addParameter(const QString& name, const QVector<double>& values, const QVector<int64_t>& offsets);

signals:
    void offsetClicked(int64_t offset);

private slots:
    void onModeChanged(int index);
    void onRangingModeChanged(int index);
    void onZoomIn();
    void onZoomOut();
    void onZoomFit();
    void onChartPointClicked(int64_t offset);
    void onRemoveParameter();
    void onExport();

private:
    void createUI();
    void updateParameterList();
    QColor getNextColor();

    TSParser* m_parser;
    int m_colorIndex;

    // UI components
    QComboBox* m_modeCombo;
    QComboBox* m_rangingCombo;
    QPushButton* m_zoomInButton;
    QPushButton* m_zoomOutButton;
    QPushButton* m_fitButton;
    QPushButton* m_removeButton;
    QPushButton* m_exportButton;
    QLabel* m_infoLabel;
    GraphicsChart* m_chart;

    // Predefined colors for parameters
    QVector<QColor> m_colors;
};

} // namespace VideoStudio

#endif // GRAPHICSPANEL_H
