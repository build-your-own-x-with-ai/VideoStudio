#ifndef METRICSCOLLECTOR_H
#define METRICSCOLLECTOR_H

#include <QObject>
#include <QVector>
#include "FrameInfo.h"

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

signals:
    void frameAdded(const FrameInfo& frame);
    void metricsUpdated();

private:
    QVector<FrameInfo> frames;
};

#endif // METRICSCOLLECTOR_H
