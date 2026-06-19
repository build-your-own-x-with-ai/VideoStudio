#include "compliancevalidator.h"
#include <QDebug>
#include <QFile>

extern "C" {
#include <libavutil/opt.h>
}

namespace VideoStudio {

ComplianceValidator::ComplianceValidator(QObject* parent)
    : QObject(parent)
{
}

ComplianceValidator::~ComplianceValidator() = default;

bool ComplianceValidator::validateFile(const QString& filePath) {
    clear();
    m_currentFile = filePath;

    AVFormatContext* formatCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;

    // Open input file
    int ret = avformat_open_input(&formatCtx, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) {
        addIssue(IssueSeverity::Critical, IssueCategory::Bitstream,
                 "Failed to open file", "", "Check file path and format");
        return false;
    }

    // Find stream info
    ret = avformat_find_stream_info(formatCtx, nullptr);
    if (ret < 0) {
        avformat_close_input(&formatCtx);
        addIssue(IssueSeverity::Critical, IssueCategory::Bitstream,
                 "Failed to find stream information");
        return false;
    }

    // Find video stream
    int videoStreamIdx = -1;
    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIdx = i;
            break;
        }
    }

    if (videoStreamIdx == -1) {
        avformat_close_input(&formatCtx);
        addIssue(IssueSeverity::Critical, IssueCategory::Bitstream,
                 "No video stream found in file");
        return false;
    }

    // Get codec context
    AVCodecParameters* codecPar = formatCtx->streams[videoStreamIdx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        avformat_close_input(&formatCtx);
        addIssue(IssueSeverity::Critical, IssueCategory::Bitstream,
                 "Unsupported codec");
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);
    avcodec_open2(codecCtx, codec, nullptr);

    // Validate based on codec type
    if (codecPar->codec_id == AV_CODEC_ID_H264) {
        validateH264(formatCtx, codecCtx);
    } else if (codecPar->codec_id == AV_CODEC_ID_HEVC) {
        validateH265(formatCtx, codecCtx);
    } else {
        addIssue(IssueSeverity::Info, IssueCategory::Bitstream,
                 QString("Codec %1 is not H.264 or H.265, limited validation")
                     .arg(codec->name));
    }

    // Common validation (applies to all codecs)
    validateBitstreamStructure(formatCtx, codecCtx);
    validateTimestamps(formatCtx);

    // Cleanup
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);

    emit validationComplete();
    return true;
}

void ComplianceValidator::validateH264(AVFormatContext* formatCtx, AVCodecContext* codecCtx) {
    qDebug() << "Validating H.264/AVC compliance";

    // Validate profile and level
    validateH264_ProfileLevel(codecCtx);

    // Validate extradata (contains SPS/PPS)
    if (codecCtx->extradata && codecCtx->extradata_size > 0) {
        // Parse SPS/PPS from extradata
        const uint8_t* extradata = codecCtx->extradata;
        int size = codecCtx->extradata_size;

        // Check for AVC configuration record (avCC)
        if (size >= 7 && extradata[0] == 1) {
            // avCC format
            int spsCount = extradata[5] & 0x1F;
            int offset = 6;

            for (int i = 0; i < spsCount && offset + 2 < size; i++) {
                int spsSize = (extradata[offset] << 8) | extradata[offset + 1];
                offset += 2;
                if (offset + spsSize <= size) {
                    validateH264_SPS(extradata + offset, spsSize);
                }
                offset += spsSize;
            }

            if (offset + 1 < size) {
                int ppsCount = extradata[offset];
                offset++;
                for (int i = 0; i < ppsCount && offset + 2 < size; i++) {
                    int ppsSize = (extradata[offset] << 8) | extradata[offset + 1];
                    offset += 2;
                    if (offset + ppsSize <= size) {
                        validateH264_PPS(extradata + offset, ppsSize);
                    }
                    offset += ppsSize;
                }
            }
        }
    }

    // Validate NAL units
    validateH264_NALUnits(formatCtx, codecCtx);
}

void ComplianceValidator::validateH264_ProfileLevel(AVCodecContext* codecCtx) {
    int profile = codecCtx->profile;
    int level = codecCtx->level;

    // Check if profile is valid
    bool validProfile = false;
    QString profileName;

    switch (profile) {
        case FF_PROFILE_H264_BASELINE:
            validProfile = true;
            profileName = "Baseline";
            break;
        case FF_PROFILE_H264_MAIN:
            validProfile = true;
            profileName = "Main";
            break;
        case FF_PROFILE_H264_HIGH:
            validProfile = true;
            profileName = "High";
            break;
        case FF_PROFILE_H264_HIGH_10:
            validProfile = true;
            profileName = "High 10";
            break;
        case FF_PROFILE_H264_HIGH_422:
            validProfile = true;
            profileName = "High 4:2:2";
            break;
        case FF_PROFILE_H264_HIGH_444:
            validProfile = true;
            profileName = "High 4:4:4";
            break;
        default:
            addIssue(IssueSeverity::Warning, IssueCategory::Profile,
                     QString("Unknown H.264 profile: %1").arg(profile),
                     "ISO/IEC 14496-10 Annex A");
            return;
    }

    addIssue(IssueSeverity::Info, IssueCategory::Profile,
             QString("H.264 Profile: %1, Level: %2").arg(profileName).arg(level / 10.0, 0, 'f', 1));

    // Check level limits
    H264ProfileLimits limits = getH264LevelLimits(level);

    // Validate resolution against level
    int macroblocks = ((codecCtx->width + 15) / 16) * ((codecCtx->height + 15) / 16);
    if (macroblocks > limits.maxMacroblocks) {
        addIssue(IssueSeverity::Error, IssueCategory::Profile,
                 QString("Resolution (%1x%2, %3 MBs) exceeds level %4 limit of %5 MBs")
                     .arg(codecCtx->width).arg(codecCtx->height)
                     .arg(macroblocks).arg(level / 10.0, 0, 'f', 1)
                     .arg(limits.maxMacroblocks),
                 "ISO/IEC 14496-10 Annex A",
                 QString("Use Level %1 or higher").arg((level + 10) / 10.0, 0, 'f', 1));
    }

    // Validate frame rate
    if (codecCtx->framerate.num > 0 && codecCtx->framerate.den > 0) {
        double fps = (double)codecCtx->framerate.num / codecCtx->framerate.den;
        int mbsPerSecond = macroblocks * fps;

        if (mbsPerSecond > limits.maxMacroblocksPerSecond) {
            addIssue(IssueSeverity::Error, IssueCategory::Profile,
                     QString("Macroblocks per second (%1) exceeds level %2 limit of %3")
                         .arg(mbsPerSecond).arg(level / 10.0, 0, 'f', 1)
                         .arg(limits.maxMacroblocksPerSecond),
                     "ISO/IEC 14496-10 Annex A");
        }
    }

    // Validate bitrate
    if (codecCtx->bit_rate > 0 && codecCtx->bit_rate > limits.maxBitrate) {
        addIssue(IssueSeverity::Warning, IssueCategory::Profile,
                 QString("Bitrate (%1 kbps) exceeds level %2 limit of %3 kbps")
                     .arg(codecCtx->bit_rate / 1000)
                     .arg(level / 10.0, 0, 'f', 1)
                     .arg(limits.maxBitrate / 1000),
                 "ISO/IEC 14496-10 Annex A");
    }
}

void ComplianceValidator::validateH264_SPS(const uint8_t* data, int size) {
    if (size < 4) {
        addIssue(IssueSeverity::Error, IssueCategory::SPS,
                 "SPS too short", "ISO/IEC 14496-10 Section 7.3.2.1.1");
        return;
    }

    // Basic SPS validation
    addIssue(IssueSeverity::Info, IssueCategory::SPS,
             QString("SPS found, size: %1 bytes").arg(size));

    // TODO: Deep SPS parsing with bitstream reader
}

void ComplianceValidator::validateH264_PPS(const uint8_t* data, int size) {
    if (size < 2) {
        addIssue(IssueSeverity::Error, IssueCategory::PPS,
                 "PPS too short", "ISO/IEC 14496-10 Section 7.3.2.2");
        return;
    }

    addIssue(IssueSeverity::Info, IssueCategory::PPS,
             QString("PPS found, size: %1 bytes").arg(size));

    // TODO: Deep PPS parsing
}

void ComplianceValidator::validateH264_NALUnits(AVFormatContext* formatCtx, AVCodecContext* codecCtx) {
    // Count NAL unit types
    int idrCount = 0, nonIDRCount = 0, spsCount = 0, ppsCount = 0;

    AVPacket* packet = av_packet_alloc();
    int frameCount = 0;

    while (av_read_frame(formatCtx, packet) >= 0 && frameCount < 100) {
        if (packet->stream_index == 0) { // Video stream
            // Parse NAL units in packet
            const uint8_t* data = packet->data;
            int size = packet->size;

            // Look for NAL unit start codes
            for (int i = 0; i < size - 4; i++) {
                if (data[i] == 0 && data[i+1] == 0 && data[i+2] == 0 && data[i+3] == 1) {
                    if (i + 5 < size) {
                        uint8_t nalType = data[i+4] & 0x1F;
                        switch (nalType) {
                            case 5: idrCount++; break;     // IDR
                            case 1: nonIDRCount++; break;  // Non-IDR
                            case 7: spsCount++; break;     // SPS
                            case 8: ppsCount++; break;     // PPS
                        }
                    }
                }
            }

            frameCount++;
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_seek_frame(formatCtx, 0, 0, AVSEEK_FLAG_BACKWARD);

    addIssue(IssueSeverity::Info, IssueCategory::NALUnit,
             QString("NAL units in first 100 frames: IDR=%1, Non-IDR=%2, SPS=%3, PPS=%4")
                 .arg(idrCount).arg(nonIDRCount).arg(spsCount).arg(ppsCount));

    if (idrCount == 0) {
        addIssue(IssueSeverity::Warning, IssueCategory::NALUnit,
                 "No IDR frames found in first 100 frames",
                 "", "Consider adding periodic IDR frames for seeking");
    }
}

void ComplianceValidator::validateH265(AVFormatContext* formatCtx, AVCodecContext* codecCtx) {
    qDebug() << "Validating H.265/HEVC compliance";

    validateH265_ProfileLevel(codecCtx);

    addIssue(IssueSeverity::Info, IssueCategory::Bitstream,
             "H.265/HEVC detailed validation not yet implemented");
}

void ComplianceValidator::validateH265_ProfileLevel(AVCodecContext* codecCtx) {
    int profile = codecCtx->profile;
    int level = codecCtx->level;

    QString profileName;
    // Use if-else instead of switch for older FFmpeg versions where these aren't compile-time constants
    if (profile == FF_PROFILE_HEVC_MAIN) {
        profileName = "Main";
#ifdef FF_PROFILE_HEVC_MAIN_10
    } else if (profile == FF_PROFILE_HEVC_MAIN_10) {
        profileName = "Main 10";
#endif
#ifdef FF_PROFILE_HEVC_MAIN_STILL_PICTURE
    } else if (profile == FF_PROFILE_HEVC_MAIN_STILL_PICTURE) {
        profileName = "Main Still Picture";
#endif
    } else {
        profileName = QString("Unknown (%1)").arg(profile);
    }

    addIssue(IssueSeverity::Info, IssueCategory::Profile,
             QString("H.265 Profile: %1, Level: %2").arg(profileName).arg(level / 30.0, 0, 'f', 1));
}

// Continuation from previous chunk...

void ComplianceValidator::validateH265_VPS(const uint8_t* data, int size) {
    addIssue(IssueSeverity::Info, IssueCategory::VPS,
             QString("VPS found, size: %1 bytes").arg(size));
}

void ComplianceValidator::validateH265_SPS(const uint8_t* data, int size) {
    addIssue(IssueSeverity::Info, IssueCategory::SPS,
             QString("H.265 SPS found, size: %1 bytes").arg(size));
}

void ComplianceValidator::validateH265_PPS(const uint8_t* data, int size) {
    addIssue(IssueSeverity::Info, IssueCategory::PPS,
             QString("H.265 PPS found, size: %1 bytes").arg(size));
}

void ComplianceValidator::validateBitstreamStructure(AVFormatContext* formatCtx, AVCodecContext* codecCtx) {
    // Check for consistent frame sizes
    AVStream* stream = formatCtx->streams[0];

    // Validate container format
    if (formatCtx->iformat) {
        addIssue(IssueSeverity::Info, IssueCategory::Bitstream,
                 QString("Container format: %1").arg(formatCtx->iformat->name));
    }

    // Check if video has B-frames
    if (codecCtx->has_b_frames > 0) {
        addIssue(IssueSeverity::Info, IssueCategory::Bitstream,
                 QString("Video has B-frames (max_b_frames: %1)").arg(codecCtx->has_b_frames));
    }

    // Check reference frames
    if (codecCtx->refs > 0) {
        addIssue(IssueSeverity::Info, IssueCategory::Bitstream,
                 QString("Reference frames: %1").arg(codecCtx->refs));

        if (codecCtx->refs > 16) {
            addIssue(IssueSeverity::Warning, IssueCategory::Bitstream,
                     QString("Reference frame count (%1) is very high, may cause decoding issues")
                         .arg(codecCtx->refs),
                     "", "Consider reducing to 16 or less");
        }
    }
}

void ComplianceValidator::validateTimestamps(AVFormatContext* formatCtx) {
    AVPacket* packet = av_packet_alloc();
    int64_t lastPts = AV_NOPTS_VALUE;
    int64_t lastDts = AV_NOPTS_VALUE;
    int ptsIssues = 0;
    int dtsIssues = 0;
    int frameCount = 0;

    while (av_read_frame(formatCtx, packet) >= 0 && frameCount < 100) {
        if (packet->stream_index == 0) {
            // Check PTS
            if (packet->pts != AV_NOPTS_VALUE) {
                if (lastPts != AV_NOPTS_VALUE && packet->pts < lastPts) {
                    ptsIssues++;
                }
                lastPts = packet->pts;
            }

            // Check DTS
            if (packet->dts != AV_NOPTS_VALUE) {
                if (lastDts != AV_NOPTS_VALUE && packet->dts < lastDts) {
                    dtsIssues++;
                }
                lastDts = packet->dts;
            }

            // Check PTS >= DTS
            if (packet->pts != AV_NOPTS_VALUE && packet->dts != AV_NOPTS_VALUE) {
                if (packet->pts < packet->dts) {
                    addIssue(IssueSeverity::Error, IssueCategory::Timing,
                             QString("Frame %1: PTS (%2) < DTS (%3)")
                                 .arg(frameCount).arg(packet->pts).arg(packet->dts),
                             "ISO/IEC 14496-12",
                             "Ensure PTS >= DTS for all frames",
                             frameCount);
                }
            }

            frameCount++;
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_seek_frame(formatCtx, 0, 0, AVSEEK_FLAG_BACKWARD);

    if (ptsIssues > 0) {
        addIssue(IssueSeverity::Warning, IssueCategory::Timing,
                 QString("Found %1 PTS discontinuities in first 100 frames").arg(ptsIssues),
                 "", "Check timestamp generation");
    }

    if (dtsIssues > 0) {
        addIssue(IssueSeverity::Warning, IssueCategory::Timing,
                 QString("Found %1 DTS discontinuities in first 100 frames").arg(dtsIssues),
                 "", "Check decoding timestamp generation");
    }

    if (ptsIssues == 0 && dtsIssues == 0) {
        addIssue(IssueSeverity::Info, IssueCategory::Timing,
                 "Timestamps are monotonically increasing (checked first 100 frames)");
    }
}

void ComplianceValidator::validateReferenceFrames(AVCodecContext* codecCtx) {
    // Placeholder for reference frame validation
}

void ComplianceValidator::addIssue(IssueSeverity severity, IssueCategory category,
                                    const QString& description, const QString& standard,
                                    const QString& suggestion, int frameNumber,
                                    int64_t byteOffset) {
    m_issues.append(ComplianceIssue(severity, category, description, standard,
                                     suggestion, frameNumber, byteOffset));
}

int ComplianceValidator::getErrorCount() const {
    int count = 0;
    for (const auto& issue : m_issues) {
        if (issue.severity == IssueSeverity::Error || issue.severity == IssueSeverity::Critical) {
            count++;
        }
    }
    return count;
}

int ComplianceValidator::getWarningCount() const {
    int count = 0;
    for (const auto& issue : m_issues) {
        if (issue.severity == IssueSeverity::Warning) {
            count++;
        }
    }
    return count;
}

int ComplianceValidator::getInfoCount() const {
    int count = 0;
    for (const auto& issue : m_issues) {
        if (issue.severity == IssueSeverity::Info) {
            count++;
        }
    }
    return count;
}

QString ComplianceValidator::severityToString(IssueSeverity severity) const {
    switch (severity) {
        case IssueSeverity::Info: return "INFO";
        case IssueSeverity::Warning: return "WARNING";
        case IssueSeverity::Error: return "ERROR";
        case IssueSeverity::Critical: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

QString ComplianceValidator::categoryToString(IssueCategory category) const {
    switch (category) {
        case IssueCategory::SPS: return "SPS";
        case IssueCategory::PPS: return "PPS";
        case IssueCategory::VPS: return "VPS";
        case IssueCategory::SliceHeader: return "Slice Header";
        case IssueCategory::NALUnit: return "NAL Unit";
        case IssueCategory::Bitstream: return "Bitstream";
        case IssueCategory::Profile: return "Profile/Level";
        case IssueCategory::Timing: return "Timing";
        default: return "Unknown";
    }
}

QJsonObject ComplianceValidator::toJson() const {
    QJsonObject root;
    root["file"] = m_currentFile;
    root["total_issues"] = m_issues.size();
    root["errors"] = getErrorCount();
    root["warnings"] = getWarningCount();
    root["info"] = getInfoCount();

    QJsonArray issuesArray;
    for (const auto& issue : m_issues) {
        QJsonObject issueObj;
        issueObj["severity"] = severityToString(issue.severity);
        issueObj["category"] = categoryToString(issue.category);
        issueObj["description"] = issue.description;
        if (!issue.standard.isEmpty()) {
            issueObj["standard"] = issue.standard;
        }
        if (!issue.suggestion.isEmpty()) {
            issueObj["suggestion"] = issue.suggestion;
        }
        if (issue.frameNumber >= 0) {
            issueObj["frame"] = issue.frameNumber;
        }
        if (issue.byteOffset >= 0) {
            issueObj["byte_offset"] = QString::number(issue.byteOffset);
        }
        issuesArray.append(issueObj);
    }
    root["issues"] = issuesArray;

    return root;
}

QString ComplianceValidator::toTextReport() const {
    QString report;
    QTextStream stream(&report);

    stream << "===========================================\n";
    stream << "  H.264/H.265 Compliance Validation Report\n";
    stream << "===========================================\n\n";
    stream << "File: " << m_currentFile << "\n";
    stream << "Total Issues: " << m_issues.size() << "\n";
    stream << "  Errors:   " << getErrorCount() << "\n";
    stream << "  Warnings: " << getWarningCount() << "\n";
    stream << "  Info:     " << getInfoCount() << "\n\n";

    stream << "-------------------------------------------\n";
    stream << "Issues:\n";
    stream << "-------------------------------------------\n\n";

    for (const auto& issue : m_issues) {
        stream << "[" << severityToString(issue.severity) << "] ";
        stream << categoryToString(issue.category) << ": ";
        stream << issue.description << "\n";

        if (!issue.standard.isEmpty()) {
            stream << "  Standard: " << issue.standard << "\n";
        }
        if (!issue.suggestion.isEmpty()) {
            stream << "  Suggestion: " << issue.suggestion << "\n";
        }
        if (issue.frameNumber >= 0) {
            stream << "  Frame: " << issue.frameNumber << "\n";
        }
        stream << "\n";
    }

    return report;
}

void ComplianceValidator::clear() {
    m_issues.clear();
    m_currentFile.clear();
}

ComplianceValidator::H264ProfileLimits ComplianceValidator::getH264LevelLimits(int level) const {
    H264ProfileLimits limits;

    // ISO/IEC 14496-10 Annex A Table A-1
    switch (level) {
        case 10: // Level 1
            limits.maxMacroblocks = 99;
            limits.maxMacroblocksPerSecond = 1485;
            limits.maxDpbSize = 396;
            limits.maxBitrate = 64000;
            limits.maxCPBSize = 175000;
            break;
        case 30: // Level 3
            limits.maxMacroblocks = 1620;
            limits.maxMacroblocksPerSecond = 40500;
            limits.maxDpbSize = 8100;
            limits.maxBitrate = 10000000;
            limits.maxCPBSize = 10000000;
            break;
        case 40: // Level 4
            limits.maxMacroblocks = 8192;
            limits.maxMacroblocksPerSecond = 245760;
            limits.maxDpbSize = 32768;
            limits.maxBitrate = 20000000;
            limits.maxCPBSize = 25000000;
            break;
        case 41: // Level 4.1
            limits.maxMacroblocks = 8192;
            limits.maxMacroblocksPerSecond = 245760;
            limits.maxDpbSize = 32768;
            limits.maxBitrate = 50000000;
            limits.maxCPBSize = 62500000;
            break;
        case 50: // Level 5
            limits.maxMacroblocks = 22080;
            limits.maxMacroblocksPerSecond = 589824;
            limits.maxDpbSize = 110400;
            limits.maxBitrate = 135000000;
            limits.maxCPBSize = 135000000;
            break;
        case 51: // Level 5.1
            limits.maxMacroblocks = 36864;
            limits.maxMacroblocksPerSecond = 983040;
            limits.maxDpbSize = 184320;
            limits.maxBitrate = 240000000;
            limits.maxCPBSize = 240000000;
            break;
        default:
            // Default to Level 5.1 limits
            limits.maxMacroblocks = 36864;
            limits.maxMacroblocksPerSecond = 983040;
            limits.maxDpbSize = 184320;
            limits.maxBitrate = 240000000;
            limits.maxCPBSize = 240000000;
            break;
    }

    return limits;
}

ComplianceValidator::H265ProfileLimits ComplianceValidator::getH265LevelLimits(int level) const {
    H265ProfileLimits limits;

    // ISO/IEC 23008-2 Annex A Table A.8
    switch (level) {
        case 30: // Level 1
            limits.maxLumaSampleRate = 552960;
            limits.maxLumaPictureSize = 36864;
            limits.maxBitrate = 128000;
            limits.maxCPBSize = 350000;
            break;
        case 90: // Level 3
            limits.maxLumaSampleRate = 16588800;
            limits.maxLumaPictureSize = 983040;
            limits.maxBitrate = 6000000;
            limits.maxCPBSize = 10000000;
            break;
        case 120: // Level 4
            limits.maxLumaSampleRate = 33177600;
            limits.maxLumaPictureSize = 2228224;
            limits.maxBitrate = 12000000;
            limits.maxCPBSize = 30000000;
            break;
        case 150: // Level 5
            limits.maxLumaSampleRate = 133022720;
            limits.maxLumaPictureSize = 8912896;
            limits.maxBitrate = 25000000;
            limits.maxCPBSize = 100000000;
            break;
        default:
            limits.maxLumaSampleRate = 133022720;
            limits.maxLumaPictureSize = 8912896;
            limits.maxBitrate = 25000000;
            limits.maxCPBSize = 100000000;
            break;
    }

    return limits;
}

} // namespace VideoStudio

