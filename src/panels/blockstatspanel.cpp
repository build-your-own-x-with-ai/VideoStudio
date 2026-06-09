#include "panels/blockstatspanel.h"
#include "core/videodecoder.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <cmath>

extern "C" {
#include <libavutil/motion_vector.h>
#include <libavutil/video_enc_params.h>
}

namespace VideoStudio {

BlockStatsPanel::BlockStatsPanel(QWidget* parent)
    : QWidget(parent)
    , m_statsTree(nullptr)
    , m_decoder(nullptr)
{
    createUI();
}

BlockStatsPanel::~BlockStatsPanel() {
}

void BlockStatsPanel::setVideoDecoder(VideoDecoder* decoder) {
    m_decoder = decoder;
    m_overallStats = BlockStatistics();  // Reset statistics
    m_currentFrameStats = BlockStatistics();
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

    // Update current frame statistics
    m_currentFrameStats = analyzeFrame(frame);

    // Update overall statistics
    updateOverallStatistics();
    updateCurrentFrameStatistics(frame);

    displayStatistics(m_currentFrameStats);
}

void BlockStatsPanel::clear() {
    m_statsTree->clear();
}

BlockStatistics BlockStatsPanel::analyzeFrame(AVFrame* frame) {
    BlockStatistics stats;

    if (!frame) {
        return stats;
    }

    // Update frame type based on pict_type
    switch (frame->pict_type) {
        case AV_PICTURE_TYPE_I:
            stats.iFrameCount = 1;
            break;
        case AV_PICTURE_TYPE_P:
            stats.pFrameCount = 1;
            break;
        case AV_PICTURE_TYPE_B:
            stats.bFrameCount = 1;
            break;
        default:
            break;
    }

    // Get QP data from video encoding parameters
    AVFrameSideData* encParamsSd = av_frame_get_side_data(frame, AV_FRAME_DATA_VIDEO_ENC_PARAMS);
    if (encParamsSd) {
        AVVideoEncParams* encParams = reinterpret_cast<AVVideoEncParams*>(encParamsSd->data);

        // Initialize QP statistics
        stats.minQP = 51;
        stats.maxQP = 0;
        double totalQP = 0.0;
        int qpCount = 0;

        // Process per-block QP values
        for (uint32_t i = 0; i < encParams->nb_blocks; i++) {
            AVVideoBlockParams* block = av_video_enc_params_block(encParams, i);
            int qp = encParams->qp + block->delta_qp;

            if (qp < stats.minQP) stats.minQP = qp;
            if (qp > stats.maxQP) stats.maxQP = qp;
            totalQP += qp;
            qpCount++;
        }

        if (qpCount > 0) {
            stats.avgQP = totalQP / qpCount;
        }
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

            // Count reference frame indices
            int refIdx = mv->source;
            if (refIdx >= 0) {
                // Assume ref_idx_l0 for now (would need more info to distinguish l0/l1)
                stats.refIdx0Count[refIdx]++;
            }
        }

        // Motion vector statistics
        if (mv->motion_x == 0 && mv->motion_y == 0) {
            stats.zeroMVCount++;
        } else {
            stats.nonZeroMVCount++;
            // Motion vectors are in quarter-pixel units
            double mvX = mv->motion_x / 4.0;
            double mvY = mv->motion_y / 4.0;
            double magnitude = std::sqrt(mvX * mvX + mvY * mvY);
            totalMotion += magnitude;

            if (magnitude > stats.maxMotionMagnitude) {
                stats.maxMotionMagnitude = magnitude;
            }
        }

        // Count skip blocks (blocks with zero motion and predicted from reference)
        if (!isIntra && mv->motion_x == 0 && mv->motion_y == 0) {
            stats.skipMBCount++;
        }
    }

    // Calculate average motion
    if (stats.nonZeroMVCount > 0) {
        stats.avgMotionMagnitude = totalMotion / stats.nonZeroMVCount;
    }

    return stats;
}

void BlockStatsPanel::displayStatistics(const BlockStatistics& stats) {
    m_statsTree->clear();

    // Overall Video Statistics (from decoder)
    if (m_decoder && m_decoder->isOpen()) {
        QTreeWidgetItem* overallItem = new QTreeWidgetItem(m_statsTree);
        overallItem->setText(0, "Overall Video Statistics");
        overallItem->setExpanded(true);
        overallItem->setBackground(0, QColor(220, 240, 255));
        overallItem->setBackground(1, QColor(220, 240, 255));
        overallItem->setForeground(0, QBrush(QColor(0, 0, 0)));
        overallItem->setForeground(1, QBrush(QColor(0, 0, 0)));
        QFont boldFont = overallItem->font(0);
        boldFont.setBold(true);
        overallItem->setFont(0, boldFont);

        QTreeWidgetItem* totalFramesItem = new QTreeWidgetItem(overallItem);
        totalFramesItem->setText(0, "Total Frames");
        totalFramesItem->setText(1, QString::number(m_overallStats.totalFrames));

        if (m_overallStats.totalFrames > 0) {
            QTreeWidgetItem* iFramesItem = new QTreeWidgetItem(overallItem);
            iFramesItem->setText(0, "I-Frames");
            double iPercent = 100.0 * m_overallStats.iFrameCount / m_overallStats.totalFrames;
            iFramesItem->setText(1, QString("%1 (%2%)").arg(m_overallStats.iFrameCount).arg(iPercent, 0, 'f', 1));
            iFramesItem->setForeground(1, QBrush(QColor(0, 200, 0)));

            QTreeWidgetItem* pFramesItem = new QTreeWidgetItem(overallItem);
            pFramesItem->setText(0, "P-Frames");
            double pPercent = 100.0 * m_overallStats.pFrameCount / m_overallStats.totalFrames;
            pFramesItem->setText(1, QString("%1 (%2%)").arg(m_overallStats.pFrameCount).arg(pPercent, 0, 'f', 1));
            pFramesItem->setForeground(1, QBrush(QColor(100, 150, 255)));

            QTreeWidgetItem* bFramesItem = new QTreeWidgetItem(overallItem);
            bFramesItem->setText(0, "B-Frames");
            double bPercent = 100.0 * m_overallStats.bFrameCount / m_overallStats.totalFrames;
            bFramesItem->setText(1, QString("%1 (%2%)").arg(m_overallStats.bFrameCount).arg(bPercent, 0, 'f', 1));
            bFramesItem->setForeground(1, QBrush(QColor(200, 100, 200)));
        }
    }

    // Current Frame Statistics
    QTreeWidgetItem* frameItem = new QTreeWidgetItem(m_statsTree);
    frameItem->setText(0, "Current Frame Statistics");
    frameItem->setExpanded(true);
    frameItem->setBackground(0, QColor(240, 240, 240));
    frameItem->setBackground(1, QColor(240, 240, 240));
    frameItem->setForeground(0, QBrush(QColor(0, 0, 0)));
    frameItem->setForeground(1, QBrush(QColor(0, 0, 0)));
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

    QTreeWidgetItem* skipItem = new QTreeWidgetItem(frameItem);
    skipItem->setText(0, "Skip Blocks");
    double skipPercent = stats.totalBlocks > 0 ? (100.0 * stats.skipMBCount / stats.totalBlocks) : 0.0;
    skipItem->setText(1, QString("%1 (%2%)").arg(stats.skipMBCount).arg(skipPercent, 0, 'f', 1));
    skipItem->setForeground(1, QBrush(QColor(150, 150, 150)));

    // QP Statistics
    if (stats.minQP <= 51 && stats.maxQP >= 0) {
        QTreeWidgetItem* qpItem = new QTreeWidgetItem(m_statsTree);
        qpItem->setText(0, "Quantization Parameter (QP)");
        qpItem->setExpanded(true);
        qpItem->setBackground(0, QColor(255, 245, 220));
        qpItem->setBackground(1, QColor(255, 245, 220));
        qpItem->setForeground(0, QBrush(QColor(0, 0, 0)));
        qpItem->setForeground(1, QBrush(QColor(0, 0, 0)));
        QFont boldFont = qpItem->font(0);
        boldFont.setBold(true);
        qpItem->setFont(0, boldFont);

        QTreeWidgetItem* minQPItem = new QTreeWidgetItem(qpItem);
        minQPItem->setText(0, "Min QP");
        minQPItem->setText(1, QString::number(stats.minQP));
        minQPItem->setForeground(1, QBrush(QColor(0, 180, 0)));

        QTreeWidgetItem* avgQPItem = new QTreeWidgetItem(qpItem);
        avgQPItem->setText(0, "Average QP");
        avgQPItem->setText(1, QString::number(stats.avgQP, 'f', 1));

        QTreeWidgetItem* maxQPItem = new QTreeWidgetItem(qpItem);
        maxQPItem->setText(0, "Max QP");
        maxQPItem->setText(1, QString::number(stats.maxQP));
        maxQPItem->setForeground(1, QBrush(QColor(220, 0, 0)));

        QTreeWidgetItem* rangeQPItem = new QTreeWidgetItem(qpItem);
        rangeQPItem->setText(0, "QP Range");
        rangeQPItem->setText(1, QString::number(stats.maxQP - stats.minQP));
    }

    // Motion vector statistics
    if (stats.interMBCount > 0) {
        QTreeWidgetItem* motionItem = new QTreeWidgetItem(m_statsTree);
        motionItem->setText(0, "Motion Statistics");
        motionItem->setExpanded(true);
        motionItem->setBackground(0, QColor(240, 255, 240));
        motionItem->setBackground(1, QColor(240, 255, 240));
        motionItem->setForeground(0, QBrush(QColor(0, 0, 0)));
        motionItem->setForeground(1, QBrush(QColor(0, 0, 0)));
        QFont boldFont = motionItem->font(0);
        boldFont.setBold(true);
        motionItem->setFont(0, boldFont);

        QTreeWidgetItem* zeroMVItem = new QTreeWidgetItem(motionItem);
        zeroMVItem->setText(0, "Zero Motion Blocks");
        double zeroPercent = stats.totalBlocks > 0 ? (100.0 * stats.zeroMVCount / stats.totalBlocks) : 0.0;
        zeroMVItem->setText(1, QString("%1 (%2%)").arg(stats.zeroMVCount).arg(zeroPercent, 0, 'f', 1));

        QTreeWidgetItem* nonZeroMVItem = new QTreeWidgetItem(motionItem);
        nonZeroMVItem->setText(0, "Non-Zero Motion Blocks");
        double nonZeroPercent = stats.totalBlocks > 0 ? (100.0 * stats.nonZeroMVCount / stats.totalBlocks) : 0.0;
        nonZeroMVItem->setText(1, QString("%1 (%2%)").arg(stats.nonZeroMVCount).arg(nonZeroPercent, 0, 'f', 1));

        QTreeWidgetItem* avgMotionItem = new QTreeWidgetItem(motionItem);
        avgMotionItem->setText(0, "Avg. Motion Magnitude");
        avgMotionItem->setText(1, QString("%1 px").arg(stats.avgMotionMagnitude, 0, 'f', 2));

        QTreeWidgetItem* maxMotionItem = new QTreeWidgetItem(motionItem);
        maxMotionItem->setText(0, "Max. Motion Magnitude");
        maxMotionItem->setText(1, QString("%1 px").arg(stats.maxMotionMagnitude, 0, 'f', 2));
    }

    // Reference frame usage
    if (!stats.refIdx0Count.isEmpty()) {
        QTreeWidgetItem* refItem = new QTreeWidgetItem(m_statsTree);
        refItem->setText(0, "Reference Frame Usage");
        refItem->setExpanded(true);
        refItem->setBackground(0, QColor(255, 240, 255));
        refItem->setBackground(1, QColor(255, 240, 255));
        refItem->setForeground(0, QBrush(QColor(0, 0, 0)));
        refItem->setForeground(1, QBrush(QColor(0, 0, 0)));
        QFont boldFont = refItem->font(0);
        boldFont.setBold(true);
        refItem->setFont(0, boldFont);

        QList<int> refIndices = stats.refIdx0Count.keys();
        std::sort(refIndices.begin(), refIndices.end());

        for (int refIdx : refIndices) {
            int count = stats.refIdx0Count[refIdx];
            double percent = stats.interMBCount > 0 ? (100.0 * count / stats.interMBCount) : 0.0;

            QTreeWidgetItem* refIdxItem = new QTreeWidgetItem(refItem);
            refIdxItem->setText(0, QString("ref_idx_l0[%1]").arg(refIdx));
            refIdxItem->setText(1, QString("%1 (%2%)").arg(count).arg(percent, 0, 'f', 1));
        }
    }

    // Block size distribution
    if (!stats.blockSizeCounts.isEmpty()) {
        QTreeWidgetItem* sizeItem = new QTreeWidgetItem(m_statsTree);
        sizeItem->setText(0, "Block Size Distribution");
        sizeItem->setExpanded(true);
        sizeItem->setBackground(0, QColor(240, 240, 240));
        sizeItem->setBackground(1, QColor(240, 240, 240));
        sizeItem->setForeground(0, QBrush(QColor(0, 0, 0)));
        sizeItem->setForeground(1, QBrush(QColor(0, 0, 0)));
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

void BlockStatsPanel::updateOverallStatistics() {
    if (!m_decoder || !m_decoder->isOpen()) {
        return;
    }

    const FrameIndex& frameIndex = m_decoder->getFrameIndex();
    m_overallStats.totalFrames = frameIndex.frameCount();
    m_overallStats.iFrameCount = frameIndex.getIFrameCount();
    m_overallStats.pFrameCount = frameIndex.getPFrameCount();
    m_overallStats.bFrameCount = frameIndex.getBFrameCount();
}

void BlockStatsPanel::updateCurrentFrameStatistics(AVFrame* frame) {
    // This method can be extended in the future to track additional
    // per-frame statistics that need to be accumulated over time
    Q_UNUSED(frame);
}

} // namespace VideoStudio
