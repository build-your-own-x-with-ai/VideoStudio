#ifndef VBVANALYZER_H
#define VBVANALYZER_H

#include <QVector>
#include "FrameInfo.h"

struct VBVStats {
    double maxOccupancy;          // 最大占用率（%）
    double minOccupancy;          // 最小占用率（%）
    double avgOccupancy;          // 平均占用率（%）
    int overflows;                // 上溢次数
    int underflows;               // 下溢次数
    double bufferSize;            // 缓冲区大小（bits）
    double targetBitrate;         // 目标比特率（bps）
    double initialDelay;          // 初始延迟（秒）
};

struct VBVPoint {
    double timestamp;             // 时间（秒）
    double occupancy;             // 占用率（%）
    int frameNumber;              // 帧号
    bool isOverflow;              // 是否上溢
    bool isUnderflow;             // 是否下溢
};

class VBVAnalyzer {
public:
    VBVAnalyzer();

    void analyze(const QVector<FrameInfo>& frames,
                 double bufferSize, double bitrate, double initialDelay = 0.0);
    void clear();

    VBVStats getStats() const { return stats; }
    QVector<VBVPoint> getOccupancyCurve() const { return occupancyCurve; }

private:
    void simulateVBV(const QVector<FrameInfo>& frames);

    VBVStats stats;
    QVector<VBVPoint> occupancyCurve;
};

#endif // VBVANALYZER_H
