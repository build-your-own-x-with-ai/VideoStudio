#include "widgets/nalunitview.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QDebug>

namespace VideoStudio {

NALUnitView::NALUnitView(QWidget* parent)
    : QWidget(parent)
    , m_treeWidget(new QTreeWidget(this))
    , m_parser(nullptr)
    , m_currentNALIndex(-1)
{
    // Setup tree widget
    m_treeWidget->setColumnCount(6);
    QStringList headers;
    headers << "#" << "Offset" << "Frame" << "Type" << "Size" << "Properties";
    m_treeWidget->setHeaderLabels(headers);

    // Set column widths
    m_treeWidget->setColumnWidth(0, 60);   // Index
    m_treeWidget->setColumnWidth(1, 100);  // Offset
    m_treeWidget->setColumnWidth(2, 80);   // Frame
    m_treeWidget->setColumnWidth(3, 200);  // Type
    m_treeWidget->setColumnWidth(4, 80);   // Size
    m_treeWidget->setColumnWidth(5, 300);  // Properties

    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setRootIsDecorated(false);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Connect signals
    connect(m_treeWidget, &QTreeWidget::itemClicked,
            this, &NALUnitView::onItemClicked);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &NALUnitView::onItemDoubleClicked);

    // Layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_treeWidget);
}

NALUnitView::~NALUnitView() {
}

void NALUnitView::setNALUnitParser(NALUnitParser* parser) {
    m_parser = parser;
}

void NALUnitView::buildNALUnitList() {
    m_treeWidget->clear();
    m_currentNALIndex = -1;

    if (!m_parser) {
        return;
    }

    const QVector<NALUnitInfo>& nalUnits = m_parser->getNALUnits();
    int totalNALs = nalUnits.size();

    if (totalNALs == 0) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);
        item->setText(0, "No NAL units found");
        item->setForeground(0, QColor(150, 150, 150));
        return;
    }

    // Display limit: 1000 NAL units (same as PacketView)
    int maxNALs = qMin(1000, totalNALs);

    for (int i = 0; i < maxNALs; ++i) {
        const NALUnitInfo& nalInfo = nalUnits[i];

        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);

        // Store NAL index in user data
        item->setData(0, Qt::UserRole, nalInfo.index);

        // Column 0: Index
        item->setText(0, QString::number(nalInfo.index));

        // Column 1: Offset (hex)
        item->setText(1, QString("0x%1").arg(nalInfo.fileOffset, 0, 16));

        // Column 2: Frame number
        item->setText(2, QString::number(nalInfo.frameNumber));

        // Column 3: Type
        item->setText(3, nalInfo.typeName);

        // Column 4: Size
        item->setText(4, QString("%1 B").arg(nalInfo.size));

        // Column 5: Properties
        item->setText(5, formatProperties(nalInfo));

        // Color coding
        QColor color = getNALUnitColor(nalInfo);
        for (int col = 2; col <= 3; ++col) {
            item->setForeground(col, color);
        }
    }

    // Show message if more NAL units exist
    if (totalNALs > maxNALs) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);
        item->setText(0, QString("... (%1 more NAL units)").arg(totalNALs - maxNALs));
        item->setForeground(0, QColor(150, 150, 150));
    }

    qDebug() << "NALUnitView: Built list with" << maxNALs << "NAL units (total:" << totalNALs << ")";
}

void NALUnitView::clear() {
    m_treeWidget->clear();
    m_currentNALIndex = -1;
}

void NALUnitView::setCurrentNALUnit(int nalIndex) {
    if (nalIndex < 0) {
        return;
    }

    m_currentNALIndex = nalIndex;

    // Find and select the item
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
        int itemIndex = item->data(0, Qt::UserRole).toInt();

        if (itemIndex == nalIndex) {
            m_treeWidget->setCurrentItem(item);
            m_treeWidget->scrollToItem(item, QAbstractItemView::PositionAtCenter);

            // Highlight with blue background
            for (int col = 0; col < m_treeWidget->columnCount(); ++col) {
                item->setBackground(col, QColor(100, 150, 255, 100));
            }
            break;
        }
    }
}

void NALUnitView::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);

    if (!item) {
        qDebug() << "NALUnitView::onItemClicked: item is null";
        return;
    }

    int nalIndex = item->data(0, Qt::UserRole).toInt();
    m_currentNALIndex = nalIndex;

    qDebug() << "NALUnitView::onItemClicked: Emitting nalUnitSelected signal with nalIndex:" << nalIndex;
    emit nalUnitSelected(nalIndex);
}

void NALUnitView::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);

    if (!item || !m_parser) {
        return;
    }

    int nalIndex = item->data(0, Qt::UserRole).toInt();
    const NALUnitInfo* nalInfo = m_parser->getNALUnit(nalIndex);

    if (nalInfo) {
        emit nalUnitDoubleClicked(nalIndex);
        emit frameSelected(nalInfo->frameNumber);
    }
}

QString NALUnitView::formatProperties(const NALUnitInfo& info) {
    QStringList props;

    if (info.isIDR) {
        props << "IDR";
    }

    if (info.isKeyFrame) {
        props << "KeyFrame";
    }

    if (info.isSlice && !info.sliceType.isEmpty()) {
        props << QString("SliceType=%1").arg(info.sliceType);
    }

    if (info.sliceQP >= 0) {
        props << QString("QP=%1").arg(info.sliceQP);
    }

    if (info.layerId > 0) {
        props << QString("LayerID=%1").arg(info.layerId);
    }

    if (info.temporalId >= 0) {
        props << QString("TemporalID=%1").arg(info.temporalId);
    }

    return props.join(", ");
}

QColor NALUnitView::getNALUnitColor(const NALUnitInfo& info) {
    // Color coding based on NAL type
    // SPS/PPS/VPS: Orange
    if (info.typeName.contains("SPS") || info.typeName.contains("PPS") || info.typeName.contains("VPS")) {
        return QColor(255, 140, 0);  // Brighter orange
    }

    // SEI: Light Blue
    if (info.typeName.contains("SEI")) {
        return QColor(100, 150, 255);  // Lighter blue for better contrast
    }

    // IDR/I-Slices: Bright Green
    if (info.isIDR || info.sliceType == "I") {
        return QColor(50, 200, 50);  // Brighter green
    }

    // P-Slices: Light Green
    if (info.sliceType.contains("P")) {
        return QColor(120, 220, 120);  // Even lighter green
    }

    // B-Slices: Light Purple
    if (info.sliceType.contains("B")) {
        return QColor(200, 100, 200);  // Lighter purple
    }

    // Default: Light Gray (good contrast on dark background)
    return QColor(200, 200, 200);
}

} // namespace VideoStudio
