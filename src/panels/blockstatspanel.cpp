#include "panels/blockstatspanel.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <cmath>

extern "C" {
#include <libavutil/motion_vector.h>
}

namespace VideoStudio {

BlockStatsPanel::BlockStatsPanel(QWidget* parent)
    : QWidget(parent)
    , m_statsTree(nullptr)
{
    createUI();
}

BlockStatsPanel::~BlockStatsPanel() {
}

void BlockStatsPanel::createUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_statsTree = new QTreeWidget(this);
    m_statsTree->setHeaderLabels(QStringList() << "Property" << "Value");
    m_statsTree->setAlternatingRowColors(true);
    m_statsTree->header()->setStretchLastSection(true);
    m_statsTree->setColumnWidth(0, 200);

    layout->addWidget(m_statsTree);
}

void BlockStatsPanel::updateStatistics(AVFrame* frame) {
    if (!frame) {
        clear();
        return;
    }

    BlockStatistics stats = analyzeFrame(frame);
    displayStatistics(stats);
}

void BlockStatsPanel::clear() {
    m_statsTree->clear();
}

BlockStatistics BlockStatsPanel::analyzeFrame(AVFrame* frame) {
    BlockStatistics stats;

    if (!frame) {
        return stats;
    }

    // Get motion vector side data
    AVFrameSideData* sd = av_frame_get_side_data(frame, AV_FRAME_DATA_MOTION_VECTORS);
    if (!sd) {
        return stats;
    }

    const AVMotionVector* mvs = reinterpret_cast<const AVMotionVector*>(sd->data);
    int mvCount = sd->size / sizeof(AVMotionVector);

    stats.totalBlocks = mvCount;
    double totalMotion = 0.0;

    for (int i = 0; i < mvCount; ++i) {
        const AVMotionVector* mv = &mvs[i];

        // Count block sizes
        int blockSize = mv->w;
        stats.blockSizeCounts[blockSize]++;

        // Count prediction modes
        bool isIntra = (mv->source == -1);
        if (isIntra) {
            stats.intraMBCount++;
        } else {
            stats.interMBCount++;
        }

        // Motion vector statistics
        if (mv->motion_x == 0 && mv->motion_y == 0) {
            stats.zeroMVCount++;
        } else {
            // Motion vectors are in quarter-pixel units
            double mvX = mv->motion_x / 4.0;
            double mvY = mv->motion_y / 4.0;
            double magnitude = std::sqrt(mvX * mvX + mvY * mvY);
            totalMotion += magnitude;
        }
    }

    // Calculate average motion
    if (stats.totalBlocks > stats.zeroMVCount) {
        stats.avgMotionMagnitude = totalMotion / (stats.totalBlocks - stats.zeroMVCount);
    }

    return stats;
}

void BlockStatsPanel::displayStatistics(const BlockStatistics& stats) {
    m_statsTree->clear();

    // Frame-level statistics
    QTreeWidgetItem* frameItem = new QTreeWidgetItem(m_statsTree);
    frameItem->setText(0, "Frame Statistics");
    frameItem->setExpanded(true);
    frameItem->setBackground(0, QColor(240, 240, 240));
    frameItem->setBackground(1, QColor(240, 240, 240));
    frameItem->setForeground(0, QBrush(QColor(0, 0, 0)));  // Black text
    frameItem->setForeground(1, QBrush(QColor(0, 0, 0)));  // Black text
    QFont boldFont = frameItem->font(0);
    boldFont.setBold(true);
    frameItem->setFont(0, boldFont);

    QTreeWidgetItem* totalItem = new QTreeWidgetItem(frameItem);
    totalItem->setText(0, "Total Blocks");
    totalItem->setText(1, QString::number(stats.totalBlocks));

    QTreeWidgetItem* intraItem = new QTreeWidgetItem(frameItem);
    intraItem->setText(0, "Intra-Predicted Blocks");
    double intraPercent = stats.totalBlocks > 0 ? (100.0 * stats.intraMBCount / stats.totalBlocks) : 0.0;
    intraItem->setText(1, QString("%1 (%2%)").arg(stats.intraMBCount).arg(intraPercent, 0, 'f', 1));
    intraItem->setForeground(1, QBrush(QColor(255, 100, 100)));

    QTreeWidgetItem* interItem = new QTreeWidgetItem(frameItem);
    interItem->setText(0, "Inter-Predicted Blocks");
    double interPercent = stats.totalBlocks > 0 ? (100.0 * stats.interMBCount / stats.totalBlocks) : 0.0;
    interItem->setText(1, QString("%1 (%2%)").arg(stats.interMBCount).arg(interPercent, 0, 'f', 1));
    interItem->setForeground(1, QBrush(QColor(100, 100, 255)));

    // Motion vector statistics
    if (stats.interMBCount > 0) {
        QTreeWidgetItem* motionItem = new QTreeWidgetItem(m_statsTree);
        motionItem->setText(0, "Motion Statistics");
        motionItem->setExpanded(true);
        motionItem->setBackground(0, QColor(240, 240, 240));
        motionItem->setBackground(1, QColor(240, 240, 240));
        motionItem->setForeground(0, QBrush(QColor(0, 0, 0)));  // Black text
        motionItem->setForeground(1, QBrush(QColor(0, 0, 0)));  // Black text
        QFont boldFont = motionItem->font(0);
        boldFont.setBold(true);
        motionItem->setFont(0, boldFont);

        QTreeWidgetItem* zeroMVItem = new QTreeWidgetItem(motionItem);
        zeroMVItem->setText(0, "Zero Motion Blocks");
        double zeroPercent = stats.totalBlocks > 0 ? (100.0 * stats.zeroMVCount / stats.totalBlocks) : 0.0;
        zeroMVItem->setText(1, QString("%1 (%2%)").arg(stats.zeroMVCount).arg(zeroPercent, 0, 'f', 1));

        QTreeWidgetItem* avgMotionItem = new QTreeWidgetItem(motionItem);
        avgMotionItem->setText(0, "Avg. Motion Magnitude");
        avgMotionItem->setText(1, QString("%1 px").arg(stats.avgMotionMagnitude, 0, 'f', 2));
    }

    // Block size distribution
    if (!stats.blockSizeCounts.isEmpty()) {
        QTreeWidgetItem* sizeItem = new QTreeWidgetItem(m_statsTree);
        sizeItem->setText(0, "Block Size Distribution");
        sizeItem->setExpanded(true);
        sizeItem->setBackground(0, QColor(240, 240, 240));
        sizeItem->setBackground(1, QColor(240, 240, 240));
        sizeItem->setForeground(0, QBrush(QColor(0, 0, 0)));  // Black text
        sizeItem->setForeground(1, QBrush(QColor(0, 0, 0)));  // Black text
        QFont boldFont = sizeItem->font(0);
        boldFont.setBold(true);
        sizeItem->setFont(0, boldFont);

        // Define colors for different block sizes
        QMap<int, QColor> blockColors;
        blockColors[64] = QColor(255, 100, 100);   // Red
        blockColors[32] = QColor(255, 180, 100);   // Orange
        blockColors[16] = QColor(255, 255, 100);   // Yellow
        blockColors[8] = QColor(100, 255, 100);    // Green
        blockColors[4] = QColor(100, 255, 255);    // Cyan

        // Sort by block size (descending)
        QList<int> sizes = stats.blockSizeCounts.keys();
        std::sort(sizes.begin(), sizes.end(), std::greater<int>());

        for (int size : sizes) {
            int count = stats.blockSizeCounts[size];
            double percent = stats.totalBlocks > 0 ? (100.0 * count / stats.totalBlocks) : 0.0;

            QTreeWidgetItem* blockItem = new QTreeWidgetItem(sizeItem);
            blockItem->setText(0, QString("%1x%1").arg(size));
            blockItem->setText(1, QString("%1 (%2%)").arg(count).arg(percent, 0, 'f', 1));

            // Set color indicator
            if (blockColors.contains(size)) {
                blockItem->setForeground(0, QBrush(blockColors[size]));
                blockItem->setForeground(1, QBrush(blockColors[size]));
            }
        }
    }

    // Auto-resize columns
    m_statsTree->resizeColumnToContents(0);
    m_statsTree->resizeColumnToContents(1);
}

} // namespace VideoStudio
