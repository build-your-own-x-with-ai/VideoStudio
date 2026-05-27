#ifndef QUALITYHEATMAPOVERLAY_H
#define QUALITYHEATMAPOVERLAY_H

#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QImage>

class QualityHeatmapOverlay : public QWidget {
    Q_OBJECT

public:
    enum HeatmapMode {
        None,
        PSNR,
        SSIM,
        Temperature,
        Subtraction
    };

    explicit QualityHeatmapOverlay(QWidget* parent = nullptr);

    void setHeatmapMode(HeatmapMode mode);
    void setHeatmapData(const QVector<double>& data, int rows, int cols);
    void setHeatmapImage(const QImage& image);
    void clear();

    HeatmapMode getHeatmapMode() const { return mode; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawBlockHeatmap(QPainter& painter);
    void drawImageHeatmap(QPainter& painter);
    QColor valueToColor(double value, HeatmapMode mode);

    HeatmapMode mode;
    QVector<double> heatmapData;
    int blockRows;
    int blockCols;
    QImage heatmapImage;
};

#endif // QUALITYHEATMAPOVERLAY_H
