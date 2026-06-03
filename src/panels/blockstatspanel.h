#ifndef BLOCKSTATSPANEL_H
#define BLOCKSTATSPANEL_H

#include <QWidget>
#include <QTreeWidget>
#include <QMap>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace VideoStudio {

struct BlockStatistics {
    QMap<int, int> blockSizeCounts;     // Block size -> count
    int intraMBCount;                    // Number of intra-predicted blocks
    int interMBCount;                    // Number of inter-predicted blocks
    int totalBlocks;                     // Total number of blocks
    double avgMotionMagnitude;           // Average motion vector magnitude
    int zeroMVCount;                     // Number of blocks with zero motion

    BlockStatistics()
        : intraMBCount(0), interMBCount(0), totalBlocks(0),
          avgMotionMagnitude(0.0), zeroMVCount(0) {}
};

class BlockStatsPanel : public QWidget {
    Q_OBJECT

public:
    explicit BlockStatsPanel(QWidget* parent = nullptr);
    ~BlockStatsPanel();

    void updateStatistics(AVFrame* frame);
    void clear();

private:
    void createUI();
    BlockStatistics analyzeFrame(AVFrame* frame);
    void displayStatistics(const BlockStatistics& stats);

    QTreeWidget* m_statsTree;
};

} // namespace VideoStudio

#endif // BLOCKSTATSPANEL_H
