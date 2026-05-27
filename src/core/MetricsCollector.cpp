#include "MetricsCollector.h"
#include <algorithm>

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

FrameSizeDistribution MetricsCollector::calculateFrameSizeDistribution(int numBins) const {
    FrameSizeDistribution dist;

    if (frames.isEmpty()) {
        return dist;
    }

    // Find min and max frame sizes
    int minSize = frames.first().size;
    int maxSize = frames.first().size;
    int64_t totalSize = 0;

    for (const auto& frame : frames) {
        minSize = std::min(minSize, frame.size);
        maxSize = std::max(maxSize, frame.size);
        totalSize += frame.size;
    }

    dist.totalFrames = frames.size();
    dist.minSize = minSize;
    dist.maxSize = maxSize;
    dist.avgSize = static_cast<double>(totalSize) / frames.size();

    // Create bins
    int binWidth = std::max(1, (maxSize - minSize + 1) / numBins);
    dist.binEdges.resize(numBins);
    dist.binCounts.resize(numBins);

    for (int i = 0; i < numBins; i++) {
        dist.binEdges[i] = minSize + i * binWidth;
        dist.binCounts[i] = 0;
    }

    // Initialize type distributions
    QVector<int> iFrameCounts(numBins, 0);
    QVector<int> pFrameCounts(numBins, 0);
    QVector<int> bFrameCounts(numBins, 0);

    // Count frames in each bin
    for (const auto& frame : frames) {
        int binIndex = std::min(numBins - 1, (frame.size - minSize) / binWidth);
        dist.binCounts[binIndex]++;

        // Count by type
        if (frame.frameType == 'I') {
            iFrameCounts[binIndex]++;
        } else if (frame.frameType == 'P') {
            pFrameCounts[binIndex]++;
        } else if (frame.frameType == 'B') {
            bFrameCounts[binIndex]++;
        }
    }

    dist.typeDistribution["I"] = iFrameCounts;
    dist.typeDistribution["P"] = pFrameCounts;
    dist.typeDistribution["B"] = bFrameCounts;

    return dist;
}
