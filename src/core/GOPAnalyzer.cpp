#include "GOPAnalyzer.h"

GOPAnalyzer::GOPAnalyzer(QObject* parent)
    : QObject(parent) {
    stats = {0, 0.0, 0, 0, 0.0, 0, 0, 0};
}

void GOPAnalyzer::analyze(const QVector<FrameInfo>& frames) {
    gops.clear();

    if (frames.isEmpty()) {
        stats = {0, 0.0, 0, 0, 0.0, 0, 0, 0};
        return;
    }

    GOP currentGOP;
    currentGOP.startFrame = 0;
    currentGOP.frames.clear();

    for (int i = 0; i < frames.size(); ++i) {
        const FrameInfo& frame = frames[i];

        // 如果遇到 I 帧且不是第一帧，则结束当前 GOP
        if (frame.frameType == 'I' && i > 0 && !currentGOP.frames.isEmpty()) {
            currentGOP.endFrame = i - 1;
            currentGOP.size = currentGOP.frames.size();
            gops.append(currentGOP);

            // 开始新的 GOP
            currentGOP.startFrame = i;
            currentGOP.frames.clear();
        }

        currentGOP.frames.append(frame);
    }

    // 添加最后一个 GOP
    if (!currentGOP.frames.isEmpty()) {
        currentGOP.endFrame = frames.size() - 1;
        currentGOP.size = currentGOP.frames.size();
        gops.append(currentGOP);
    }

    calculateStats();
}

void GOPAnalyzer::calculateStats() {
    if (gops.isEmpty()) {
        stats = {0, 0.0, 0, 0, 0.0, 0, 0, 0};
        return;
    }

    int totalSize = 0;
    int maxSize = 0;
    int minSize = gops.first().size;
    int iCount = 0;
    int pCount = 0;
    int bCount = 0;

    for (const auto& gop : gops) {
        totalSize += gop.size;
        maxSize = std::max(maxSize, gop.size);
        minSize = std::min(minSize, gop.size);

        for (const auto& frame : gop.frames) {
            if (frame.frameType == 'I') iCount++;
            else if (frame.frameType == 'P') pCount++;
            else if (frame.frameType == 'B') bCount++;
        }
    }

    stats.totalGOPs = gops.size();
    stats.averageGOPSize = static_cast<double>(totalSize) / gops.size();
    stats.maxGOPSize = maxSize;
    stats.minGOPSize = minSize;
    stats.averageKeyFrameInterval = stats.averageGOPSize;
    stats.iFrameCount = iCount;
    stats.pFrameCount = pCount;
    stats.bFrameCount = bCount;
}

void GOPAnalyzer::clear() {
    gops.clear();
    stats = {0, 0.0, 0, 0, 0.0, 0, 0, 0};
}
