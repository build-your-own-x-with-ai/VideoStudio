#ifndef DUPLICATEFRAMEDETECTOR_H
#define DUPLICATEFRAMEDETECTOR_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QByteArray>
#include <QString>

extern "C" {
#include <libavutil/frame.h>
}

namespace VideoStudio {

class VideoDecoder;

struct DuplicateGroup {
    QByteArray frameHash;
    QVector<int> frameNumbers;
    int occurrences;
    bool isConsecutive;

    DuplicateGroup() : occurrences(0), isConsecutive(false) {}
};

struct DetectionResult {
    int totalFrames;
    int uniqueFrames;
    int duplicateFrames;
    double duplicatePercentage;
    QVector<DuplicateGroup> duplicateGroups;

    int maxConsecutiveDuplicates;
    int duplicateGroupCount;

    DetectionResult()
        : totalFrames(0), uniqueFrames(0), duplicateFrames(0),
          duplicatePercentage(0.0), maxConsecutiveDuplicates(0),
          duplicateGroupCount(0) {}
};

class DuplicateFrameDetector : public QObject {
    Q_OBJECT

public:
    explicit DuplicateFrameDetector(QObject* parent = nullptr);
    ~DuplicateFrameDetector();

    bool analyzeVideo(VideoDecoder* decoder);
    void cancel();
    DetectionResult getResults() const;
    bool isFrameDuplicate(int frameNumber) const;
    QVector<int> getDuplicatesOf(int frameNumber) const;

signals:
    void progressUpdated(int current, int total, const QString& status);
    void analysisCompleted(const DetectionResult& result);

private:
    QByteArray computeFrameHash(AVFrame* frame);
    void processHashMap();

    QMap<QByteArray, QVector<int>> m_hashToFrames;
    QMap<int, QByteArray> m_frameToHash;
    DetectionResult m_result;
    bool m_cancelled;
};

} // namespace VideoStudio

#endif // DUPLICATEFRAMEDETECTOR_H
