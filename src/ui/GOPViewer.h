#ifndef GOPVIEWER_H
#define GOPVIEWER_H

#include <QWidget>
#include <QScrollArea>
#include <QLabel>
#include <QVBoxLayout>
#include "core/GOPAnalyzer.h"

class GOPViewer : public QWidget {
    Q_OBJECT

public:
    explicit GOPViewer(QWidget* parent = nullptr);
    void setGOPData(const QVector<GOP>& gops, const GOPStats& stats);
    void clear();

private:
    void setupUI();
    void updateGOPView();
    void updateStats();
    QWidget* createGOPWidget(const GOP& gop, int gopIndex);
    QColor getFrameColor(char frameType);

    QScrollArea* scrollArea;
    QWidget* gopContainer;
    QVBoxLayout* gopLayout;

    QLabel* totalGOPsLabel;
    QLabel* avgGOPSizeLabel;
    QLabel* maxGOPSizeLabel;
    QLabel* minGOPSizeLabel;
    QLabel* avgKeyFrameIntervalLabel;

    QVector<GOP> gops;
    GOPStats stats;
};

#endif // GOPVIEWER_H
