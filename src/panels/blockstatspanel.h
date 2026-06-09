#ifndef BLOCKSTATSPANEL_H
#define BLOCKSTATSPANEL_H

#include <QWidget>
#include <QTreeWidget>
#include <QMap>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace VideoStudio {

class VideoDecoder;

struct BlockStatistics {
    // Frame type distribution
    int totalFrames;
    int iFrameCount;
    int pFrameCount;
    int bFrameCount;

    // Current frame stats
    int totalBlocks;
    int intraMBCount;                    // Intra-predicted blocks
    int interMBCount;                    // Inter-predicted blocks
    int skipMBCount;                     // Skip blocks
    QMap<int, int> blockSizeCounts;     // Block size -> count

    // Motion vector statistics
    int zeroMVCount;                     // Blocks with zero motion
    int nonZeroMVCount;                  // Blocks with non-zero motion
    double avgMotionMagnitude;           // Average MV magnitude (pixels)
    double maxMotionMagnitude;           // Maximum MV magnitude

    // QP statistics
    int minQP;
    int maxQP;
    double avgQP;

    // Reference frame usage
    QMap<int, int> refIdx0Count;        // ref_idx_l0 -> count
    QMap<int, int> refIdx1Count;        // ref_idx_l1 -> count

    BlockStatistics()
        : totalFrames(0), iFrameCount(0), pFrameCount(0), bFrameCount(0),
          totalBlocks(0), intraMBCount(0), interMBCount(0), skipMBCount(0),
          zeroMVCount(0), nonZeroMVCount(0),
          avgMotionMagnitude(0.0), maxMotionMagnitude(0.0),
          minQP(51), maxQP(0), avgQP(0.0) {}
};

class BlockStatsPanel : public QWidget {
    Q_OBJECT

public:
    explicit BlockStatsPanel(QWidget* parent = nullptr);
    ~BlockStatsPanel();

    void setVideoDecoder(VideoDecoder* decoder);
    void updateStatistics(AVFrame* frame);
    void clear();

private:
    void createUI();
    void updateOverallStatistics();
    void updateCurrentFrameStatistics(AVFrame* frame);
    BlockStatistics analyzeFrame(AVFrame* frame);
    void displayStatistics(const BlockStatistics& stats);

    QTreeWidget* m_statsTree;
    VideoDecoder* m_decoder;
    BlockStatistics m_overallStats;
    BlockStatistics m_currentFrameStats;
};

} // namespace VideoStudio

#endif // BLOCKSTATSPANEL_H
