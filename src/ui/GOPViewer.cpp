#include "GOPViewer.h"
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPainter>

GOPViewer::GOPViewer(QWidget* parent)
    : QWidget(parent) {
    setupUI();
}

void GOPViewer::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 统计信息
    QGroupBox* statsGroup = new QGroupBox("GOP 统计", this);
    QHBoxLayout* statsLayout = new QHBoxLayout(statsGroup);

    totalGOPsLabel = new QLabel("总 GOP 数: 0");
    avgGOPSizeLabel = new QLabel("平均大小: 0 帧");
    maxGOPSizeLabel = new QLabel("最大: 0 帧");
    minGOPSizeLabel = new QLabel("最小: 0 帧");
    avgKeyFrameIntervalLabel = new QLabel("关键帧间隔: 0 帧");

    statsLayout->addWidget(totalGOPsLabel);
    statsLayout->addWidget(avgGOPSizeLabel);
    statsLayout->addWidget(maxGOPSizeLabel);
    statsLayout->addWidget(minGOPSizeLabel);
    statsLayout->addWidget(avgKeyFrameIntervalLabel);
    statsLayout->addStretch();

    mainLayout->addWidget(statsGroup);

    // GOP 可视化区域
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    gopContainer = new QWidget();
    gopLayout = new QVBoxLayout(gopContainer);
    gopLayout->setSpacing(10);
    gopLayout->setContentsMargins(10, 10, 10, 10);

    scrollArea->setWidget(gopContainer);
    mainLayout->addWidget(scrollArea, 1);
}

void GOPViewer::setGOPData(const QVector<GOP>& gops, const GOPStats& stats) {
    this->gops = gops;
    this->stats = stats;
    updateGOPView();
    updateStats();
}

void GOPViewer::updateGOPView() {
    // 清除旧的 GOP 组件
    QLayoutItem* item;
    while ((item = gopLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    // 创建新的 GOP 组件
    for (int i = 0; i < gops.size(); ++i) {
        QWidget* gopWidget = createGOPWidget(gops[i], i);
        gopLayout->addWidget(gopWidget);
    }

    gopLayout->addStretch();
}

QWidget* GOPViewer::createGOPWidget(const GOP& gop, int gopIndex) {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    // GOP 标题
    QLabel* titleLabel = new QLabel(QString("GOP #%1 (帧 %2-%3, 共 %4 帧)")
        .arg(gopIndex + 1)
        .arg(gop.startFrame)
        .arg(gop.endFrame)
        .arg(gop.size));
    titleLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(titleLabel);

    // 帧序列可视化
    QWidget* framesWidget = new QWidget();
    QHBoxLayout* framesLayout = new QHBoxLayout(framesWidget);
    framesLayout->setSpacing(2);
    framesLayout->setContentsMargins(0, 0, 0, 0);

    for (const auto& frame : gop.frames) {
        QLabel* frameLabel = new QLabel(QString(frame.frameType));
        frameLabel->setFixedSize(30, 30);
        frameLabel->setAlignment(Qt::AlignCenter);

        QColor color = getFrameColor(frame.frameType);
        frameLabel->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold; border: 1px solid #555;")
            .arg(color.name()));

        framesLayout->addWidget(frameLabel);
    }

    framesLayout->addStretch();
    layout->addWidget(framesWidget);

    widget->setStyleSheet("QWidget { background-color: #2b2b2b; border: 1px solid #555; border-radius: 5px; }");
    return widget;
}

QColor GOPViewer::getFrameColor(char frameType) {
    switch (frameType) {
        case 'I': return QColor(255, 100, 100);  // 红色
        case 'P': return QColor(100, 255, 100);  // 绿色
        case 'B': return QColor(100, 100, 255);  // 蓝色
        default:  return QColor(128, 128, 128);  // 灰色
    }
}

void GOPViewer::updateStats() {
    totalGOPsLabel->setText(QString("总 GOP 数: %1").arg(stats.totalGOPs));
    avgGOPSizeLabel->setText(QString("平均大小: %1 帧").arg(stats.averageGOPSize, 0, 'f', 1));
    maxGOPSizeLabel->setText(QString("最大: %1 帧").arg(stats.maxGOPSize));
    minGOPSizeLabel->setText(QString("最小: %1 帧").arg(stats.minGOPSize));
    avgKeyFrameIntervalLabel->setText(QString("关键帧间隔: %1 帧").arg(stats.averageKeyFrameInterval, 0, 'f', 1));
}

void GOPViewer::clear() {
    QLayoutItem* item;
    while ((item = gopLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    gops.clear();
    stats = {0, 0.0, 0, 0, 0.0, 0, 0, 0};

    totalGOPsLabel->setText("总 GOP 数: 0");
    avgGOPSizeLabel->setText("平均大小: 0 帧");
    maxGOPSizeLabel->setText("最大: 0 帧");
    minGOPSizeLabel->setText("最小: 0 帧");
    avgKeyFrameIntervalLabel->setText("关键帧间隔: 0 帧");
}
