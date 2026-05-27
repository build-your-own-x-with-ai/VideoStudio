#ifndef GOPANALYZER_H
#define GOPANALYZER_H

#include <QObject>
#include <QVector>
#include "FrameInfo.h"

struct GOP {
    int startFrame;
    int endFrame;
    int size;
    QVector<FrameInfo> frames;
};

struct GOPStats {
    int totalGOPs;
    double averageGOPSize;
    int maxGOPSize;
    int minGOPSize;
    double averageKeyFrameInterval;
    int iFrameCount;
    int pFrameCount;
    int bFrameCount;
};

class GOPAnalyzer : public QObject {
    Q_OBJECT

public:
    explicit GOPAnalyzer(QObject* parent = nullptr);

    void analyze(const QVector<FrameInfo>& frames);
    void clear();

    const QVector<GOP>& getGOPs() const { return gops; }
    const GOPStats& getStats() const { return stats; }

private:
    void calculateStats();

    QVector<GOP> gops;
    GOPStats stats;
};

#endif // GOPANALYZER_H
