#ifndef TIMEDYNAMICSPANEL_H
#define TIMEDYNAMICSPANEL_H

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

namespace VideoStudio {

class TSParser;
class TimingAnalyzer;

enum class TimeDynamicsMode {
    PTSDTSDynamics,      // PTS/DTS variation with offset
    PCRDynamics,         // PCR variation with offset
    PCRPTSDynamics,      // PTS/DTS in relation to PCR
    OffsetPCRDynamics,   // Offset variation between PCR values
    PCRAccuracy          // PCR accuracy (±500ns)
};

struct TimePoint {
    int64_t offset;      // Packet offset
    int64_t timestamp;   // PTS/DTS/PCR value
    int packetIndex;     // For navigation
};

class TimeDynamicsChart : public QWidget {
    Q_OBJECT

public:
    explicit TimeDynamicsChart(QWidget* parent = nullptr);
    ~TimeDynamicsChart();

    void setData(const QVector<TimePoint>& data);
    void clear();
    void zoomIn();
    void zoomOut();
    void zoomFit();

signals:
    void pointClicked(int packetIndex);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void drawAxes(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawData(QPainter& painter);
    void drawCursor(QPainter& painter);
    QPointF dataToScreen(const TimePoint& point) const;
    int findNearestPoint(const QPoint& pos) const;

    QVector<TimePoint> m_data;
    double m_zoomLevel;
    double m_offsetX;
    double m_offsetY;
    QPoint m_cursorPos;
    int m_hoveredPointIndex;
    bool m_hasCursor;
};

class TimeDynamicsPanel : public QWidget {
    Q_OBJECT

public:
    explicit TimeDynamicsPanel(QWidget* parent = nullptr);
    ~TimeDynamicsPanel();

    void setTSParser(TSParser* parser);
    void clear();

signals:
    void packetClicked(int packetIndex);

private slots:
    void onModeChanged(int index);
    void onPIDChanged(int index);
    void onZoomIn();
    void onZoomOut();
    void onZoomFit();
    void onChartPointClicked(int packetIndex);

private:
    void createUI();
    void updatePIDList();
    void updateChart();

    TSParser* m_parser;
    TimingAnalyzer* m_analyzer;
    TimeDynamicsMode m_currentMode;
    uint16_t m_currentPID;

    // UI components
    QComboBox* m_modeCombo;
    QComboBox* m_pidCombo;
    QPushButton* m_zoomInButton;
    QPushButton* m_zoomOutButton;
    QPushButton* m_fitButton;
    QLabel* m_infoLabel;
    TimeDynamicsChart* m_chart;
};

} // namespace VideoStudio

#endif // TIMEDYNAMICSPANEL_H
