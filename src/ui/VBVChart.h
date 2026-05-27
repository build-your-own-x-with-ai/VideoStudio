#ifndef VBVCHART_H
#define VBVCHART_H

#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "core/FrameInfo.h"
#include "core/VBVAnalyzer.h"

class VBVChart : public QWidget {
    Q_OBJECT

public:
    explicit VBVChart(QWidget* parent = nullptr);

    void setFrameData(const QVector<FrameInfo>& frames);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private slots:
    void onAnalyzeClicked();

private:
    void drawOccupancyChart(QPainter& painter);
    void drawStatistics(QPainter& painter);
    void drawTooltip(QPainter& painter);
    void setupControls();

    VBVAnalyzer* analyzer;
    QVector<FrameInfo> frameData;
    bool hasData;

    // Control widgets
    QLabel* bufferSizeLabel;
    QLineEdit* bufferSizeEdit;
    QLabel* bitrateLabel;
    QLineEdit* bitrateEdit;
    QLabel* delayLabel;
    QLineEdit* delayEdit;
    QPushButton* analyzeButton;

    // Layout parameters
    int leftMargin;
    int rightMargin;
    int topMargin;
    int bottomMargin;
    int chartHeight;
    int controlPanelHeight;

    // Mouse tracking
    QPoint mousePos;
    bool mouseInWidget;
};

#endif // VBVCHART_H
