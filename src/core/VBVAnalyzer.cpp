#include "VBVAnalyzer.h"
#include <algorithm>
#include <cmath>

VBVAnalyzer::VBVAnalyzer() {
    clear();
}

void VBVAnalyzer::clear() {
    stats = VBVStats();
    occupancyCurve.clear();
}

void VBVAnalyzer::analyze(const QVector<FrameInfo>& frames,
                          double bufferSize, double bitrate, double initialDelay) {
    clear();

    if (frames.isEmpty() || bufferSize <= 0 || bitrate <= 0) {
        return;
    }

    stats.bufferSize = bufferSize;
    stats.targetBitrate = bitrate;
    stats.initialDelay = initialDelay;

    simulateVBV(frames);
}

void VBVAnalyzer::simulateVBV(const QVector<FrameInfo>& frames) {
    // VBV buffer simulation
    // Buffer fills at constant bitrate and empties when frames are decoded

    double bufferLevel = stats.bufferSize * stats.initialDelay * stats.targetBitrate;
    double minOccupancy = 100.0;
    double maxOccupancy = 0.0;
    double sumOccupancy = 0.0;
    int overflows = 0;
    int underflows = 0;

    for (int i = 0; i < frames.size(); i++) {
        const FrameInfo& frame = frames[i];

        // Calculate time since last frame
        double timeDelta = 0.0;
        if (i > 0) {
            timeDelta = frame.timestamp - frames[i-1].timestamp;
        } else if (frames.size() > 1) {
            timeDelta = frames[1].timestamp - frames[0].timestamp;
        }

        // Buffer fills at constant bitrate during the frame interval
        double bitsAdded = stats.targetBitrate * timeDelta;
        bufferLevel += bitsAdded;

        // Check for overflow before removing frame
        bool overflow = false;
        if (bufferLevel > stats.bufferSize) {
            overflow = true;
            overflows++;
            bufferLevel = stats.bufferSize;  // Clamp to buffer size
        }

        // Remove frame bits from buffer (frame is decoded)
        double frameBits = frame.size * 8.0;  // Convert bytes to bits
        bufferLevel -= frameBits;

        // Check for underflow after removing frame
        bool underflow = false;
        if (bufferLevel < 0) {
            underflow = true;
            underflows++;
            bufferLevel = 0;  // Clamp to zero
        }

        // Calculate occupancy percentage
        double occupancy = (bufferLevel / stats.bufferSize) * 100.0;

        // Update statistics
        minOccupancy = std::min(minOccupancy, occupancy);
        maxOccupancy = std::max(maxOccupancy, occupancy);
        sumOccupancy += occupancy;

        // Store point
        VBVPoint point;
        point.timestamp = frame.timestamp;
        point.occupancy = occupancy;
        point.frameNumber = frame.frameNumber;
        point.isOverflow = overflow;
        point.isUnderflow = underflow;
        occupancyCurve.append(point);
    }

    // Finalize statistics
    stats.minOccupancy = minOccupancy;
    stats.maxOccupancy = maxOccupancy;
    stats.avgOccupancy = occupancyCurve.isEmpty() ? 0.0 : sumOccupancy / occupancyCurve.size();
    stats.overflows = overflows;
    stats.underflows = underflows;
}
