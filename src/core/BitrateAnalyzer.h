#ifndef BITRATEANALYZER_H
#define BITRATEANALYZER_H

#include <QObject>
#include <QVector>
#include "FrameInfo.h"

struct BitratePoint {
    double timestamp;
    double bitrate;
};

struct BitrateStats {
    double averageBitrate;
    double peakBitrate;
    double minBitrate;
    double totalSize;
    double duration;
};

class BitrateAnalyzer : public QObject {
    Q_OBJECT

public:
    explicit BitrateAnalyzer(QObject* parent = nullptr);

    void analyze(const QVector<FrameInfo>& frames, double windowSize = 1.0);
    void clear();

    const QVector<BitratePoint>& getBitratePoints() const { return bitratePoints; }
    const BitrateStats& getStats() const { return stats; }

private:
    void calculateStats();

    QVector<BitratePoint> bitratePoints;
    BitrateStats stats;
    double windowSize;
};

#endif // BITRATEANALYZER_H
