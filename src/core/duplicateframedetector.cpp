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
        return 1.0;  // Return high difference if invalid
    }

    // Ensure frames have same dimensions
    if (frame1->width != frame2->width || frame1->height != frame2->height) {
        return 1.0;  // Return high difference if dimensions don't match
    }

    // Calculate average absolute difference on Y plane only for speed
    uint64_t totalDiff = 0;
    int pixelCount = frame1->width * frame1->height;

    for (int y = 0; y < frame1->height; y++) {
        const uint8_t* row1 = frame1->data[0] + y * frame1->linesize[0];
        const uint8_t* row2 = frame2->data[0] + y * frame2->linesize[0];

        for (int x = 0; x < frame1->width; x++) {
            int diff = static_cast<int>(row1[x]) - static_cast<int>(row2[x]);
            totalDiff += (diff < 0) ? -diff : diff;  // Absolute value
        }
    }

    // Calculate average pixel difference (0-255 range)
    // Normalize to 0.0-1.0 range where:
    // 0.0 = identical frames
    // 1.0 = maximum difference (all pixels differ by 255)
    double avgDiff = static_cast<double>(totalDiff) / pixelCount;
    double normalizedDiff = avgDiff / 255.0;

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

    // Store previous frame's pixel data (Y plane only) for similarity comparison
    QByteArray prevFrameData;
    int prevFrameWidth = 0;
    int prevFrameHeight = 0;

    QByteArray prevHash;
    int groupStartFrame = -1;

    // Analyze all frames and compare with previous frame
    qDebug() << "DuplicateFrameDetector: Starting frame analysis, frameCount=" << frameCount << "threshold=" << m_similarityThreshold;
    for (int i = 0; i < frameCount; ++i) {
        if (i <= 5) {
            qDebug() << "DuplicateFrameDetector: Processing frame" << i;
        }
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

        // Seek to frame (this also decodes the frame internally)
        if (!decoder->seekToFrame(i)) {
            qDebug() << "DuplicateFrameDetector: Failed to seek to frame" << i;
            continue;
        }

        // Get the current frame (already decoded by seekToFrame)
        AVFrame* frame = decoder->getCurrentFrame();
        if (!frame) {
            qDebug() << "DuplicateFrameDetector: Failed to get frame" << i;
            continue;
        }

        bool isDuplicate = false;

        if (i > 0 && !prevFrameData.isEmpty()) {
            if (m_similarityThreshold <= 0.0) {
                // Use exact hash matching for threshold = 0
                QByteArray hash = computeFrameHash(frame);
                isDuplicate = (hash == prevHash);
                if (i <= 5) {
                    qDebug() << "Frame" << i << ": Using hash comparison, isDuplicate=" << isDuplicate;
                }
            } else {
                // Use similarity calculation for threshold > 0
                // Compare current frame's Y plane with saved previous frame data
                if (i <= 5) {
                    qDebug() << "Frame" << i << ": Entering similarity calculation branch";
                }
                if (frame->width == prevFrameWidth && frame->height == prevFrameHeight) {
                    uint64_t totalDiff = 0;
                    int pixelCount = frame->width * frame->height;

                    const uint8_t* prevData = reinterpret_cast<const uint8_t*>(prevFrameData.constData());

                    // Debug: print first pixel values of current and previous frame
                    if (i <= 5) {
                        qDebug() << "Frame" << i << "current: first 10 pixels="
                                 << (int)frame->data[0][0] << (int)frame->data[0][1] << (int)frame->data[0][2]
                                 << (int)frame->data[0][3] << (int)frame->data[0][4] << (int)frame->data[0][5]
                                 << (int)frame->data[0][6] << (int)frame->data[0][7] << (int)frame->data[0][8] << (int)frame->data[0][9];
                        qDebug() << "Frame" << i << "prevData: first 10 pixels="
                                 << (int)prevData[0] << (int)prevData[1] << (int)prevData[2]
                                 << (int)prevData[3] << (int)prevData[4] << (int)prevData[5]
                                 << (int)prevData[6] << (int)prevData[7] << (int)prevData[8] << (int)prevData[9];
                    }

                    for (int y = 0; y < frame->height; y++) {
                        const uint8_t* currentRow = frame->data[0] + y * frame->linesize[0];
                        const uint8_t* prevRow = prevData + y * frame->width;

                        for (int x = 0; x < frame->width; x++) {
                            int diff = static_cast<int>(currentRow[x]) - static_cast<int>(prevRow[x]);
                            totalDiff += (diff < 0) ? -diff : diff;
                        }
                    }

                    double avgDiff = static_cast<double>(totalDiff) / pixelCount;
                    double normalizedDiff = avgDiff / 255.0;
                    isDuplicate = (normalizedDiff <= m_similarityThreshold);

                    // Debug: print first 10 comparisons
                    if (i <= 10) {
                        qDebug() << "Frame" << i << "vs" << (i-1) << ": avgDiff=" << avgDiff
                                 << "normalized=" << normalizedDiff
                                 << "threshold=" << m_similarityThreshold
                                 << "isDuplicate=" << isDuplicate;
                    }
                } else {
                    if (i <= 5) {
                        qDebug() << "Frame" << i << ": Frame dimensions don't match!";
                    }
                }
            }
        } else {
            if (i <= 5) {
                qDebug() << "Frame" << i << ": Skipping comparison (i=" << i << ", prevFrameData.isEmpty=" << prevFrameData.isEmpty() << ")";
            }
        }

        if (isDuplicate) {
            // This frame is duplicate/similar to previous frame
            if (groupStartFrame == -1) {
                // Start new group
                groupStartFrame = i - 1;
                QByteArray groupHash = prevHash;
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

        // Save current frame's Y plane data for next comparison
        prevFrameWidth = frame->width;
        prevFrameHeight = frame->height;
        prevFrameData.resize(frame->width * frame->height);
        uint8_t* destData = reinterpret_cast<uint8_t*>(prevFrameData.data());
        for (int y = 0; y < frame->height; y++) {
            memcpy(destData + y * frame->width,
                   frame->data[0] + y * frame->linesize[0],
                   frame->width);
        }

        // Debug: print first pixel values
        if (i <= 5) {
            const uint8_t* firstPixels = reinterpret_cast<const uint8_t*>(prevFrameData.constData());
            qDebug() << "Frame" << i << "saved: first 10 pixels="
                     << (int)firstPixels[0] << (int)firstPixels[1] << (int)firstPixels[2]
                     << (int)firstPixels[3] << (int)firstPixels[4] << (int)firstPixels[5]
                     << (int)firstPixels[6] << (int)firstPixels[7] << (int)firstPixels[8] << (int)firstPixels[9];
        }
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
