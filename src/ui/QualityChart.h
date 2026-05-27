#ifndef QUALITYCHART_H
#define QUALITYCHART_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include "core/QualityAnalyzer.h"

class QualityChart : public QWidget {
    Q_OBJECT

public:
    explicit QualityChart(QWidget* parent = nullptr);

    void setQualityData(const QVector<QualityMetrics>& data);
    void setStats(const QualityStats& stats);
    void clear();

signals:
    void referenceVideoSelected(const QString& filePath);
    void analyzeRequested();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onSelectReferenceVideo();
    void onAnalyze();

private:
    void drawChart(QPainter& painter);
    void setupUI();

    QVector<QualityMetrics> qualityData;
    QualityStats stats;

    // UI components
    QPushButton* selectRefButton;
    QPushButton* analyzeButton;
    QLabel* refVideoLabel;
    QLabel* statsLabel;
    QGroupBox* statsBox;

    QString referenceVideoPath;
};

#endif // QUALITYCHART_H
