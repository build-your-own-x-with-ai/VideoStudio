#include "core/nalunitparser.h"
#include "core/videodecoder.h"
#include "core/framedata.h"
#include <QDebug>
#include <QFile>

namespace VideoStudio {

NALUnitParser::NALUnitParser(QObject* parent)
    : QObject(parent)
{
}

NALUnitParser::~NALUnitParser() {
    clear();
}

void NALUnitParser::clear() {
    m_nalUnits.clear();
    m_filePath.clear();
}

const NALUnitInfo* NALUnitParser::getNALUnit(int index) const {
    if (index < 0 || index >= m_nalUnits.size()) {
        return nullptr;
    }
    return &m_nalUnits[index];
}

bool NALUnitParser::parseFile(const QString& filePath, VideoDecoder* decoder, const QString& containerType) {
    if (!decoder || !decoder->isOpen()) {
        emit parseError("Invalid decoder or file not open");
        return false;
    }

    clear();
    m_filePath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit parseError(QString("Failed to open file: %1").arg(filePath));
        return false;
    }

    // Get codec information
    AVCodecID codecId = decoder->getCodecContext()->codec_id;
    if (codecId != AV_CODEC_ID_H264 && codecId != AV_CODEC_ID_HEVC) {
        qDebug() << "NALUnitParser: Unsupported codec" << avcodec_get_name(codecId);
        emit parseError(QString("Unsupported codec: %1").arg(avcodec_get_name(codecId)));
        file.close();
        return false;
    }

    qDebug() << "NALUnitParser: Parsing" << containerType << "file with codec" << avcodec_get_name(codecId);

    // Iterate through all frames
    const FrameIndex& frameIndex = decoder->getFrameIndex();
    int totalFrames = frameIndex.frameCount();
    int nalIndex = 0;

    for (int frameNum = 0; frameNum < totalFrames; ++frameNum) {
        const FrameInfo* frameInfo = frameIndex.getFrame(frameNum);
        if (!frameInfo) {
            continue;
        }

        // Emit progress every 50 frames
        if (frameNum % 50 == 0 || frameNum == totalFrames - 1) {
            emit parseProgress(frameNum + 1, totalFrames,
                             QString("Parsing NAL units... Frame %1 / %2").arg(frameNum + 1).arg(totalFrames));
        }

        // Read compressed frame data from file
        if (!file.seek(frameInfo->offset)) {
            qDebug() << "NALUnitParser: Failed to seek to offset" << frameInfo->offset;
            continue;
        }

        QByteArray frameData = file.read(frameInfo->size);
        if (frameData.size() != frameInfo->size) {
            qDebug() << "NALUnitParser: Failed to read frame data, expected" << frameInfo->size << "got" << frameData.size();
            continue;
        }

        // Extract NAL units from this frame
        QVector<NALUnitInfo> frameNALs = extractNALUnits(frameData, codecId, frameNum, frameInfo->offset);

        // Add to global list with sequential indexing
        for (NALUnitInfo& nalInfo : frameNALs) {
            nalInfo.index = nalIndex++;
            m_nalUnits.append(nalInfo);
        }
    }

    file.close();

    qDebug() << "NALUnitParser: Parsed" << m_nalUnits.size() << "NAL units from" << totalFrames << "frames";
    emit parseComplete();
    return true;
}

NALUnitParser::BitstreamFormat NALUnitParser::detectFormat(const QByteArray& data) const {
    if (data.size() < 4) {
        return FORMAT_AVCC;  // Default to AVCC
    }

    // Check for AnnexB start codes: 0x000001 or 0x00000001
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());

    if (bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x01) {
        return FORMAT_ANNEXB;
    }

    if (bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x00 && bytes[3] == 0x01) {
        return FORMAT_ANNEXB;
    }

    return FORMAT_AVCC;
}

QVector<NALUnitInfo> NALUnitParser::extractNALUnits(const QByteArray& frameData, AVCodecID codecId,
                                                      int frameNumber, int64_t frameOffset) {
    QVector<NALUnitInfo> nalUnits;

    if (frameData.isEmpty()) {
        return nalUnits;
    }

    BitstreamFormat format = detectFormat(frameData);
    const uint8_t* data = reinterpret_cast<const uint8_t*>(frameData.constData());
    int dataSize = frameData.size();
    int pos = 0;

    if (format == FORMAT_AVCC) {
        // AVCC format: 4-byte length prefix followed by NAL data
        while (pos + 4 < dataSize) {
            // Read 4-byte big-endian length
            uint32_t nalSize = (data[pos] << 24) | (data[pos + 1] << 16) |
                              (data[pos + 2] << 8) | data[pos + 3];
            pos += 4;

            if (nalSize == 0 || pos + nalSize > dataSize) {
                qDebug() << "NALUnitParser: Invalid NAL size" << nalSize << "at pos" << pos;
                break;
            }

            // Create NAL unit info
            NALUnitInfo nalInfo;
            nalInfo.fileOffset = frameOffset + pos;
            nalInfo.size = nalSize;
            nalInfo.frameNumber = frameNumber;

            // Parse NAL header
            if (codecId == AV_CODEC_ID_H264) {
                parseH264NALHeader(&data[pos], nalSize, nalInfo);
            } else if (codecId == AV_CODEC_ID_HEVC) {
                parseH265NALHeader(&data[pos], nalSize, nalInfo);
            }

            nalUnits.append(nalInfo);
            pos += nalSize;
        }
    } else {
        // AnnexB format: start codes 0x000001 or 0x00000001
        while (pos < dataSize) {
            // Find next start code
            int startCodeSize = 0;
            int nextPos = pos;

            // Skip current start code if at beginning
            if (pos < 3 || (data[pos - 3] == 0x00 && data[pos - 2] == 0x00 && data[pos - 1] == 0x01)) {
                nextPos = pos;
            } else if (pos < 4 || (data[pos - 4] == 0x00 && data[pos - 3] == 0x00 &&
                                   data[pos - 2] == 0x00 && data[pos - 1] == 0x01)) {
                nextPos = pos;
            }

            // Find next start code
            bool foundNext = false;
            for (int i = nextPos + 3; i < dataSize; ++i) {
                if (i >= 2 && data[i - 2] == 0x00 && data[i - 1] == 0x00 && data[i] == 0x01) {
                    nextPos = i + 1;
                    startCodeSize = 3;
                    foundNext = true;
                    break;
                } else if (i >= 3 && data[i - 3] == 0x00 && data[i - 2] == 0x00 &&
                          data[i - 1] == 0x00 && data[i] == 0x01) {
                    nextPos = i + 1;
                    startCodeSize = 4;
                    foundNext = true;
                    break;
                }
            }

            if (!foundNext) {
                nextPos = dataSize;
            }

            int nalSize = nextPos - pos - startCodeSize;
            if (nalSize > 0) {
                NALUnitInfo nalInfo;
                nalInfo.fileOffset = frameOffset + pos;
                nalInfo.size = nalSize;
                nalInfo.frameNumber = frameNumber;

                // Parse NAL header
                if (codecId == AV_CODEC_ID_H264) {
                    parseH264NALHeader(&data[pos], nalSize, nalInfo);
                } else if (codecId == AV_CODEC_ID_HEVC) {
                    parseH265NALHeader(&data[pos], nalSize, nalInfo);
                }

                nalUnits.append(nalInfo);
            }

            pos = nextPos;
            if (!foundNext) {
                break;
            }
        }
    }

    return nalUnits;
}

void NALUnitParser::parseH264NALHeader(const uint8_t* data, int size, NALUnitInfo& info) {
    if (size < 1) {
        return;
    }

    uint8_t byte0 = data[0];
    info.nalUnitType = byte0 & 0x1F;  // bits 0-4
    info.typeName = getH264NALTypeName(info.nalUnitType);
    info.layerId = 0;  // H.264 doesn't have layer ID
    info.temporalId = -1;  // H.264 doesn't have temporal ID in base profile

    // Check if this is an IDR slice
    info.isIDR = (info.nalUnitType == H264_NAL_IDR_SLICE);
    info.isKeyFrame = info.isIDR;

    // Check if this is a slice NAL
    info.isSlice = (info.nalUnitType >= H264_NAL_SLICE && info.nalUnitType <= H264_NAL_IDR_SLICE);

    // For slices, we could parse the slice header for more details
    // but that requires complex bit-level parsing, so we'll skip for now
    if (info.isSlice) {
        info.sliceType = info.isIDR ? "I" : "P/B";  // Simplified
        info.sliceQP = -1;  // Would require slice header parsing
    }
}

void NALUnitParser::parseH265NALHeader(const uint8_t* data, int size, NALUnitInfo& info) {
    if (size < 2) {
        return;
    }

    uint8_t byte0 = data[0];
    uint8_t byte1 = data[1];

    // Parse NAL unit type (bits 1-6 of byte 0)
    info.nalUnitType = (byte0 >> 1) & 0x3F;
    info.typeName = getH265NALTypeName(info.nalUnitType);

    // Parse layer ID (bit 0 of byte 0 + bits 3-7 of byte 1)
    info.layerId = ((byte0 & 0x01) << 5) | ((byte1 >> 3) & 0x1F);

    // Parse temporal ID (bits 0-2 of byte 1)
    info.temporalId = (byte1 & 0x07) - 1;

    // Check if this is an IDR slice
    info.isIDR = (info.nalUnitType == HEVC_NAL_IDR_W_RADL || info.nalUnitType == HEVC_NAL_IDR_N_LP);
    info.isKeyFrame = info.isIDR || (info.nalUnitType >= HEVC_NAL_BLA_W_LP && info.nalUnitType <= HEVC_NAL_CRA_NUT);

    // Check if this is a slice NAL (types 0-9 and 16-21)
    info.isSlice = (info.nalUnitType <= 9) || (info.nalUnitType >= 16 && info.nalUnitType <= 21);

    if (info.isSlice) {
        if (info.isIDR) {
            info.sliceType = "I";
        } else {
            info.sliceType = "P/B";  // Simplified
        }
        info.sliceQP = -1;  // Would require slice header parsing
    }
}

QString NALUnitParser::getH264NALTypeName(int type) const {
    switch (type) {
        case H264_NAL_SLICE: return "H.264 Non-IDR Slice";
        case H264_NAL_DPA: return "H.264 DPA";
        case H264_NAL_DPB: return "H.264 DPB";
        case H264_NAL_DPC: return "H.264 DPC";
        case H264_NAL_IDR_SLICE: return "H.264 IDR Slice";
        case H264_NAL_SEI: return "H.264 SEI";
        case H264_NAL_SPS: return "H.264 SPS";
        case H264_NAL_PPS: return "H.264 PPS";
        case H264_NAL_AUD: return "H.264 AUD";
        case H264_NAL_END_SEQUENCE: return "H.264 End Sequence";
        case H264_NAL_END_STREAM: return "H.264 End Stream";
        case H264_NAL_FILLER_DATA: return "H.264 Filler Data";
        case H264_NAL_SPS_EXT: return "H.264 SPS Extension";
        case H264_NAL_PREFIX: return "H.264 Prefix";
        case H264_NAL_SUB_SPS: return "H.264 Subset SPS";
        case H264_NAL_AUXILIARY_SLICE: return "H.264 Auxiliary Slice";
        case H264_NAL_SLICE_EXT: return "H.264 Slice Extension";
        default: return QString("H.264 NAL Type %1").arg(type);
    }
}

QString NALUnitParser::getH265NALTypeName(int type) const {
    switch (type) {
        case HEVC_NAL_TRAIL_N: return "HEVC TRAIL_N";
        case HEVC_NAL_TRAIL_R: return "HEVC TRAIL_R";
        case HEVC_NAL_TSA_N: return "HEVC TSA_N";
        case HEVC_NAL_TSA_R: return "HEVC TSA_R";
        case HEVC_NAL_STSA_N: return "HEVC STSA_N";
        case HEVC_NAL_STSA_R: return "HEVC STSA_R";
        case HEVC_NAL_RADL_N: return "HEVC RADL_N";
        case HEVC_NAL_RADL_R: return "HEVC RADL_R";
        case HEVC_NAL_RASL_N: return "HEVC RASL_N";
        case HEVC_NAL_RASL_R: return "HEVC RASL_R";
        case HEVC_NAL_BLA_W_LP: return "HEVC BLA_W_LP";
        case HEVC_NAL_BLA_W_RADL: return "HEVC BLA_W_RADL";
        case HEVC_NAL_BLA_N_LP: return "HEVC BLA_N_LP";
        case HEVC_NAL_IDR_W_RADL: return "HEVC IDR_W_RADL";
        case HEVC_NAL_IDR_N_LP: return "HEVC IDR_N_LP";
        case HEVC_NAL_CRA_NUT: return "HEVC CRA";
        case HEVC_NAL_VPS: return "HEVC VPS";
        case HEVC_NAL_SPS: return "HEVC SPS";
        case HEVC_NAL_PPS: return "HEVC PPS";
        case HEVC_NAL_AUD: return "HEVC AUD";
        case HEVC_NAL_EOS_NUT: return "HEVC EOS";
        case HEVC_NAL_EOB_NUT: return "HEVC EOB";
        case HEVC_NAL_FD_NUT: return "HEVC Filler Data";
        case HEVC_NAL_SEI_PREFIX: return "HEVC SEI PREFIX";
        case HEVC_NAL_SEI_SUFFIX: return "HEVC SEI SUFFIX";
        default: return QString("HEVC NAL Type %1").arg(type);
    }
}

} // namespace VideoStudio
