#include "MetricsCollector.h"

MetricsCollector::MetricsCollector(QObject* parent)
    : QObject(parent) {
}

void MetricsCollector::addFrame(const FrameInfo& frame) {
    frames.append(frame);
    emit frameAdded(frame);
    emit metricsUpdated();
}

void MetricsCollector::addFrames(const QVector<FrameInfo>& newFrames) {
    frames.append(newFrames);
    emit metricsUpdated();
}

void MetricsCollector::clear() {
    frames.clear();
    emit metricsUpdated();
}

int MetricsCollector::getIFrameCount() const {
    int count = 0;
    for (const auto& frame : frames) {
        if (frame.frameType == 'I') {
            count++;
        }
    }
    return count;
}

int MetricsCollector::getPFrameCount() const {
    int count = 0;
    for (const auto& frame : frames) {
        if (frame.frameType == 'P') {
            count++;
        }
    }
    return count;
}

int MetricsCollector::getBFrameCount() const {
    int count = 0;
    for (const auto& frame : frames) {
        if (frame.frameType == 'B') {
            count++;
        }
    }
    return count;
}

double MetricsCollector::getAverageBitrate() const {
    if (frames.isEmpty()) {
        return 0.0;
    }

    int64_t totalSize = getTotalSize();
    double duration = frames.last().timestamp - frames.first().timestamp;

    if (duration <= 0) {
        return 0.0;
    }

    return (totalSize * 8.0) / duration;
}

int64_t MetricsCollector::getTotalSize() const {
    int64_t total = 0;
    for (const auto& frame : frames) {
        total += frame.size;
    }
    return total;
}
