#ifndef BITRATEPANEL_H
#define BITRATEPANEL_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVector>
#include <QMap>
#include <QPainter>
#include <QMouseEvent>

namespace VideoStudio {

class TSParser;

struct BitratePoint {
    int64_t offset;      // Packet offset
    double bitrate;      // Bitrate in bps
    int packetIndex;     // For navigation
};

class BitrateChart : public QWidget {
    Q_OBJECT

public:
    explicit BitrateChart(QWidget* parent = nullptr);
    ~BitrateChart();

    void setData(const QVector<BitratePoint>& data);
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
    void drawStatistics(QPainter& painter);
    int findNearestPoint(const QPoint& pos) const;
    QString formatBitrate(double bps) const;

    QVector<BitratePoint> m_data;
    double m_zoomLevel;
    double m_offsetX;
    double m_offsetY;
    QPoint m_cursorPos;
    int m_hoveredPointIndex;
    bool m_hasCursor;

    // Statistics
    double m_minBitrate;
    double m_maxBitrate;
    double m_avgBitrate;
};

class BitratePanel : public QWidget {
    Q_OBJECT

public:
    explicit BitratePanel(QWidget* parent = nullptr);
    ~BitratePanel();

    void setTSParser(TSParser* parser);
    void clear();

signals:
    void packetClicked(int packetIndex);

private slots:
    void onModeChanged(int index);
    void onPIDChanged(int index);
    void onWindowSizeChanged(int index);
    void onZoomIn();
    void onZoomOut();
    void onZoomFit();
    void onChartPointClicked(int packetIndex);
    void onShowAllPIDsToggled(bool checked);

private:
    void createUI();
    void updatePIDList();
    void updateChart();
    void analyzeOverallBitrate();
    void analyzePIDBitrate(uint16_t pid);
    void analyzeAllPIDsBitrate();
    QVector<BitratePoint> calculateBitrate(const QVector<int>& packetIndices, int windowSize);

    TSParser* m_parser;
    uint16_t m_currentPID;
    int m_windowSize; // in packets
    bool m_showAllPIDs;

    // UI components
    QComboBox* m_modeCombo;
    QComboBox* m_pidCombo;
    QComboBox* m_windowCombo;
    QCheckBox* m_showAllPIDsCheckbox;
    QPushButton* m_zoomInButton;
    QPushButton* m_zoomOutButton;
    QPushButton* m_fitButton;
    QLabel* m_infoLabel;
    BitrateChart* m_chart;
};

} // namespace VideoStudio

#endif // BITRATEPANEL_H
