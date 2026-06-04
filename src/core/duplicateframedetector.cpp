#include "core/duplicateframedetector.h"
#include "core/videodecoder.h"
#include <QCryptographicHash>
#include <QDebug>

namespace VideoStudio {

DuplicateFrameDetector::DuplicateFrameDetector(QObject* parent)
    : QObject(parent)
    , m_cancelled(false)
{
}

DuplicateFrameDetector::~DuplicateFrameDetector() {
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

    qDebug() << "DuplicateFrameDetector: Analyzing" << frameCount << "frames";

    // Save current position
    int originalFrame = decoder->getCurrentFrameNumber();

    // Analyze all frames
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

        // Compute hash
        QByteArray hash = computeFrameHash(frame);
        if (hash.isEmpty()) {
            qDebug() << "DuplicateFrameDetector: Failed to compute hash for frame" << i;
            continue;
        }

        // Store hash
        m_hashToFrames[hash].append(i);
        m_frameToHash[i] = hash;
    }

    // Restore original position
    if (originalFrame >= 0) {
        decoder->seekToFrame(originalFrame);
    }

    // Process hash map to identify duplicates
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

    for (auto it = m_hashToFrames.begin(); it != m_hashToFrames.end(); ++it) {
        const QByteArray& hash = it.key();
        const QVector<int>& frameNumbers = it.value();

        if (frameNumbers.size() == 1) {
            // Unique frame
            m_result.uniqueFrames++;
        } else {
            // Duplicate group
            m_result.duplicateFrames += frameNumbers.size();
            m_result.duplicateGroupCount++;

            DuplicateGroup group;
            group.frameHash = hash;
            group.frameNumbers = frameNumbers;
            group.occurrences = frameNumbers.size();

            // Check if consecutive
            group.isConsecutive = true;
            for (int i = 1; i < frameNumbers.size(); ++i) {
                if (frameNumbers[i] != frameNumbers[i - 1] + 1) {
                    group.isConsecutive = false;
                    break;
                }
            }

            // Track max consecutive duplicates
            if (group.isConsecutive && group.occurrences > m_result.maxConsecutiveDuplicates) {
                m_result.maxConsecutiveDuplicates = group.occurrences;
            }

            m_result.duplicateGroups.append(group);
        }
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
