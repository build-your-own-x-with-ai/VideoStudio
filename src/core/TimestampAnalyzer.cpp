#include "TimestampAnalyzer.h"
#include <cmath>
#include <algorithm>

TimestampAnalyzer::TimestampAnalyzer() {
    clear();
}

void TimestampAnalyzer::clear() {
    stats = TimestampStats();
    ptsCurve.clear();
    dtsCurve.clear();
    ptsDtsDiff.clear();
    frameIntervals.clear();
}

void TimestampAnalyzer::analyze(const QVector<FrameInfo>& frames) {
    clear();

    if (frames.isEmpty()) {
        return;
    }

    // Extract PTS and DTS curves
    for (const auto& frame : frames) {
        ptsCurve.append(frame.pts);
        dtsCurve.append(frame.dts);
        ptsDtsDiff.append(frame.pts - frame.dts);
    }

    // Calculate frame intervals (based on PTS)
    for (int i = 1; i < frames.size(); i++) {
        double interval = frames[i].timestamp - frames[i-1].timestamp;
        frameIntervals.append(interval);
    }

    calculateStatistics(frames);
    detectAnomalies(frames);
}

void TimestampAnalyzer::calculateStatistics(const QVector<FrameInfo>& frames) {
    if (frameIntervals.isEmpty()) {
        return;
    }

    // Calculate min, max, avg frame interval
    stats.minFrameInterval = *std::min_element(frameIntervals.begin(), frameIntervals.end());
    stats.maxFrameInterval = *std::max_element(frameIntervals.begin(), frameIntervals.end());

    double sum = 0.0;
    for (double interval : frameIntervals) {
        sum += interval;
    }
    stats.avgFrameInterval = sum / frameIntervals.size();

    // Calculate jitter (standard deviation)
    double variance = 0.0;
    for (double interval : frameIntervals) {
        variance += std::pow(interval - stats.avgFrameInterval, 2);
    }
    variance /= frameIntervals.size();
    stats.jitter = std::sqrt(variance);
}

void TimestampAnalyzer::detectAnomalies(const QVector<FrameInfo>& frames) {
    if (frames.size() < 2) {
        return;
    }

    stats.discontinuities = 0;
    stats.ptsReversals = 0;
    stats.dtsReversals = 0;
    stats.duplicatePTS = 0;
    stats.duplicateDTS = 0;

    // Threshold for discontinuity detection (3x average interval)
    double discontinuityThreshold = stats.avgFrameInterval * 3.0;

    for (int i = 1; i < frames.size(); i++) {
        const FrameInfo& prev = frames[i-1];
        const FrameInfo& curr = frames[i];

        // Check for PTS reversal
        if (curr.pts < prev.pts) {
            stats.ptsReversals++;
            stats.ptsReversalFrames.append(i);
        }

        // Check for DTS reversal
        if (curr.dts < prev.dts) {
            stats.dtsReversals++;
            stats.dtsReversalFrames.append(i);
        }

        // Check for duplicate PTS
        if (curr.pts == prev.pts) {
            stats.duplicatePTS++;
        }

        // Check for duplicate DTS
        if (curr.dts == prev.dts) {
            stats.duplicateDTS++;
        }

        // Check for discontinuity (large gap)
        double interval = curr.timestamp - prev.timestamp;
        if (interval > discontinuityThreshold && discontinuityThreshold > 0) {
            stats.discontinuities++;
            stats.discontinuityFrames.append(i);
        }
    }
}
