#ifndef TIMESTAMPANALYZER_H
#define TIMESTAMPANALYZER_H

#include <QVector>
#include "FrameInfo.h"

struct TimestampStats {
    double avgFrameInterval;      // 平均帧间隔（秒）
    double maxFrameInterval;      // 最大帧间隔
    double minFrameInterval;      // 最小帧间隔
    double jitter;                // 时间戳抖动（标准差）
    int discontinuities;          // 不连续点数量
    int ptsReversals;             // PTS 倒序次数
    int dtsReversals;             // DTS 倒序次数
    int duplicatePTS;             // 重复 PTS 次数
    int duplicateDTS;             // 重复 DTS 次数
    QVector<int> discontinuityFrames;  // 不连续帧索引
    QVector<int> ptsReversalFrames;    // PTS 倒序帧索引
    QVector<int> dtsReversalFrames;    // DTS 倒序帧索引
};

class TimestampAnalyzer {
public:
    TimestampAnalyzer();

    void analyze(const QVector<FrameInfo>& frames);
    void clear();

    TimestampStats getStats() const { return stats; }
    QVector<double> getPTSCurve() const { return ptsCurve; }
    QVector<double> getDTSCurve() const { return dtsCurve; }
    QVector<double> getPTSDTSDiff() const { return ptsDtsDiff; }
    QVector<double> getFrameIntervals() const { return frameIntervals; }

private:
    void calculateStatistics(const QVector<FrameInfo>& frames);
    void detectAnomalies(const QVector<FrameInfo>& frames);

    TimestampStats stats;
    QVector<double> ptsCurve;
    QVector<double> dtsCurve;
    QVector<double> ptsDtsDiff;
    QVector<double> frameIntervals;
};

#endif // TIMESTAMPANALYZER_H
