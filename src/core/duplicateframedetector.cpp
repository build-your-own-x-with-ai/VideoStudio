#include "core/duplicateframedetector.h"
#include "core/videodecoder.h"
#include <QCryptographicHash>
#include <QDebug>

namespace VideoStudio {

DuplicateFrameDetector::DuplicateFrameDetector(QObject* parent)
    : QObject(parent)
    , m_cancelled(false)
    , m_similarityThreshold(0.0)  // Default: exact match only
{
}

DuplicateFrameDetector::~DuplicateFrameDetector() {
}

void DuplicateFrameDetector::setSimilarityThreshold(double threshold) {
    m_similarityThreshold = qBound(0.0, threshold, 1.0);
}

double DuplicateFrameDetector::getSimilarityThreshold() const {
    return m_similarityThreshold;
}

QByteArray DuplicateFrameDetector::computeFrameHash(AVFrame* frame) {
    if (!frame || !frame->data[0]) {
        return QByteArray();
    }

    QCryptographicHash hash(QCryptographicHash::Md5);

    // Hash Y plane
    for (int y = 0; y < frame->height; y++) {
        hash.addData(QByteArrayView(reinterpret_cast<const char*>(frame->data[0] + y * frame->linesize[0]), frame->width));
    }

    // Hash U plane (4:2:0)
    int chromaHeight = frame->height / 2;
    int chromaWidth = frame->width / 2;
    for (int y = 0; y < chromaHeight; y++) {
        hash.addData(QByteArrayView(reinterpret_cast<const char*>(frame->data[1] + y * frame->linesize[1]), chromaWidth));
    }

    // Hash V plane (4:2:0)
    for (int y = 0; y < chromaHeight; y++) {
        hash.addData(QByteArrayView(reinterpret_cast<const char*>(frame->data[2] + y * frame->linesize[2]), chromaWidth));
    }

    return hash.result();
}

double DuplicateFrameDetector::computeFrameSimilarity(AVFrame* frame1, AVFrame* frame2) {
    if (!frame1 || !frame2 || !frame1->data[0] || !frame2->data[0]) {
        return 0.0;
    }

    // Ensure frames have same dimensions
    if (frame1->width != frame2->width || frame1->height != frame2->height) {
        return 0.0;
    }

    // Calculate MSE (Mean Squared Error) on Y plane only for speed
    uint64_t totalDiff = 0;
    int pixelCount = frame1->width * frame1->height;

    for (int y = 0; y < frame1->height; y++) {
        const uint8_t* row1 = frame1->data[0] + y * frame1->linesize[0];
        const uint8_t* row2 = frame2->data[0] + y * frame2->linesize[0];

        for (int x = 0; x < frame1->width; x++) {
            int diff = row1[x] - row2[x];
            totalDiff += diff * diff;
        }
    }

    // Calculate normalized difference (0.0 = identical, 1.0 = maximum difference)
    // Maximum possible MSE for 8-bit data is 255^2 = 65025
    double mse = static_cast<double>(totalDiff) / pixelCount;
    double normalizedDiff = mse / 65025.0;

    return normalizedDiff;
}

bool DuplicateFrameDetector::analyzeVideo(VideoDecoder* decoder) {
    if (!decoder || !decoder->isOpen()) {
        qDebug() << "DuplicateFrameDetector: Invalid decoder";
        return false;
    }

    m_cancelled = false;
    m_hashToFrames.clear();
    m_frameToHash.clear();
    m_result = DetectionResult();

    int frameCount = decoder->getFrameCount();
    m_result.totalFrames = frameCount;

    qDebug() << "DuplicateFrameDetector: Analyzing consecutive frames for" << frameCount << "frames";
    qDebug() << "DuplicateFrameDetector: Similarity threshold:" << m_similarityThreshold;

    // Save current position
    int originalFrame = decoder->getCurrentFrameNumber();

    AVFrame* prevFrame = nullptr;
    QByteArray prevHash;
    int groupStartFrame = -1;

    // Analyze all frames and compare with previous frame
    for (int i = 0; i < frameCount; ++i) {
        if (m_cancelled) {
            qDebug() << "DuplicateFrameDetector: Analysis cancelled";
            // Restore original position
            if (originalFrame >= 0) {
                decoder->seekToFrame(originalFrame);
            }
            return false;
        }

        // Emit progress every 30 frames
        if (i % 30 == 0 || i == frameCount - 1) {
            QString status = QString("Analyzing frame %1 of %2...").arg(i + 1).arg(frameCount);
            emit progressUpdated(i + 1, frameCount, status);
        }

        // Seek to frame
        if (!decoder->seekToFrame(i)) {
            qDebug() << "DuplicateFrameDetector: Failed to seek to frame" << i;
            continue;
        }

        // Decode frame
        AVFrame* frame = decoder->decodeNextFrame();
        if (!frame) {
            qDebug() << "DuplicateFrameDetector: Failed to decode frame" << i;
            continue;
        }

        bool isDuplicate = false;

        if (i > 0 && prevFrame) {
            if (m_similarityThreshold <= 0.0) {
                // Use exact hash matching for threshold = 0
                QByteArray hash = computeFrameHash(frame);
                isDuplicate = (hash == prevHash);
            } else {
                // Use similarity calculation for threshold > 0
                double diff = computeFrameSimilarity(prevFrame, frame);
                isDuplicate = (diff <= m_similarityThreshold);
            }
        }

        if (isDuplicate) {
            // This frame is duplicate/similar to previous frame
            if (groupStartFrame == -1) {
                // Start new group
                groupStartFrame = i - 1;
                QByteArray groupHash = computeFrameHash(prevFrame);
                m_hashToFrames[groupHash].append(i - 1);
                m_hashToFrames[groupHash].append(i);
            } else {
                // Extend existing group
                QByteArray groupHash = m_frameToHash[groupStartFrame];
                m_hashToFrames[groupHash].append(i);
            }
            m_frameToHash[i] = m_frameToHash[groupStartFrame];
        } else {
            // Not a duplicate, end current group if any
            groupStartFrame = -1;
            // Compute and store hash for current frame
            QByteArray hash = computeFrameHash(frame);
            m_frameToHash[i] = hash;
            prevHash = hash;
        }

        prevFrame = frame;
    }

    // Restore original position
    if (originalFrame >= 0) {
        decoder->seekToFrame(originalFrame);
    }

    // Process hash map to identify consecutive duplicate groups
    processHashMap();

    qDebug() << "DuplicateFrameDetector: Analysis complete";
    qDebug() << "  Total frames:" << m_result.totalFrames;
    qDebug() << "  Unique frames:" << m_result.uniqueFrames;
    qDebug() << "  Duplicate frames:" << m_result.duplicateFrames;
    qDebug() << "  Duplicate percentage:" << m_result.duplicatePercentage << "%";
    qDebug() << "  Duplicate groups:" << m_result.duplicateGroupCount;

    emit analysisCompleted(m_result);
    return true;
}

void DuplicateFrameDetector::processHashMap() {
    m_result.duplicateGroups.clear();
    m_result.uniqueFrames = 0;
    m_result.duplicateFrames = 0;
    m_result.maxConsecutiveDuplicates = 0;
    m_result.duplicateGroupCount = 0;

    // Count unique frames (frames not in any duplicate group)
    QSet<int> duplicateFrameSet;
    for (auto it = m_hashToFrames.begin(); it != m_hashToFrames.end(); ++it) {
        const QVector<int>& frameNumbers = it.value();
        for (int frameNum : frameNumbers) {
            duplicateFrameSet.insert(frameNum);
        }
    }

    m_result.duplicateFrames = duplicateFrameSet.size();
    m_result.uniqueFrames = m_result.totalFrames - m_result.duplicateFrames;

    // Process each duplicate group (all are consecutive by design)
    for (auto it = m_hashToFrames.begin(); it != m_hashToFrames.end(); ++it) {
        const QByteArray& hash = it.key();
        const QVector<int>& frameNumbers = it.value();

        if (frameNumbers.size() < 2) {
            continue; // Should not happen with new logic
        }

        m_result.duplicateGroupCount++;

        DuplicateGroup group;
        group.frameHash = hash;
        group.frameNumbers = frameNumbers;
        group.occurrences = frameNumbers.size();
        group.isConsecutive = true; // Always true for consecutive frame comparison

        // Track max consecutive duplicates
        if (group.occurrences > m_result.maxConsecutiveDuplicates) {
            m_result.maxConsecutiveDuplicates = group.occurrences;
        }

        m_result.duplicateGroups.append(group);
    }

    // Sort groups by occurrence count (descending)
    std::sort(m_result.duplicateGroups.begin(), m_result.duplicateGroups.end(),
              [](const DuplicateGroup& a, const DuplicateGroup& b) {
                  return a.occurrences > b.occurrences;
              });

    // Calculate percentage
    if (m_result.totalFrames > 0) {
        m_result.duplicatePercentage = (m_result.duplicateFrames * 100.0) / m_result.totalFrames;
    }
}

void DuplicateFrameDetector::cancel() {
    m_cancelled = true;
}

DetectionResult DuplicateFrameDetector::getResults() const {
    return m_result;
}

bool DuplicateFrameDetector::isFrameDuplicate(int frameNumber) const {
    if (!m_frameToHash.contains(frameNumber)) {
        return false;
    }

    const QByteArray& hash = m_frameToHash[frameNumber];
    return m_hashToFrames[hash].size() > 1;
}

QVector<int> DuplicateFrameDetector::getDuplicatesOf(int frameNumber) const {
    if (!m_frameToHash.contains(frameNumber)) {
        return QVector<int>();
    }

    const QByteArray& hash = m_frameToHash[frameNumber];
    return m_hashToFrames[hash];
}

} // namespace VideoStudio
