#include "BitrateAnalyzer.h"
#include <algorithm>

BitrateAnalyzer::BitrateAnalyzer(QObject* parent)
    : QObject(parent), windowSize(1.0) {
    stats = {0.0, 0.0, 0.0, 0.0, 0.0};
}

void BitrateAnalyzer::analyze(const QVector<FrameInfo>& frames, double windowSize) {
    this->windowSize = windowSize;
    bitratePoints.clear();

    if (frames.isEmpty()) {
        stats = {0.0, 0.0, 0.0, 0.0, 0.0};
        return;
    }

    double startTime = frames.first().timestamp;
    double endTime = frames.last().timestamp;
    stats.duration = endTime - startTime;

    if (stats.duration <= 0) {
        stats = {0.0, 0.0, 0.0, 0.0, 0.0};
        return;
    }

    double currentTime = startTime;
    int frameIndex = 0;

    while (currentTime <= endTime) {
        double windowEnd = currentTime + windowSize;
        int64_t windowSize = 0;

        while (frameIndex < frames.size() &&
               frames[frameIndex].timestamp < windowEnd) {
            windowSize += frames[frameIndex].size;
            frameIndex++;
        }

        double bitrate = (windowSize * 8.0) / this->windowSize;

        BitratePoint point;
        point.timestamp = currentTime;
        point.bitrate = bitrate;
        bitratePoints.append(point);

        currentTime += this->windowSize;
    }

    calculateStats();
}

void BitrateAnalyzer::calculateStats() {
    if (bitratePoints.isEmpty()) {
        stats = {0.0, 0.0, 0.0, 0.0, stats.duration};
        return;
    }

    double sum = 0.0;
    double peak = 0.0;
    double min = bitratePoints.first().bitrate;

    for (const auto& point : bitratePoints) {
        sum += point.bitrate;
        peak = std::max(peak, point.bitrate);
        min = std::min(min, point.bitrate);
    }

    stats.averageBitrate = sum / bitratePoints.size();
    stats.peakBitrate = peak;
    stats.minBitrate = min;
}

void BitrateAnalyzer::clear() {
    bitratePoints.clear();
    stats = {0.0, 0.0, 0.0, 0.0, 0.0};
}
