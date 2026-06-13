#ifndef COMPLIANCEVALIDATOR_H
#define COMPLIANCEVALIDATOR_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaType>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace VideoStudio {

// Compliance issue severity
enum class IssueSeverity {
    Info,       // Informational
    Warning,    // Non-critical issue
    Error,      // Standard violation
    Critical    // Severe standard violation
};

// Compliance issue category
enum class IssueCategory {
    SPS,            // Sequence Parameter Set
    PPS,            // Picture Parameter Set
    VPS,            // Video Parameter Set (HEVC)
    SliceHeader,    // Slice header
    NALUnit,        // NAL unit structure
    Bitstream,      // General bitstream issues
    Profile,        // Profile/Level compliance
    Timing,         // Timing and timestamp issues
};

// Compliance issue
struct ComplianceIssue {
    IssueSeverity severity;
    IssueCategory category;
    QString description;
    QString standard;       // e.g., "ISO/IEC 14496-10 Section 7.3.2.1.1"
    QString suggestion;     // How to fix
    int frameNumber;        // -1 if not frame-specific
    int64_t byteOffset;     // -1 if not applicable

    ComplianceIssue()
        : severity(IssueSeverity::Info), category(IssueCategory::Bitstream),
          frameNumber(-1), byteOffset(-1) {}

    ComplianceIssue(IssueSeverity sev, IssueCategory cat, const QString& desc,
                    const QString& std = "", const QString& sug = "",
                    int frame = -1, int64_t offset = -1)
        : severity(sev), category(cat), description(desc), standard(std),
          suggestion(sug), frameNumber(frame), byteOffset(offset) {}
};

class ComplianceValidator : public QObject {
    Q_OBJECT

public:
    explicit ComplianceValidator(QObject* parent = nullptr);
    ~ComplianceValidator();

    // Validate a video file
    bool validateFile(const QString& filePath);

    // Get validation results
    const QVector<ComplianceIssue>& getIssues() const { return m_issues; }
    int getErrorCount() const;
    int getWarningCount() const;
    int getInfoCount() const;

    // Export results
    QJsonObject toJson() const;
    QString toTextReport() const;

    // Clear results
    void clear();

signals:
    void validationProgress(int current, int total);
    void validationComplete();

private:
    // H.264/AVC validation
    void validateH264(AVFormatContext* formatCtx, AVCodecContext* codecCtx);
    void validateH264_SPS(const uint8_t* data, int size);
    void validateH264_PPS(const uint8_t* data, int size);
    void validateH264_SliceHeader(const uint8_t* data, int size, int frameNum);
    void validateH264_ProfileLevel(AVCodecContext* codecCtx);
    void validateH264_NALUnits(AVFormatContext* formatCtx, AVCodecContext* codecCtx);

    // H.265/HEVC validation
    void validateH265(AVFormatContext* formatCtx, AVCodecContext* codecCtx);
    void validateH265_VPS(const uint8_t* data, int size);
    void validateH265_SPS(const uint8_t* data, int size);
    void validateH265_PPS(const uint8_t* data, int size);
    void validateH265_ProfileLevel(AVCodecContext* codecCtx);

    // Common validation
    void validateBitstreamStructure(AVFormatContext* formatCtx, AVCodecContext* codecCtx);
    void validateTimestamps(AVFormatContext* formatCtx);
    void validateReferenceFrames(AVCodecContext* codecCtx);

    // Helper functions
    void addIssue(IssueSeverity severity, IssueCategory category,
                  const QString& description, const QString& standard = "",
                  const QString& suggestion = "", int frameNumber = -1,
                  int64_t byteOffset = -1);

    QString severityToString(IssueSeverity severity) const;
    QString categoryToString(IssueCategory category) const;

    // Profile/Level limits for H.264
    struct H264ProfileLimits {
        int maxMacroblocks;
        int maxMacroblocksPerSecond;
        int maxDpbSize;
        int maxBitrate;
        int maxCPBSize;
    };
    H264ProfileLimits getH264LevelLimits(int level) const;

    // Profile/Level limits for H.265
    struct H265ProfileLimits {
        int maxLumaSampleRate;
        int maxLumaPictureSize;
        int maxBitrate;
        int maxCPBSize;
    };
    H265ProfileLimits getH265LevelLimits(int level) const;

    QVector<ComplianceIssue> m_issues;
    QString m_currentFile;
};

} // namespace VideoStudio

Q_DECLARE_METATYPE(VideoStudio::ComplianceIssue)

#endif // COMPLIANCEVALIDATOR_H
