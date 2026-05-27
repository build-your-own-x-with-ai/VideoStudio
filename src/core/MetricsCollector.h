#ifndef METRICSCOLLECTOR_H
#define METRICSCOLLECTOR_H

#include <QObject>
#include <QVector>
#include <QMap>
#include "FrameInfo.h"

struct FrameSizeDistribution {
    QVector<int> binEdges;      // 区间边界（字节）
    QVector<int> binCounts;     // 每个区间的帧数量
    QMap<QString, QVector<int>> typeDistribution; // 按帧类型分类的分布
    int totalFrames;
    int minSize;
    int maxSize;
    double avgSize;

    FrameSizeDistribution() : totalFrames(0), minSize(0), maxSize(0), avgSize(0.0) {}
};

class MetricsCollector : public QObject {
    Q_OBJECT

public:
    explicit MetricsCollector(QObject* parent = nullptr);

    void addFrame(const FrameInfo& frame);
    void addFrames(const QVector<FrameInfo>& frames);
    void clear();

    int getFrameCount() const { return frames.size(); }
    const FrameInfo& getFrame(int index) const { return frames[index]; }
    const QVector<FrameInfo>& getAllFrames() const { return frames; }

    int getIFrameCount() const;
    int getPFrameCount() const;
    int getBFrameCount() const;

    double getAverageBitrate() const;
    int64_t getTotalSize() const;

    FrameSizeDistribution calculateFrameSizeDistribution(int numBins = 50) const;

signals:
    void frameAdded(const FrameInfo& frame);
    void metricsUpdated();

private:
    QVector<FrameInfo> frames;
};

#endif // METRICSCOLLECTOR_H
