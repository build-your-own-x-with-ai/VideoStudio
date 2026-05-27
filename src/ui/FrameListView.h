#ifndef FRAMELISTVIEW_H
#define FRAMELISTVIEW_H

#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QLabel>
#include "core/MetricsCollector.h"

class FrameListView : public QWidget {
    Q_OBJECT

public:
    explicit FrameListView(QWidget* parent = nullptr);
    void setMetricsCollector(MetricsCollector* collector);
    void updateFrameList();
    void clear();

signals:
    void frameSelected(int frameIndex);

private slots:
    void onFrameClicked(const QModelIndex& index);
    void onFilterChanged();

private:
    void setupUI();
    void updateStatistics();
    QString formatSize(int size);
    QString formatTimestamp(double timestamp);

    QTableView* tableView;
    QStandardItemModel* model;
    MetricsCollector* metricsCollector;

    QLabel* totalFramesLabel;
    QLabel* iFramesLabel;
    QLabel* pFramesLabel;
    QLabel* bFramesLabel;

    QPushButton* showAllButton;
    QPushButton* showIFramesButton;
    QPushButton* showPFramesButton;
    QPushButton* showBFramesButton;

    char currentFilter;
};

#endif // FRAMELISTVIEW_H
