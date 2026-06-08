#include "widgets/nalunitview.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <QTextStream>
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QTimer>
#include <algorithm>

namespace VideoStudio {

// Arrow overlay widget that sits on top of tree viewport
class ArrowOverlayWidget : public QWidget {
public:
    explicit ArrowOverlayWidget(QWidget* parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);  // Pass mouse events through
        setAttribute(Qt::WA_TranslucentBackground);       // Transparent background
        setAutoFillBackground(false);
        qDebug() << "ArrowOverlayWidget: Created with parent" << parent << "geometry:" << geometry();
    }

    void setArrows(const QVector<NALUnitView::ArrowInfo>& arrows, QTreeWidget* tree) {
        qDebug() << "ArrowOverlayWidget::setArrows: Called with" << arrows.size() << "arrows, geometry:" << geometry();
        m_arrows = arrows;
        m_treeWidget = tree;
        qDebug() << "ArrowOverlayWidget::setArrows: Calling update()";
        update();
        qDebug() << "ArrowOverlayWidget::setArrows: After update(), isVisible:" << isVisible() << "rect:" << rect();
    }

    void clearArrows() {
        m_arrows.clear();
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);

        if (m_arrows.isEmpty() || !m_treeWidget) {
            return;
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        qDebug() << "ArrowOverlayWidget::paintEvent: Drawing" << m_arrows.size() << "arrows";

        // Draw each arrow
        for (const NALUnitView::ArrowInfo& arrow : m_arrows) {
            if (!arrow.sourceItem || !arrow.targetItem) {
                continue;
            }

            // Get visual rectangles for source and target items
            QRect sourceRect = m_treeWidget->visualItemRect(arrow.sourceItem);
            QRect targetRect = m_treeWidget->visualItemRect(arrow.targetItem);

            qDebug() << "ArrowOverlayWidget::paintEvent: sourceRect:" << sourceRect << "targetRect:" << targetRect;

            // Arrows in the # column area
            int arrowX = 5;
            QPoint sourcePos(arrowX, sourceRect.center().y());
            QPoint targetPos(arrowX, targetRect.center().y());

            qDebug() << "ArrowOverlayWidget::paintEvent: Drawing arrow from" << sourcePos << "to" << targetPos << "color:" << arrow.color;

            // Draw L-shaped arrow path
            QPainterPath path;
            path.moveTo(sourcePos);

            int horizontalOffset = 15;
            QPoint corner1(sourcePos.x() - horizontalOffset, sourcePos.y());
            QPoint corner2(targetPos.x() - horizontalOffset, targetPos.y());

            path.lineTo(corner1);
            path.lineTo(corner2);
            path.lineTo(targetPos);

            // Draw arrow line - use bright red for testing visibility
            QPen pen(Qt::red);
            pen.setWidth(10);  // Very thick for testing
            painter.setPen(pen);
            painter.drawPath(path);

            // Draw arrowhead at target - pointing right (→)
            int arrowSize = 12;
            QPointF arrowTip = targetPos;
            QPointF arrowP1 = arrowTip + QPointF(-arrowSize, -arrowSize / 2);
            QPointF arrowP2 = arrowTip + QPointF(-arrowSize, arrowSize / 2);
            QPolygonF arrowHead;
            arrowHead << arrowTip << arrowP1 << arrowP2;
            painter.setBrush(Qt::red);
            painter.setPen(Qt::NoPen);
            painter.drawPolygon(arrowHead);
        }
    }

private:
    QVector<NALUnitView::ArrowInfo> m_arrows;
    QTreeWidget* m_treeWidget = nullptr;
};


NALUnitView::NALUnitView(QWidget* parent)
    : QWidget(parent)
    , m_treeWidget(new QTreeWidget(this))
    , m_exportButton(new QPushButton("Export to CSV", this))
    , m_parser(nullptr)
    , m_currentNALIndex(-1)
    , m_arrowOverlay(nullptr)
{
    // Setup tree widget
    m_treeWidget->setColumnCount(6);
    QStringList headers;
    headers << "#" << "Offset" << "Frame" << "Type" << "Size" << "Properties";
    m_treeWidget->setHeaderLabels(headers);

    // Set column widths
    m_treeWidget->setColumnWidth(0, 50);   // Index (narrower)
    m_treeWidget->setColumnWidth(1, 90);   // Offset
    m_treeWidget->setColumnWidth(2, 60);   // Frame (narrower)
    m_treeWidget->setColumnWidth(3, 220);  // Type (wider)
    m_treeWidget->setColumnWidth(4, 90);   // Size
    m_treeWidget->setColumnWidth(5, 400);  // Properties (much wider)

    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setRootIsDecorated(false);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Setup export button
    m_exportButton->setMaximumWidth(150);

    // Connect signals
    connect(m_treeWidget, &QTreeWidget::itemClicked,
            this, &NALUnitView::onItemClicked);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &NALUnitView::onItemDoubleClicked);
    connect(m_exportButton, &QPushButton::clicked,
            this, &NALUnitView::onExportToCSV);

    // Layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Add button at top
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_exportButton);
    layout->addLayout(buttonLayout);

    layout->addWidget(m_treeWidget);

    // Install event filter on viewport to draw arrows
    m_treeWidget->viewport()->installEventFilter(this);
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
    const QVector<AudioFrameInfo>& audioFrames = m_parser->getAudioFrames();
    int totalNALs = nalUnits.size();
    int totalAudio = audioFrames.size();
    int totalItems = totalNALs + totalAudio;

    if (totalItems == 0) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);
        item->setText(0, "No NAL units or audio frames found");
        item->setForeground(0, QColor(150, 150, 150));
        return;
    }

    // Create combined list sorted by file offset
    struct MediaItem {
        enum Type { NAL, AUDIO };
        Type type;
        int index;
        int64_t offset;
    };

    QVector<MediaItem> items;
    for (int i = 0; i < totalNALs; ++i) {
        items.append({MediaItem::NAL, i, nalUnits[i].fileOffset});
    }
    for (int i = 0; i < totalAudio; ++i) {
        items.append({MediaItem::AUDIO, i, audioFrames[i].fileOffset});
    }

    // Sort by file offset
    std::sort(items.begin(), items.end(), [](const MediaItem& a, const MediaItem& b) {
        return a.offset < b.offset;
    });

    // Display limit: 1000 items (same as PacketView)
    int maxItems = qMin(1000, totalItems);

    for (int i = 0; i < maxItems; ++i) {
        const MediaItem& mediaItem = items[i];
        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);

        if (mediaItem.type == MediaItem::NAL) {
            const NALUnitInfo& nalInfo = nalUnits[mediaItem.index];

            // Store NAL index and type in user data
            item->setData(0, Qt::UserRole, QVariant::fromValue(QPair<int, int>(0, nalInfo.index)));  // 0 = NAL type

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

            // Color coding - apply to Type and Properties columns
            QColor color = getNALUnitColor(nalInfo);
            item->setForeground(3, color);  // Type column

            // For slices, add background color for better visibility
            if (nalInfo.isSlice) {
                QColor bgColor = color;
                bgColor.setAlpha(30);  // Semi-transparent background
                item->setBackground(3, bgColor);
            }
        } else {
            // Audio frame
            const AudioFrameInfo& audioInfo = audioFrames[mediaItem.index];

            // Store audio index and type in user data
            item->setData(0, Qt::UserRole, QVariant::fromValue(QPair<int, int>(1, audioInfo.index)));  // 1 = AUDIO type

            // Column 0: Index
            item->setText(0, QString("A%1").arg(audioInfo.index));

            // Column 1: Offset (hex)
            item->setText(1, QString("0x%1").arg(audioInfo.fileOffset, 0, 16));

            // Column 2: Frame number
            item->setText(2, QString::number(audioInfo.frameNumber));

            // Column 3: Type
            item->setText(3, audioInfo.frameName);

            // Column 4: Size
            item->setText(4, QString("%1 B").arg(audioInfo.size));

            // Column 5: Properties
            item->setText(5, audioInfo.codecType);

            // Color coding for audio - dark blue
            QColor audioColor(50, 100, 200);
            for (int col = 2; col <= 3; ++col) {
                item->setForeground(col, audioColor);
            }
        }
    }

    // Show message if more items exist
    if (totalItems > maxItems) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);
        item->setText(0, QString("... (%1 more items)").arg(totalItems - maxItems));
        item->setForeground(0, QColor(150, 150, 150));
    }

    qDebug() << "NALUnitView: Built list with" << maxItems << "items (NAL:" << totalNALs << "Audio:" << totalAudio << "Total:" << totalItems << ")";
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

    // Clear previous highlights
    clearReferenceHighlights();

    QVariant userData = item->data(0, Qt::UserRole);
    if (userData.canConvert<QPair<int, int>>()) {
        QPair<int, int> typeAndIndex = userData.value<QPair<int, int>>();
        int itemType = typeAndIndex.first;  // 0 = NAL, 1 = AUDIO
        int itemIndex = typeAndIndex.second;

        if (itemType == 0) {
            // NAL unit
            m_currentNALIndex = itemIndex;
            qDebug() << "NALUnitView::onItemClicked: Emitting nalUnitSelected signal with nalIndex:" << itemIndex;

            // Highlight referenced frames
            highlightReferencedFrames(itemIndex);

            emit nalUnitSelected(itemIndex);
        } else {
            // Audio frame
            qDebug() << "NALUnitView::onItemClicked: Emitting audioFrameSelected signal with audioIndex:" << itemIndex;
            emit audioFrameSelected(itemIndex);
        }
    } else {
        // Old format (backward compatibility)
        int nalIndex = item->data(0, Qt::UserRole).toInt();
        m_currentNALIndex = nalIndex;
        qDebug() << "NALUnitView::onItemClicked: Emitting nalUnitSelected signal with nalIndex:" << nalIndex;

        highlightReferencedFrames(nalIndex);

        emit nalUnitSelected(nalIndex);
    }
}

void NALUnitView::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);

    if (!item || !m_parser) {
        return;
    }

    QVariant userData = item->data(0, Qt::UserRole);
    if (userData.canConvert<QPair<int, int>>()) {
        QPair<int, int> typeAndIndex = userData.value<QPair<int, int>>();
        int itemType = typeAndIndex.first;  // 0 = NAL, 1 = AUDIO
        int itemIndex = typeAndIndex.second;

        if (itemType == 0) {
            // NAL unit
            const NALUnitInfo* nalInfo = m_parser->getNALUnit(itemIndex);
            if (nalInfo) {
                emit nalUnitDoubleClicked(itemIndex);
                emit frameSelected(nalInfo->frameNumber);
            }
        } else {
            // Audio frame
            const AudioFrameInfo* audioInfo = m_parser->getAudioFrame(itemIndex);
            if (audioInfo) {
                emit frameSelected(audioInfo->frameNumber);
            }
        }
    } else {
        // Old format (backward compatibility)
        int nalIndex = item->data(0, Qt::UserRole).toInt();
        const NALUnitInfo* nalInfo = m_parser->getNALUnit(nalIndex);
        if (nalInfo) {
            emit nalUnitDoubleClicked(nalIndex);
            emit frameSelected(nalInfo->frameNumber);
        }
    }
}

QString NALUnitView::formatProperties(const NALUnitInfo& info) {
    QStringList props;

    // For slices, prioritize slice type and QP
    if (info.isSlice && !info.sliceType.isEmpty()) {
        QString sliceInfo = QString("Slice=%1").arg(info.sliceType);
        if (info.sliceQP >= 0) {
            sliceInfo += QString(", QP=%1").arg(info.sliceQP);
        }
        props << sliceInfo;

        // Add first MB if non-zero
        if (info.firstMbInSlice > 0) {
            props << QString("FirstMB=%1").arg(info.firstMbInSlice);
        }

        // Add PPS ID
        if (info.ppsId >= 0) {
            props << QString("PPS_ID=%1").arg(info.ppsId);
        }
    } else {
        // For non-slice NALs, show IDR/KeyFrame flags
        if (info.isIDR) {
            props << "IDR";
        } else if (info.isKeyFrame) {
            props << "KeyFrame";
        }
    }

    // HEVC layer info
    if (info.layerId > 0) {
        props << QString("Layer=%1").arg(info.layerId);
    }

    if (info.temporalId >= 0) {
        props << QString("Temporal=%1").arg(info.temporalId);
    }

    return props.join(", ");
}

QColor NALUnitView::getNALUnitColor(const NALUnitInfo& info) {
    // Color coding based on NAL type and slice type

    // SPS/PPS/VPS: Orange
    if (info.typeName.contains("SPS") || info.typeName.contains("PPS") || info.typeName.contains("VPS")) {
        return QColor(255, 140, 0);  // Orange
    }

    // SEI: Blue
    if (info.typeName.contains("SEI")) {
        return QColor(70, 130, 255);  // Bright blue
    }

    // Slice colors based on type
    if (info.isSlice) {
        // I-Slices (IDR or regular I): Dark Green
        if (info.isIDR || info.sliceType == "I") {
            return QColor(34, 177, 76);  // Professional green
        }

        // P-Slices: Teal/Cyan
        if (info.sliceType.contains("P")) {
            return QColor(0, 162, 232);  // Bright cyan for good visibility
        }

        // B-Slices: Magenta/Pink
        if (info.sliceType.contains("B")) {
            return QColor(236, 0, 140);  // Bright magenta - much more visible!
        }
    }

    // Default: Light Gray
    return QColor(180, 180, 180);
}

void NALUnitView::onExportToCSV() {
    if (!m_parser) {
        return;
    }

    // Open file dialog
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export NAL Units to CSV",
        QDir::homePath() + "/nal_units.csv",
        "CSV Files (*.csv);;All Files (*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    exportNALUnitsToCSV(filePath);
}

void NALUnitView::exportNALUnitsToCSV(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for writing:" << filePath;
        return;
    }

    QTextStream out(&file);

    // Write CSV header
    out << "Index,Offset,Frame,Type,NAL_Type_Value,Size_Bytes,"
        << "Is_Slice,Slice_Type,Slice_QP,First_MB,Slice_Type_Value,"
        << "Frame_Number,POC_LSB,PPS_ID,IDR_Pic_ID,"
        << "Is_IDR,Is_KeyFrame,Layer_ID,Temporal_ID,"
        << "SPS_Profile,SPS_Level,SPS_Width,SPS_Height,SPS_Chroma_Format,"
        << "SPS_Bit_Depth_Luma,SPS_Bit_Depth_Chroma,"
        << "PPS_Entropy_Mode,PPS_Num_Slice_Groups,PPS_Deblocking_Filter,"
        << "PPS_Weighted_Pred,PPS_Weighted_Bipred\n";

    const QVector<NALUnitInfo>& nalUnits = m_parser->getNALUnits();

    for (const NALUnitInfo& nal : nalUnits) {
        // Index
        out << nal.index << ",";

        // Offset (hex)
        out << "0x" << QString::number(nal.fileOffset, 16) << ",";

        // Frame
        out << nal.frameNumber << ",";

        // Type name (escape commas)
        QString typeName = nal.typeName;
        typeName.replace("\"", "\"\"");  // Escape quotes
        out << "\"" << typeName << "\",";

        // NAL Type Value
        out << nal.nalUnitType << ",";

        // Size
        out << nal.size << ",";

        // Slice info
        out << (nal.isSlice ? "Yes" : "No") << ",";
        out << (nal.sliceType.isEmpty() ? "" : nal.sliceType) << ",";
        out << (nal.sliceQP >= 0 ? QString::number(nal.sliceQP) : "") << ",";
        out << (nal.firstMbInSlice >= 0 ? QString::number(nal.firstMbInSlice) : "") << ",";
        out << (nal.sliceTypeValue >= 0 ? QString::number(nal.sliceTypeValue) : "") << ",";
        out << (nal.frameNum >= 0 ? QString::number(nal.frameNum) : "") << ",";
        out << (nal.picOrderCntLsb >= 0 ? QString::number(nal.picOrderCntLsb) : "") << ",";
        out << (nal.ppsId >= 0 ? QString::number(nal.ppsId) : "") << ",";
        out << (nal.idrPicId >= 0 ? QString::number(nal.idrPicId) : "") << ",";

        // Flags
        out << (nal.isIDR ? "Yes" : "No") << ",";
        out << (nal.isKeyFrame ? "Yes" : "No") << ",";
        out << (nal.layerId >= 0 ? QString::number(nal.layerId) : "") << ",";
        out << (nal.temporalId >= 0 ? QString::number(nal.temporalId) : "") << ",";

        // SPS info
        out << (nal.spsProfileIdc >= 0 ? QString::number(nal.spsProfileIdc) : "") << ",";
        out << (nal.spsLevelIdc >= 0 ? QString::number(nal.spsLevelIdc) : "") << ",";
        out << (nal.spsWidth > 0 ? QString::number(nal.spsWidth) : "") << ",";
        out << (nal.spsHeight > 0 ? QString::number(nal.spsHeight) : "") << ",";
        out << (nal.spsChromaFormat >= 0 ? QString::number(nal.spsChromaFormat) : "") << ",";
        out << (nal.spsBitDepthLuma > 0 ? QString::number(nal.spsBitDepthLuma) : "") << ",";
        out << (nal.spsBitDepthChroma > 0 ? QString::number(nal.spsBitDepthChroma) : "") << ",";

        // PPS info
        out << (nal.ppsEntropyCodingMode ? "CABAC" : "CAVLC") << ",";
        out << QString::number(nal.ppsNumSliceGroups) << ",";
        out << (nal.ppsDeblockingFilter ? "Yes" : "No") << ",";
        out << (nal.ppsWeightedPred ? "Yes" : "No") << ",";
        out << QString::number(nal.ppsWeightedBipred);

        out << "\n";
    }

    file.close();
    qDebug() << "Exported" << nalUnits.size() << "NAL units to" << filePath;
}

void NALUnitView::clearReferenceHighlights() {
    // Clear background colors for previously highlighted items
    for (QTreeWidgetItem* item : m_highlightedItems) {
        if (item) {
            for (int col = 0; col < m_treeWidget->columnCount(); ++col) {
                item->setBackground(col, QBrush());  // Clear background
            }
        }
    }
    m_highlightedItems.clear();

    // Clear arrows and trigger viewport repaint
    m_arrows.clear();
    m_treeWidget->viewport()->update();
}

int NALUnitView::findNALUnitByFrameNumber(int frameNumber) {
    if (!m_parser) return -1;

    const QVector<NALUnitInfo>& nalUnits = m_parser->getNALUnits();
    for (int i = 0; i < nalUnits.size(); ++i) {
        if (nalUnits[i].frameNumber == frameNumber && nalUnits[i].isSlice) {
            return i;
        }
    }
    return -1;
}

void NALUnitView::highlightReferencedFrames(int nalIndex) {
    if (!m_parser) return;

    const NALUnitInfo* nalInfo = m_parser->getNALUnit(nalIndex);
    if (!nalInfo || !nalInfo->isSlice) return;

    // Only P and B slices have references
    if (nalInfo->sliceType != "P" && nalInfo->sliceType != "B") return;

    qDebug() << "NALUnitView: Highlighting references for NAL" << nalIndex
             << "Frame" << nalInfo->frameNumber
             << "Type" << nalInfo->sliceType;

    // Calculate referenced frame numbers based on POC and reference indices
    // This is simplified - in reality, reference frames are determined by DPB
    int currentFrame = nalInfo->frameNumber;

    QVector<int> referencedFrames;

    // For P slices: typically reference the previous I or P frame
    if (nalInfo->sliceType == "P") {
        // Simple heuristic: reference previous keyframes/P-frames
        for (int i = nalIndex - 1; i >= 0; --i) {
            const NALUnitInfo* refNal = m_parser->getNALUnit(i);
            if (refNal && refNal->isSlice &&
                (refNal->sliceType == "I" || refNal->sliceType == "P")) {
                referencedFrames.append(i);
                break;  // Only reference the most recent one (simplified)
            }
        }
    }
    // For B slices: reference both past and future frames
    else if (nalInfo->sliceType == "B") {
        // Find previous reference frame (L0 - past)
        for (int i = nalIndex - 1; i >= 0; --i) {
            const NALUnitInfo* refNal = m_parser->getNALUnit(i);
            if (refNal && refNal->isSlice &&
                (refNal->sliceType == "I" || refNal->sliceType == "P")) {
                referencedFrames.append(i);
                break;
            }
        }
        // Find next reference frame (L1 - future)
        for (int i = nalIndex + 1; i < m_parser->getNALUnits().size(); ++i) {
            const NALUnitInfo* refNal = m_parser->getNALUnit(i);
            if (refNal && refNal->isSlice &&
                (refNal->sliceType == "I" || refNal->sliceType == "P")) {
                referencedFrames.append(i);
                break;
            }
        }
    }

    qDebug() << "NALUnitView: Found" << referencedFrames.size() << "reference frames";

    // Get the current item for arrow drawing
    QTreeWidgetItem* currentItem = nullptr;
    for (int row = 0; row < m_treeWidget->topLevelItemCount(); ++row) {
        QTreeWidgetItem* item = m_treeWidget->topLevelItem(row);
        QVariant userData = item->data(0, Qt::UserRole);

        if (userData.canConvert<QPair<int, int>>()) {
            QPair<int, int> typeAndIndex = userData.value<QPair<int, int>>();
            int itemType = typeAndIndex.first;
            int itemIndex = typeAndIndex.second;

            if (itemType == 0 && itemIndex == nalIndex) {
                currentItem = item;
                break;
            }
        }
    }

    // Collect arrows
    QVector<ArrowInfo> arrows;

    // Highlight the referenced items in the tree
    for (int refNalIndex : referencedFrames) {
        // Find the tree item corresponding to this NAL index
        for (int row = 0; row < m_treeWidget->topLevelItemCount(); ++row) {
            QTreeWidgetItem* item = m_treeWidget->topLevelItem(row);
            QVariant userData = item->data(0, Qt::UserRole);

            if (userData.canConvert<QPair<int, int>>()) {
                QPair<int, int> typeAndIndex = userData.value<QPair<int, int>>();
                int itemType = typeAndIndex.first;
                int itemIndex = typeAndIndex.second;

                if (itemType == 0 && itemIndex == refNalIndex) {
                    // Highlight this item
                    QColor highlightColor;
                    if (nalInfo->sliceType == "P") {
                        highlightColor = QColor(0, 162, 232, 80);  // Cyan (L0 - past)
                    } else {
                        // For B slices, differentiate past vs future
                        if (refNalIndex < nalIndex) {
                            highlightColor = QColor(0, 162, 232, 80);  // Cyan (L0 - past)
                        } else {
                            highlightColor = QColor(236, 0, 140, 80);  // Magenta (L1 - future)
                        }
                    }

                    for (int col = 0; col < m_treeWidget->columnCount(); ++col) {
                        item->setBackground(col, highlightColor);
                    }

                    m_highlightedItems.append(item);

                    // Add arrow info for drawing (arrow points FROM reference TO current)
                    if (currentItem) {
                        ArrowInfo arrow;
                        arrow.sourceItem = item;          // Start from referenced item
                        arrow.targetItem = currentItem;   // Point to current item
                        arrow.color = highlightColor;
                        arrows.append(arrow);
                    }

                    qDebug() << "NALUnitView: Highlighted NAL" << refNalIndex << "at row" << row;
                    break;
                }
            }
        }
    }

    // Store arrows and trigger viewport repaint
    m_arrows = arrows;
    m_treeWidget->viewport()->update();
}

bool NALUnitView::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_treeWidget->viewport()) {
        if (event->type() == QEvent::Paint && !m_arrows.isEmpty()) {
            // Let the tree paint first - don't intercept the event
            QWidget::eventFilter(obj, event);

            // Now draw arrows on top using a separate painter after tree is painted
            QPainter painter(m_treeWidget->viewport());
            painter.setRenderHint(QPainter::Antialiasing);

            qDebug() << "NALUnitView::eventFilter: Drawing" << m_arrows.size() << "arrows on viewport";

            for (const ArrowInfo& arrow : m_arrows) {
                if (!arrow.sourceItem || !arrow.targetItem) {
                    continue;
                }

                QRect sourceRect = m_treeWidget->visualItemRect(arrow.sourceItem);
                QRect targetRect = m_treeWidget->visualItemRect(arrow.targetItem);

                // Draw arrow on the RIGHT side of # column
                int columnLeftEdge = m_treeWidget->columnViewportPosition(0);
                int columnWidth = m_treeWidget->columnWidth(0);

                // Position arrow just inside the right edge of # column
                int arrowX = columnLeftEdge + columnWidth - 8;  // 8 pixels from right edge

                int sourceY = sourceRect.center().y();
                int targetY = targetRect.center().y();

                // Use proper colors
                QColor arrowColor = arrow.color;
                arrowColor.setAlpha(220);
                QPen pen(arrowColor);
                pen.setWidth(2);
                painter.setPen(pen);

                // Draw simple vertical line connecting source to target
                painter.drawLine(arrowX, sourceY, arrowX, targetY);

                // Draw larger arrowheads at BOTH ends
                int arrowSize = 10;  // Large arrowheads

                painter.setBrush(arrowColor);
                painter.setPen(Qt::NoPen);

                // Arrowhead at source - pointing TOWARD target
                QPolygonF sourceArrowHead;
                if (targetY > sourceY) {
                    // Target is BELOW source, so arrow points UP (▲)
                    QPointF tip(arrowX, sourceY);
                    QPointF left = tip + QPointF(-arrowSize/2.0, arrowSize);
                    QPointF right = tip + QPointF(arrowSize/2.0, arrowSize);
                    sourceArrowHead << tip << left << right;
                } else {
                    // Target is ABOVE source, so arrow points DOWN (▼)
                    QPointF tip(arrowX, sourceY);
                    QPointF left = tip + QPointF(-arrowSize/2.0, -arrowSize);
                    QPointF right = tip + QPointF(arrowSize/2.0, -arrowSize);
                    sourceArrowHead << tip << left << right;
                }
                painter.drawPolygon(sourceArrowHead);

                // Arrowhead at target - pointing TOWARD target
                QPolygonF targetArrowHead;
                if (targetY > sourceY) {
                    // Arrow pointing DOWN (▼) - toward target
                    QPointF tip(arrowX, targetY);
                    QPointF left = tip + QPointF(-arrowSize/2.0, -arrowSize);
                    QPointF right = tip + QPointF(arrowSize/2.0, -arrowSize);
                    targetArrowHead << tip << left << right;
                } else {
                    // Arrow pointing UP (▲) - toward target
                    QPointF tip(arrowX, targetY);
                    QPointF left = tip + QPointF(-arrowSize/2.0, arrowSize);
                    QPointF right = tip + QPointF(arrowSize/2.0, arrowSize);
                    targetArrowHead << tip << left << right;
                }
                painter.drawPolygon(targetArrowHead);
            }

            return false;  // Don't block the event
        }
    }

    return QWidget::eventFilter(obj, event);
}

} // namespace VideoStudio
