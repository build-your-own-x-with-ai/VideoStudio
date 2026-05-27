#include "FrameListView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QGroupBox>

FrameListView::FrameListView(QWidget* parent)
    : QWidget(parent), metricsCollector(nullptr), currentFilter('\0') {
    setupUI();
}

void FrameListView::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QGroupBox* statsGroup = new QGroupBox("统计信息", this);
    QHBoxLayout* statsLayout = new QHBoxLayout(statsGroup);

    totalFramesLabel = new QLabel("总帧数: 0");
    iFramesLabel = new QLabel("I 帧: 0");
    pFramesLabel = new QLabel("P 帧: 0");
    bFramesLabel = new QLabel("B 帧: 0");

    statsLayout->addWidget(totalFramesLabel);
    statsLayout->addWidget(iFramesLabel);
    statsLayout->addWidget(pFramesLabel);
    statsLayout->addWidget(bFramesLabel);
    statsLayout->addStretch();

    mainLayout->addWidget(statsGroup);

    QGroupBox* filterGroup = new QGroupBox("筛选", this);
    QHBoxLayout* filterLayout = new QHBoxLayout(filterGroup);

    showAllButton = new QPushButton("显示全部");
    showIFramesButton = new QPushButton("仅 I 帧");
    showPFramesButton = new QPushButton("仅 P 帧");
    showBFramesButton = new QPushButton("仅 B 帧");

    showAllButton->setCheckable(true);
    showIFramesButton->setCheckable(true);
    showPFramesButton->setCheckable(true);
    showBFramesButton->setCheckable(true);
    showAllButton->setChecked(true);

    connect(showAllButton, &QPushButton::clicked, this, &FrameListView::onFilterChanged);
    connect(showIFramesButton, &QPushButton::clicked, this, &FrameListView::onFilterChanged);
    connect(showPFramesButton, &QPushButton::clicked, this, &FrameListView::onFilterChanged);
    connect(showBFramesButton, &QPushButton::clicked, this, &FrameListView::onFilterChanged);

    filterLayout->addWidget(showAllButton);
    filterLayout->addWidget(showIFramesButton);
    filterLayout->addWidget(showPFramesButton);
    filterLayout->addWidget(showBFramesButton);
    filterLayout->addStretch();

    mainLayout->addWidget(filterGroup);

    tableView = new QTableView(this);
    model = new QStandardItemModel(this);

    model->setHorizontalHeaderLabels({"帧号", "类型", "大小", "时间戳", "PTS", "DTS", "关键帧"});

    tableView->setModel(model);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView->setSortingEnabled(true);
    tableView->horizontalHeader()->setStretchLastSection(true);

    connect(tableView, &QTableView::clicked, this, &FrameListView::onFrameClicked);

    mainLayout->addWidget(tableView, 1);
}

void FrameListView::setMetricsCollector(MetricsCollector* collector) {
    metricsCollector = collector;
    if (metricsCollector) {
        connect(metricsCollector, &MetricsCollector::metricsUpdated,
                this, &FrameListView::updateFrameList);
    }
}

void FrameListView::updateFrameList() {
    if (!metricsCollector) {
        return;
    }

    model->removeRows(0, model->rowCount());

    const QVector<FrameInfo>& frames = metricsCollector->getAllFrames();

    for (const auto& frame : frames) {
        if (currentFilter != '\0' && frame.frameType != currentFilter) {
            continue;
        }

        QList<QStandardItem*> row;

        row.append(new QStandardItem(QString::number(frame.frameNumber)));
        row.append(new QStandardItem(QString(frame.frameType)));
        row.append(new QStandardItem(formatSize(frame.size)));
        row.append(new QStandardItem(formatTimestamp(frame.timestamp)));
        row.append(new QStandardItem(QString::number(frame.pts)));
        row.append(new QStandardItem(QString::number(frame.dts)));
        row.append(new QStandardItem(frame.isKeyFrame ? "是" : "否"));

        model->appendRow(row);
    }

    updateStatistics();
}

void FrameListView::clear() {
    model->removeRows(0, model->rowCount());
    totalFramesLabel->setText("总帧数: 0");
    iFramesLabel->setText("I 帧: 0");
    pFramesLabel->setText("P 帧: 0");
    bFramesLabel->setText("B 帧: 0");
    currentFilter = '\0';
    showAllButton->setChecked(true);
}

void FrameListView::onFrameClicked(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }

    int frameNumber = model->item(index.row(), 0)->text().toInt();
    emit frameSelected(frameNumber);
}

void FrameListView::onFilterChanged() {
    QPushButton* sender = qobject_cast<QPushButton*>(QObject::sender());

    showAllButton->setChecked(false);
    showIFramesButton->setChecked(false);
    showPFramesButton->setChecked(false);
    showBFramesButton->setChecked(false);

    if (sender) {
        sender->setChecked(true);
    }

    if (sender == showAllButton) {
        currentFilter = '\0';
    } else if (sender == showIFramesButton) {
        currentFilter = 'I';
    } else if (sender == showPFramesButton) {
        currentFilter = 'P';
    } else if (sender == showBFramesButton) {
        currentFilter = 'B';
    }

    updateFrameList();
}

void FrameListView::updateStatistics() {
    if (!metricsCollector) {
        return;
    }

    totalFramesLabel->setText(QString("总帧数: %1").arg(metricsCollector->getFrameCount()));
    iFramesLabel->setText(QString("I 帧: %1").arg(metricsCollector->getIFrameCount()));
    pFramesLabel->setText(QString("P 帧: %1").arg(metricsCollector->getPFrameCount()));
    bFramesLabel->setText(QString("B 帧: %1").arg(metricsCollector->getBFrameCount()));
}

QString FrameListView::formatSize(int size) {
    if (size < 1024) {
        return QString("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        return QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
    } else {
        return QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 2);
    }
}

QString FrameListView::formatTimestamp(double timestamp) {
    int totalSeconds = static_cast<int>(timestamp);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    int milliseconds = static_cast<int>((timestamp - totalSeconds) * 1000);

    if (hours > 0) {
        return QString("%1:%2:%3.%4")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(milliseconds, 3, 10, QChar('0'));
    } else {
        return QString("%1:%2.%3")
            .arg(minutes)
            .arg(seconds, 2, 10, QChar('0'))
            .arg(milliseconds, 3, 10, QChar('0'));
    }
}
