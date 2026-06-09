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
    m_audioFrames.clear();
    m_filePath.clear();
}

const NALUnitInfo* NALUnitParser::getNALUnit(int index) const {
    if (index < 0 || index >= m_nalUnits.size()) {
        return nullptr;
    }
    return &m_nalUnits[index];
}

const AudioFrameInfo* NALUnitParser::getAudioFrame(int index) const {
    if (index < 0 || index >= m_audioFrames.size()) {
        return nullptr;
    }
    return &m_audioFrames[index];
}

bool NALUnitParser::parseFile(const QString& filePath, VideoDecoder* decoder, const QString& containerType) {
    clear();
    m_filePath = filePath;

    // If decoder is valid and open, parse video NAL units
    if (decoder && decoder->isOpen()) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            emit parseError(QString("Failed to open file: %1").arg(filePath));
            return false;
        }

        // Get codec information
        AVCodecID codecId = decoder->getCodecContext()->codec_id;
        if (codecId != AV_CODEC_ID_H264 && codecId != AV_CODEC_ID_HEVC) {
            qDebug() << "NALUnitParser: Unsupported video codec" << avcodec_get_name(codecId);
            // Don't return false - continue to parse audio
        } else {
            qDebug() << "NALUnitParser: Parsing" << containerType << "file with codec" << avcodec_get_name(codecId);

            int nalIndex = 0;

            // Extract SPS/PPS from extradata (MP4 avcC box or MKV CodecPrivate)
            AVCodecContext* codecCtx = decoder->getCodecContext();
            if (codecCtx->extradata && codecCtx->extradata_size > 0) {
                qDebug() << "NALUnitParser: Found extradata, size:" << codecCtx->extradata_size;

                // Parse avcC/hvcC format extradata
                QVector<NALUnitInfo> extradataNALs;

                if (codecId == AV_CODEC_ID_H264) {
                    // H.264 avcC format
                    extradataNALs = parseAVCCExtradata(codecCtx->extradata, codecCtx->extradata_size, codecId);
                } else if (codecId == AV_CODEC_ID_HEVC) {
                    // H.265 hvcC format
                    extradataNALs = parseHVCCExtradata(codecCtx->extradata, codecCtx->extradata_size, codecId);
                }

                // Add extradata NAL units with sequential indexing
                for (NALUnitInfo& nalInfo : extradataNALs) {
                    nalInfo.index = nalIndex++;
                    nalInfo.frameNumber = -1;  // Mark as from extradata
                    m_nalUnits.append(nalInfo);
                }

                qDebug() << "NALUnitParser: Extracted" << extradataNALs.size() << "NAL units from extradata";
            }

            // Iterate through all frames
            const FrameIndex& frameIndex = decoder->getFrameIndex();
            int totalFrames = frameIndex.frameCount();

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
        }
    }

    // Parse audio frames - open file separately to detect all streams
    parseAudioFramesFromFile(filePath);

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

    // For slices, parse slice header for detailed info
    if (info.isSlice && size > 1) {
        parseH264SliceHeader(data, size, info);
    }

    // Parse detailed SPS/PPS if applicable
    if (info.nalUnitType == H264_NAL_SPS) {
        parseH264SPS(data, size, info);
    } else if (info.nalUnitType == H264_NAL_PPS) {
        parseH264PPS(data, size, info);
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

    // Parse slice header for detailed info
    if (info.isSlice && size > 2) {
        parseH265SliceHeader(data, size, info);
    }

    // Parse detailed VPS/SPS/PPS if applicable
    if (info.nalUnitType == HEVC_NAL_VPS) {
        parseH265VPS(data, size, info);
    } else if (info.nalUnitType == HEVC_NAL_SPS) {
        parseH265SPS(data, size, info);
    } else if (info.nalUnitType == HEVC_NAL_PPS) {
        parseH265PPS(data, size, info);
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

void NALUnitParser::parseAudioFrames(const QString& filePath, AVFormatContext* formatContext, int audioStreamIndex, AVCodecID codecId) {
    // Create a new format context for audio parsing to avoid interfering with video decoding
    AVFormatContext* audioFormatContext = nullptr;
    if (avformat_open_input(&audioFormatContext, filePath.toUtf8().constData(), nullptr, nullptr) < 0) {
        qDebug() << "NALUnitParser: Failed to open file for audio parsing";
        return;
    }

    if (avformat_find_stream_info(audioFormatContext, nullptr) < 0) {
        qDebug() << "NALUnitParser: Failed to find audio stream info";
        avformat_close_input(&audioFormatContext);
        return;
    }

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        avformat_close_input(&audioFormatContext);
        return;
    }

    int audioFrameIndex = 0;
    int audioFrameNumber = 0;

    // Determine codec name
    QString codecName = QString::fromUtf8(avcodec_get_name(codecId));
    QString frameName;

    switch (codecId) {
        case AV_CODEC_ID_AAC:
            codecName = "AAC";
            frameName = "AAC raw_data_block";
            break;
        case AV_CODEC_ID_AC3:
            codecName = "AC-3";
            frameName = "AC-3 sync frame";
            break;
        case AV_CODEC_ID_EAC3:
            codecName = "E-AC-3";
            frameName = "E-AC-3 sync frame";
            break;
        case AV_CODEC_ID_MP3:
            codecName = "MP3";
            frameName = "MP3 frame";
            break;
        case AV_CODEC_ID_MP2:
            codecName = "MP2";
            frameName = "MP2 frame";
            break;
        case AV_CODEC_ID_OPUS:
            codecName = "Opus";
            frameName = "Opus packet";
            break;
        case AV_CODEC_ID_VORBIS:
            codecName = "Vorbis";
            frameName = "Vorbis packet";
            break;
        case AV_CODEC_ID_FLAC:
            codecName = "FLAC";
            frameName = "FLAC frame";
            break;
        default:
            frameName = codecName + " frame";
            break;
    }

    // Read packets from the file
    while (av_read_frame(audioFormatContext, packet) >= 0) {
        if (packet->stream_index == audioStreamIndex) {
            AudioFrameInfo audioInfo;
            audioInfo.index = audioFrameIndex++;
            audioInfo.fileOffset = packet->pos;  // File offset of this packet
            audioInfo.size = packet->size;
            audioInfo.frameNumber = audioFrameNumber++;
            audioInfo.codecType = codecName;
            audioInfo.frameName = frameName;

            // Parse codec-specific headers
            if (codecId == AV_CODEC_ID_AAC) {
                parseADTSHeader(packet->data, packet->size, audioInfo);
            } else if (codecId == AV_CODEC_ID_AC3 || codecId == AV_CODEC_ID_EAC3) {
                parseAC3Header(packet->data, packet->size, audioInfo);
            } else if (codecId == AV_CODEC_ID_MP3 || codecId == AV_CODEC_ID_MP2) {
                parseMP3Header(packet->data, packet->size, audioInfo);
            }

            m_audioFrames.append(audioInfo);
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    avformat_close_input(&audioFormatContext);

    qDebug() << "NALUnitParser: Parsed" << m_audioFrames.size() << "audio frames";
}

void NALUnitParser::parseADTSHeader(const uint8_t* data, int size, AudioFrameInfo& info) {
    if (size < 7) {
        info.hasADTS = false;
        return;
    }

    // Check for ADTS sync word (0xFFF at start of 12 bits)
    if (data[0] != 0xFF || (data[1] & 0xF0) != 0xF0) {
        info.hasADTS = false;
        return;
    }

    info.hasADTS = true;

    // Parse ADTS header fields
    // Byte 1, bits 3: MPEG version (0=MPEG-4, 1=MPEG-2)
    bool mpegVersion = (data[1] & 0x08) != 0;

    // Byte 1, bits 1-2: Layer (always 00 for AAC)
    // Byte 1, bit 0: protection_absent
    info.protectionAbsent = (data[1] & 0x01) != 0;

    // Byte 2, bits 6-7: profile (audio object type - 1)
    info.audioObjectType = ((data[2] >> 6) & 0x03) + 1;

    // Byte 2, bits 2-5: sampling frequency index
    int samplingFreqIndex = (data[2] >> 2) & 0x0F;
    static const int samplingFrequencies[] = {
        96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
        16000, 12000, 11025, 8000, 7350, 0, 0, 0
    };
    info.samplingFrequency = samplingFrequencies[samplingFreqIndex];

    // Byte 2, bit 1 + Byte 3, bits 6-7: channel configuration
    info.channelConfig = ((data[2] & 0x01) << 2) | ((data[3] >> 6) & 0x03);

    // Byte 3-5: frame length (13 bits)
    info.frameLength = ((data[3] & 0x03) << 11) | (data[4] << 3) | ((data[5] >> 5) & 0x07);
}

void NALUnitParser::parseAC3Header(const uint8_t* data, int size, AudioFrameInfo& info) {
    if (size < 7) {
        return;
    }

    // Check for AC-3 sync word (0x0B77)
    if (data[0] != 0x0B || data[1] != 0x77) {
        return;
    }

    // Byte 4, bits 6-7: sampling rate code
    int fscod = (data[4] >> 6) & 0x03;
    static const int ac3SampleRates[] = {48000, 44100, 32000, 0};
    info.samplingFrequency = ac3SampleRates[fscod];

    // Byte 4, bits 0-5: frame size code
    int frmsizecod = data[4] & 0x3F;

    // Byte 5, bits 3-7: bitstream mode and audio coding mode (acmod)
    int acmod = (data[6] >> 5) & 0x07;

    // Determine channel configuration from acmod
    static const int ac3Channels[] = {2, 1, 2, 3, 3, 4, 4, 5};
    info.channelConfig = ac3Channels[acmod];

    // Calculate bitrate from frame size code and sample rate
    static const int ac3Bitrates[] = {
        32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256,
        320, 384, 448, 512, 576, 640
    };
    if (frmsizecod < 38) {
        int bitrateIndex = frmsizecod >> 1;
        if (bitrateIndex < 19) {
            info.bitrate = ac3Bitrates[bitrateIndex];
        }
    }
}

void NALUnitParser::parseMP3Header(const uint8_t* data, int size, AudioFrameInfo& info) {
    if (size < 4) {
        return;
    }

    // Check for MP3 sync word (11 bits set to 1)
    if (data[0] != 0xFF || (data[1] & 0xE0) != 0xE0) {
        return;
    }

    // Byte 1, bits 3-4: MPEG version
    int mpegVersion = (data[1] >> 3) & 0x03;
    static const char* mpegVersionNames[] = {"MPEG-2.5", "Reserved", "MPEG-2", "MPEG-1"};
    info.mpegVersion = mpegVersion;

    // Byte 1, bits 1-2: Layer
    int layer = (data[1] >> 1) & 0x03;
    static const int layerValues[] = {0, 3, 2, 1};  // Layer I, II, III
    info.layer = layerValues[layer];

    // Byte 2, bits 4-7: Bitrate index
    int bitrateIndex = (data[2] >> 4) & 0x0F;

    // MP3 (MPEG-1 Layer III) bitrates
    static const int mp3Bitrates[16] = {
        0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0
    };
    info.bitrate = mp3Bitrates[bitrateIndex];

    // Byte 2, bits 2-3: Sampling rate index
    int sampleRateIndex = (data[2] >> 2) & 0x03;

    // MPEG-1 sample rates
    static const int sampleRates[4] = {44100, 48000, 32000, 0};
    info.samplingFrequency = sampleRates[sampleRateIndex];

    // Adjust for MPEG-2/2.5
    if (mpegVersion == 2) {  // MPEG-2
        info.samplingFrequency /= 2;
    } else if (mpegVersion == 0) {  // MPEG-2.5
        info.samplingFrequency /= 4;
    }

    // Byte 3, bits 6-7: Channel mode
    int channelMode = (data[3] >> 6) & 0x03;
    static const int channels[] = {2, 2, 2, 1};  // Stereo, Joint Stereo, Dual Channel, Mono
    info.channelConfig = channels[channelMode];
}

void NALUnitParser::parseAudioFramesFromFile(const QString& filePath) {
    // Open file to detect all streams
    AVFormatContext* audioFormatContext = nullptr;
    if (avformat_open_input(&audioFormatContext, filePath.toUtf8().constData(), nullptr, nullptr) < 0) {
        qDebug() << "NALUnitParser: Failed to open file for audio stream detection";
        return;
    }

    if (avformat_find_stream_info(audioFormatContext, nullptr) < 0) {
        qDebug() << "NALUnitParser: Failed to find stream info for audio detection";
        avformat_close_input(&audioFormatContext);
        return;
    }

    qDebug() << "NALUnitParser: Format has" << audioFormatContext->nb_streams << "streams";

    // Find audio stream
    int audioStreamIndex = -1;
    AVCodecID audioCodecId = AV_CODEC_ID_NONE;

    for (unsigned int i = 0; i < audioFormatContext->nb_streams; i++) {
        AVMediaType codecType = audioFormatContext->streams[i]->codecpar->codec_type;
        qDebug() << "NALUnitParser: Stream" << i << "type =" << codecType << "(AUDIO=" << AVMEDIA_TYPE_AUDIO << ")";

        if (codecType == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            audioCodecId = audioFormatContext->streams[i]->codecpar->codec_id;
            break;
        }
    }

    avformat_close_input(&audioFormatContext);

    if (audioStreamIndex < 0) {
        qDebug() << "NALUnitParser: No audio stream found";
        return;
    }

    QString audioCodecName = QString::fromUtf8(avcodec_get_name(audioCodecId));
    qDebug() << "NALUnitParser: Found audio stream" << audioStreamIndex << "with codec" << audioCodecName;

    // Parse supported audio codecs
    if (audioCodecId == AV_CODEC_ID_AAC ||
        audioCodecId == AV_CODEC_ID_AC3 ||
        audioCodecId == AV_CODEC_ID_EAC3 ||
        audioCodecId == AV_CODEC_ID_MP3 ||
        audioCodecId == AV_CODEC_ID_MP2 ||
        audioCodecId == AV_CODEC_ID_OPUS ||
        audioCodecId == AV_CODEC_ID_VORBIS ||
        audioCodecId == AV_CODEC_ID_FLAC) {
        parseAudioFrames(filePath, nullptr, audioStreamIndex, audioCodecId);
    } else {
        qDebug() << "NALUnitParser: Audio codec" << audioCodecName << "not yet supported";
    }
}

// Exp-Golomb decoding helpers
uint32_t NALUnitParser::readBits(const uint8_t* data, int& bitPos, int numBits) {
    uint32_t result = 0;
    for (int i = 0; i < numBits; i++) {
        int bytePos = bitPos / 8;
        int bitOffset = 7 - (bitPos % 8);
        result = (result << 1) | ((data[bytePos] >> bitOffset) & 1);
        bitPos++;
    }
    return result;
}

uint32_t NALUnitParser::readUE(const uint8_t* data, int& bitPos, int maxBits) {
    // Read unsigned Exp-Golomb coded value
    int leadingZeros = 0;
    while (bitPos < maxBits && readBits(data, bitPos, 1) == 0) {
        leadingZeros++;
        if (leadingZeros > 31) return 0;  // Prevent infinite loop
    }

    if (leadingZeros == 0) {
        return 0;
    }

    uint32_t value = (1 << leadingZeros) - 1;
    value += readBits(data, bitPos, leadingZeros);
    return value;
}

int32_t NALUnitParser::readSE(const uint8_t* data, int& bitPos, int maxBits) {
    // Read signed Exp-Golomb coded value
    uint32_t ue = readUE(data, bitPos, maxBits);
    if (ue % 2 == 0) {
        return -(int32_t)(ue / 2);
    } else {
        return (int32_t)((ue + 1) / 2);
    }
}

void NALUnitParser::parseH264SPS(const uint8_t* data, int size, NALUnitInfo& info) {
    if (size < 4) return;

    int bitPos = 8;  // Skip NAL header (1 byte)
    int maxBits = size * 8;

    // Profile and level
    info.spsProfileIdc = readBits(data, bitPos, 8);

    // Constraint flags
    info.spsConstraintSet0Flag = readBits(data, bitPos, 1) != 0;
    info.spsConstraintSet1Flag = readBits(data, bitPos, 1) != 0;
    info.spsConstraintSet2Flag = readBits(data, bitPos, 1) != 0;
    info.spsConstraintSet3Flag = readBits(data, bitPos, 1) != 0;
    bitPos += 4;  // Skip reserved 4 bits

    info.spsLevelIdc = readBits(data, bitPos, 8);

    // SPS ID
    readUE(data, bitPos, maxBits);

    // Chroma format (for High profiles)
    if (info.spsProfileIdc == 100 || info.spsProfileIdc == 110 ||
        info.spsProfileIdc == 122 || info.spsProfileIdc == 244 ||
        info.spsProfileIdc == 44 || info.spsProfileIdc == 83 ||
        info.spsProfileIdc == 86 || info.spsProfileIdc == 118) {

        info.spsChromaFormat = readUE(data, bitPos, maxBits);
        if (info.spsChromaFormat == 3) {
            bitPos++;  // separate_colour_plane_flag
        }
        info.spsBitDepthLuma = readUE(data, bitPos, maxBits) + 8;
        info.spsBitDepthChroma = readUE(data, bitPos, maxBits) + 8;
        bitPos++;  // qpprime_y_zero_transform_bypass_flag

        // Skip scaling matrices if present
        if (readBits(data, bitPos, 1)) {
            // Simplified - just skip
            bitPos += 100;  // Rough approximation
        }
    } else {
        info.spsChromaFormat = 1;  // Default 4:2:0
        info.spsBitDepthLuma = 8;
        info.spsBitDepthChroma = 8;
    }

    // log2_max_frame_num_minus4
    info.spsLog2MaxFrameNum = readUE(data, bitPos, maxBits) + 4;

    // pic_order_cnt_type
    info.spsPicOrderCntType = readUE(data, bitPos, maxBits);
    if (info.spsPicOrderCntType == 0) {
        info.spsLog2MaxPicOrderCntLsb = readUE(data, bitPos, maxBits) + 4;
    } else if (info.spsPicOrderCntType == 1) {
        // Skip POC type 1 parameters
        bitPos++;  // delta_pic_order_always_zero_flag
        readSE(data, bitPos, maxBits);  // offset_for_non_ref_pic
        readSE(data, bitPos, maxBits);  // offset_for_top_to_bottom_field
        uint32_t num_ref_frames_in_poc_cycle = readUE(data, bitPos, maxBits);
        for (uint32_t i = 0; i < num_ref_frames_in_poc_cycle && i < 256; i++) {
            readSE(data, bitPos, maxBits);
        }
    }

    // max_num_ref_frames
    info.spsMaxNumRefFrames = readUE(data, bitPos, maxBits);

    // gaps_in_frame_num_value_allowed_flag
    info.spsGapsInFrameNumAllowed = readBits(data, bitPos, 1) != 0;

    // Width and height
    uint32_t pic_width_in_mbs = readUE(data, bitPos, maxBits) + 1;
    uint32_t pic_height_in_map_units = readUE(data, bitPos, maxBits) + 1;

    info.spsWidth = pic_width_in_mbs * 16;
    info.spsHeight = pic_height_in_map_units * 16;

    // frame_mbs_only_flag
    info.spsFrameMbsOnlyFlag = readBits(data, bitPos, 1) != 0;
    if (!info.spsFrameMbsOnlyFlag) {
        info.spsHeight *= 2;  // Interlaced
        bitPos++;  // mb_adaptive_frame_field_flag
    }

    // direct_8x8_inference_flag
    bitPos++;

    // frame_cropping_flag
    if (readBits(data, bitPos, 1)) {
        readUE(data, bitPos, maxBits);  // frame_crop_left_offset
        readUE(data, bitPos, maxBits);  // frame_crop_right_offset
        readUE(data, bitPos, maxBits);  // frame_crop_top_offset
        readUE(data, bitPos, maxBits);  // frame_crop_bottom_offset
    }

    // vui_parameters_present_flag
    info.spsVuiPresent = readBits(data, bitPos, 1) != 0;
    if (info.spsVuiPresent) {
        // aspect_ratio_info_present_flag
        if (readBits(data, bitPos, 1)) {
            info.spsAspectRatioIdc = readBits(data, bitPos, 8);
            if (info.spsAspectRatioIdc == 255) {  // Extended_SAR
                info.spsSarWidth = readBits(data, bitPos, 16);
                info.spsSarHeight = readBits(data, bitPos, 16);
            }
        }

        // overscan_info_present_flag
        if (readBits(data, bitPos, 1)) {
            bitPos++;  // overscan_appropriate_flag
        }

        // video_signal_type_present_flag
        if (readBits(data, bitPos, 1)) {
            bitPos += 3;  // video_format
            bitPos++;     // video_full_range_flag
            if (readBits(data, bitPos, 1)) {  // colour_description_present_flag
                bitPos += 8;  // colour_primaries
                bitPos += 8;  // transfer_characteristics
                bitPos += 8;  // matrix_coefficients
            }
        }

        // chroma_loc_info_present_flag
        if (readBits(data, bitPos, 1)) {
            readUE(data, bitPos, maxBits);  // chroma_sample_loc_type_top_field
            readUE(data, bitPos, maxBits);  // chroma_sample_loc_type_bottom_field
        }

        // timing_info_present_flag
        info.spsTimingInfoPresent = readBits(data, bitPos, 1) != 0;
        if (info.spsTimingInfoPresent) {
            info.spsNumUnitsInTick = readBits(data, bitPos, 32);
            info.spsTimeScale = readBits(data, bitPos, 32);
            info.spsFixedFrameRate = readBits(data, bitPos, 1) != 0;
        }
    }
}

void NALUnitParser::parseH264PPS(const uint8_t* data, int size, NALUnitInfo& info) {
    if (size < 3) return;

    int bitPos = 8;  // Skip NAL header
    int maxBits = size * 8;

    // PPS ID
    readUE(data, bitPos, maxBits);

    // SPS ID
    readUE(data, bitPos, maxBits);

    // Entropy coding mode
    info.ppsEntropyCodingMode = readBits(data, bitPos, 1) != 0;

    // pic_order_present_flag
    bitPos++;

    // num_slice_groups
    info.ppsNumSliceGroups = readUE(data, bitPos, maxBits) + 1;

    // Skip slice group parameters if FMO is used
    if (info.ppsNumSliceGroups > 1) {
        // Simplified - skip FMO parameters
        return;
    }

    // Skip num_ref_idx_l0/l1_default_active
    readUE(data, bitPos, maxBits);
    readUE(data, bitPos, maxBits);

    // Weighted prediction
    info.ppsWeightedPred = readBits(data, bitPos, 1) != 0;
    info.ppsWeightedBipred = readBits(data, bitPos, 2);

    // pic_init_qp_minus26
    info.ppsPicInitQp = readSE(data, bitPos, maxBits) + 26;

    // Skip pic_init_qs_minus26
    readSE(data, bitPos, maxBits);

    // chroma_qp_index_offset
    info.ppsChromaQpIndexOffset = readSE(data, bitPos, maxBits);

    // Deblocking filter control present flag
    info.ppsDeblockingFilter = readBits(data, bitPos, 1) != 0;

    // constrained_intra_pred_flag
    info.ppsConstainedIntraPred = readBits(data, bitPos, 1) != 0;

    // redundant_pic_cnt_present_flag
    info.ppsRedundantPicCnt = readBits(data, bitPos, 1) != 0;

    // Check if more data exists for extended PPS (High profiles)
    if (bitPos < maxBits - 8) {
        // transform_8x8_mode_flag
        info.ppsTransform8x8Mode = readBits(data, bitPos, 1) != 0;
    }
}

void NALUnitParser::parseH265VPS(const uint8_t* data, int size, NALUnitInfo& info) {
    if (size < 5) return;

    int bitPos = 16;  // Skip NAL header (2 bytes)
    int maxBits = size * 8;

    // VPS ID
    readBits(data, bitPos, 4);

    // Reserved bits
    readBits(data, bitPos, 2);

    // max_layers
    info.vpsMaxLayers = readBits(data, bitPos, 6) + 1;

    // max_sub_layers
    info.vpsMaxSubLayers = readBits(data, bitPos, 3) + 1;
}

void NALUnitParser::parseH265SPS(const uint8_t* data, int size, NALUnitInfo& info) {
    if (size < 15) return;

    int bitPos = 16;  // Skip NAL header (2 bytes)
    int maxBits = size * 8;

    info.hevcSpsPresent = true;

    // Skip VPS ID
    readBits(data, bitPos, 4);

    // max_sub_layers_minus1
    info.hevcSpsMaxSubLayersMinus1 = readBits(data, bitPos, 3);
    uint32_t max_sub_layers = info.hevcSpsMaxSubLayersMinus1 + 1;

    // temporal_id_nesting_flag
    info.hevcSpsTemporalIdNesting = readBits(data, bitPos, 1) != 0;

    // Profile Tier Level
    // general_profile_space
    readBits(data, bitPos, 2);

    // general_tier_flag
    info.hevcGeneralTierFlag = readBits(data, bitPos, 1) != 0;

    // general_profile_idc
    info.spsProfileIdc = readBits(data, bitPos, 5);

    // general_profile_compatibility_flags (32 bits)
    bitPos += 32;

    // general_progressive_source_flag
    info.hevcGeneralProgressiveSourceFlag = readBits(data, bitPos, 1) != 0;

    // general_interlaced_source_flag
    info.hevcGeneralInterlacedSourceFlag = readBits(data, bitPos, 1) != 0;

    // general_non_packed_constraint_flag
    readBits(data, bitPos, 1);

    // general_frame_only_constraint_flag
    info.hevcGeneralFrameOnlyConstraintFlag = readBits(data, bitPos, 1) != 0;

    // Skip 43 reserved bits + 1 inbld_flag
    bitPos += 44;

    // general_level_idc
    info.spsLevelIdc = readBits(data, bitPos, 8);

    // Skip sub-layer profile/level flags
    for (uint32_t i = 0; i < max_sub_layers - 1 && i < 7; i++) {
        bitPos += 2;  // profile_present_flag, level_present_flag
    }

    // Skip sub-layer profile/level data
    for (uint32_t i = 0; i < max_sub_layers - 1 && i < 7; i++) {
        bitPos += 88;  // Rough approximation
    }

    // sps_seq_parameter_set_id
    info.hevcSpsSeqParameterSetId = readUE(data, bitPos, maxBits);

    // chroma_format_idc
    info.spsChromaFormat = readUE(data, bitPos, maxBits);
    if (info.spsChromaFormat == 3) {
        bitPos++;  // separate_colour_plane_flag
    }

    // pic_width_in_luma_samples
    info.spsWidth = readUE(data, bitPos, maxBits);

    // pic_height_in_luma_samples
    info.spsHeight = readUE(data, bitPos, maxBits);

    // conformance_window_flag
    info.hevcConformanceWindowFlag = readBits(data, bitPos, 1) != 0;
    if (info.hevcConformanceWindowFlag) {
        info.hevcConfWinLeftOffset = readUE(data, bitPos, maxBits);
        info.hevcConfWinRightOffset = readUE(data, bitPos, maxBits);
        info.hevcConfWinTopOffset = readUE(data, bitPos, maxBits);
        info.hevcConfWinBottomOffset = readUE(data, bitPos, maxBits);

        // Apply cropping to width/height
        int subWidthC = (info.spsChromaFormat == 1 || info.spsChromaFormat == 2) ? 2 : 1;
        int subHeightC = (info.spsChromaFormat == 1) ? 2 : 1;
        info.spsWidth -= (info.hevcConfWinLeftOffset + info.hevcConfWinRightOffset) * subWidthC;
        info.spsHeight -= (info.hevcConfWinTopOffset + info.hevcConfWinBottomOffset) * subHeightC;
    }

    // bit_depth_luma_minus8
    info.spsBitDepthLuma = readUE(data, bitPos, maxBits) + 8;

    // bit_depth_chroma_minus8
    info.spsBitDepthChroma = readUE(data, bitPos, maxBits) + 8;

    // log2_max_pic_order_cnt_lsb_minus4
    info.spsLog2MaxPicOrderCntLsb = readUE(data, bitPos, maxBits) + 4;

    // sps_sub_layer_ordering_info_present_flag
    bool subLayerOrderingInfoPresent = readBits(data, bitPos, 1) != 0;
    int startLayer = subLayerOrderingInfoPresent ? 0 : max_sub_layers - 1;
    for (int i = startLayer; i <= (int)max_sub_layers - 1 && i < 8; i++) {
        readUE(data, bitPos, maxBits);  // sps_max_dec_pic_buffering_minus1
        readUE(data, bitPos, maxBits);  // sps_max_num_reorder_pics
        readUE(data, bitPos, maxBits);  // sps_max_latency_increase_plus1
    }

    // log2_min_luma_coding_block_size_minus3
    readUE(data, bitPos, maxBits);

    // log2_diff_max_min_luma_coding_block_size
    readUE(data, bitPos, maxBits);

    // log2_min_luma_transform_block_size_minus2
    readUE(data, bitPos, maxBits);

    // log2_diff_max_min_luma_transform_block_size
    readUE(data, bitPos, maxBits);

    // max_transform_hierarchy_depth_inter
    readUE(data, bitPos, maxBits);

    // max_transform_hierarchy_depth_intra
    readUE(data, bitPos, maxBits);

    // scaling_list_enabled_flag
    if (readBits(data, bitPos, 1)) {
        // Skip scaling list data (complex)
        bitPos += 200;  // Rough approximation
    }

    // amp_enabled_flag
    bitPos++;

    // sample_adaptive_offset_enabled_flag
    bitPos++;

    // pcm_enabled_flag
    if (readBits(data, bitPos, 1)) {
        bitPos += 4;  // pcm_sample_bit_depth_luma_minus1
        bitPos += 4;  // pcm_sample_bit_depth_chroma_minus1
        readUE(data, bitPos, maxBits);  // log2_min_pcm_luma_coding_block_size_minus3
        readUE(data, bitPos, maxBits);  // log2_diff_max_min_pcm_luma_coding_block_size
        bitPos++;  // pcm_loop_filter_disabled_flag
    }

    // num_short_term_ref_pic_sets
    uint32_t numShortTermRefPicSets = readUE(data, bitPos, maxBits);
    // Skip short term reference picture sets (complex)
    for (uint32_t i = 0; i < numShortTermRefPicSets && i < 64; i++) {
        bitPos += 50;  // Rough approximation per set
    }

    // long_term_ref_pics_present_flag
    if (readBits(data, bitPos, 1)) {
        uint32_t numLongTermRefPics = readUE(data, bitPos, maxBits);
        for (uint32_t i = 0; i < numLongTermRefPics && i < 32; i++) {
            bitPos += info.spsLog2MaxPicOrderCntLsb + 1;
        }
    }

    // sps_temporal_mvp_enabled_flag
    bitPos++;

    // strong_intra_smoothing_enabled_flag
    bitPos++;

    // vui_parameters_present_flag
    info.hevcVuiPresent = readBits(data, bitPos, 1) != 0;
    if (info.hevcVuiPresent && bitPos < maxBits - 100) {
        // aspect_ratio_info_present_flag
        if (readBits(data, bitPos, 1)) {
            info.hevcVuiAspectRatioIdc = readBits(data, bitPos, 8);
            if (info.hevcVuiAspectRatioIdc == 255) {  // EXTENDED_SAR
                info.hevcVuiSarWidth = readBits(data, bitPos, 16);
                info.hevcVuiSarHeight = readBits(data, bitPos, 16);
            }
        }

        // overscan_info_present_flag
        if (readBits(data, bitPos, 1)) {
            bitPos++;  // overscan_appropriate_flag
        }

        // video_signal_type_present_flag
        if (readBits(data, bitPos, 1)) {
            bitPos += 3;  // video_format
            bitPos++;     // video_full_range_flag
            if (readBits(data, bitPos, 1)) {  // colour_description_present_flag
                bitPos += 8;  // colour_primaries
                bitPos += 8;  // transfer_characteristics
                bitPos += 8;  // matrix_coeffs
            }
        }

        // chroma_loc_info_present_flag
        if (readBits(data, bitPos, 1)) {
            readUE(data, bitPos, maxBits);  // chroma_sample_loc_type_top_field
            readUE(data, bitPos, maxBits);  // chroma_sample_loc_type_bottom_field
        }

        // neutral_chroma_indication_flag
        bitPos++;

        // field_seq_flag
        bitPos++;

        // frame_field_info_present_flag
        bitPos++;

        // default_display_window_flag
        if (readBits(data, bitPos, 1)) {
            readUE(data, bitPos, maxBits);  // def_disp_win_left_offset
            readUE(data, bitPos, maxBits);  // def_disp_win_right_offset
            readUE(data, bitPos, maxBits);  // def_disp_win_top_offset
            readUE(data, bitPos, maxBits);  // def_disp_win_bottom_offset
        }

        // vui_timing_info_present_flag
        info.hevcVuiTimingInfoPresent = readBits(data, bitPos, 1) != 0;
        if (info.hevcVuiTimingInfoPresent) {
            info.hevcVuiNumUnitsInTick = readBits(data, bitPos, 32);
            info.hevcVuiTimeScale = readBits(data, bitPos, 32);
        }
    }
}

void NALUnitParser::parseH265PPS(const uint8_t* data, int size, NALUnitInfo& info) {
    if (size < 3) return;

    int bitPos = 16;  // Skip NAL header (2 bytes)
    int maxBits = size * 8;

    info.hevcPpsPresent = true;

    // pps_pic_parameter_set_id
    info.hevcPpsPicParameterSetId = readUE(data, bitPos, maxBits);

    // pps_seq_parameter_set_id
    info.hevcPpsSeqParameterSetId = readUE(data, bitPos, maxBits);

    // dependent_slice_segments_enabled_flag
    bitPos++;

    // output_flag_present_flag
    bitPos++;

    // num_extra_slice_header_bits
    readBits(data, bitPos, 3);

    // sign_data_hiding_enabled_flag
    bitPos++;

    // cabac_init_present_flag
    info.hevcPpsCabacInitPresent = readBits(data, bitPos, 1) != 0;

    // num_ref_idx_l0_default_active_minus1
    info.hevcPpsNumRefIdxL0DefaultActive = readUE(data, bitPos, maxBits) + 1;

    // num_ref_idx_l1_default_active_minus1
    info.hevcPpsNumRefIdxL1DefaultActive = readUE(data, bitPos, maxBits) + 1;

    // init_qp_minus26
    info.hevcPpsInitQpMinus26 = readSE(data, bitPos, maxBits);

    // constrained_intra_pred_flag
    info.hevcPpsConstrainedIntraPred = readBits(data, bitPos, 1) != 0;

    // transform_skip_enabled_flag
    info.hevcPpsTransformSkipEnabled = readBits(data, bitPos, 1) != 0;

    // cu_qp_delta_enabled_flag
    info.hevcPpsCuQpDeltaEnabled = readBits(data, bitPos, 1) != 0;
    if (info.hevcPpsCuQpDeltaEnabled) {
        readUE(data, bitPos, maxBits);  // diff_cu_qp_delta_depth
    }

    // pps_cb_qp_offset
    readSE(data, bitPos, maxBits);

    // pps_cr_qp_offset
    readSE(data, bitPos, maxBits);

    // pps_slice_chroma_qp_offsets_present_flag
    bitPos++;

    // weighted_pred_flag
    bitPos++;

    // weighted_bipred_flag
    bitPos++;

    // transquant_bypass_enabled_flag
    info.hevcPpsTransquantBypassEnabled = readBits(data, bitPos, 1) != 0;

    // tiles_enabled_flag
    bool tilesEnabled = readBits(data, bitPos, 1) != 0;

    // entropy_coding_sync_enabled_flag
    bitPos++;

    if (tilesEnabled && bitPos < maxBits - 20) {
        // num_tile_columns_minus1
        readUE(data, bitPos, maxBits);

        // num_tile_rows_minus1
        readUE(data, bitPos, maxBits);

        // uniform_spacing_flag
        if (!readBits(data, bitPos, 1)) {
            // Skip tile column/row widths
            bitPos += 100;  // Rough approximation
        }

        // loop_filter_across_tiles_enabled_flag
        bitPos++;
    }

    // Remaining PPS fields are complex, skip for now
}

QVector<NALUnitInfo> NALUnitParser::parseAVCCExtradata(const uint8_t* data, int size, AVCodecID codecId) {
    QVector<NALUnitInfo> nalUnits;

    if (size < 7) {
        return nalUnits;
    }

    // avcC format:
    // [0] configurationVersion (1 byte)
    // [1] AVCProfileIndication (1 byte)
    // [2] profile_compatibility (1 byte)
    // [3] AVCLevelIndication (1 byte)
    // [4] lengthSizeMinusOne (1 byte, bits 0-1)
    // [5] numOfSequenceParameterSets (1 byte, bits 0-4)

    int pos = 5;
    int numSPS = data[pos] & 0x1F;
    pos++;

    qDebug() << "NALUnitParser: avcC has" << numSPS << "SPS";

    // Parse SPS
    for (int i = 0; i < numSPS && pos + 2 < size; i++) {
        int spsLen = (data[pos] << 8) | data[pos + 1];
        pos += 2;

        if (pos + spsLen <= size) {
            NALUnitInfo nalInfo;
            nalInfo.fileOffset = 0;  // From extradata, not in file
            nalInfo.size = spsLen;
            nalInfo.frameNumber = -1;

            parseH264NALHeader(&data[pos], spsLen, nalInfo);
            nalUnits.append(nalInfo);

            pos += spsLen;
        }
    }

    // Parse PPS
    if (pos + 1 < size) {
        int numPPS = data[pos];
        pos++;

        qDebug() << "NALUnitParser: avcC has" << numPPS << "PPS";

        for (int i = 0; i < numPPS && pos + 2 < size; i++) {
            int ppsLen = (data[pos] << 8) | data[pos + 1];
            pos += 2;

            if (pos + ppsLen <= size) {
                NALUnitInfo nalInfo;
                nalInfo.fileOffset = 0;
                nalInfo.size = ppsLen;
                nalInfo.frameNumber = -1;

                parseH264NALHeader(&data[pos], ppsLen, nalInfo);
                nalUnits.append(nalInfo);

                pos += ppsLen;
            }
        }
    }

    return nalUnits;
}

QVector<NALUnitInfo> NALUnitParser::parseHVCCExtradata(const uint8_t* data, int size, AVCodecID codecId) {
    QVector<NALUnitInfo> nalUnits;

    if (size < 23) {
        return nalUnits;
    }

    // hvcC format is more complex, simplified parsing:
    // Skip to numOfArrays at byte 22
    int pos = 22;
    int numArrays = data[pos];
    pos++;

    qDebug() << "NALUnitParser: hvcC has" << numArrays << "NAL arrays";

    for (int i = 0; i < numArrays && pos + 3 < size; i++) {
        // Array type (1 byte, bit 0-5 = NAL unit type)
        pos++;  // Skip array_completeness and NAL type

        // numNalus (2 bytes)
        int numNalus = (data[pos] << 8) | data[pos + 1];
        pos += 2;

        for (int j = 0; j < numNalus && pos + 2 < size; j++) {
            int nalLen = (data[pos] << 8) | data[pos + 1];
            pos += 2;

            if (pos + nalLen <= size) {
                NALUnitInfo nalInfo;
                nalInfo.fileOffset = 0;
                nalInfo.size = nalLen;
                nalInfo.frameNumber = -1;

                parseH265NALHeader(&data[pos], nalLen, nalInfo);
                nalUnits.append(nalInfo);

                pos += nalLen;
            }
        }
    }

    return nalUnits;
}

void NALUnitParser::parseH264SliceHeader(const uint8_t* data, int size, NALUnitInfo& info) {
    if (size < 2) {
        return;
    }

    // Skip NAL header (1 byte) and start parsing slice header
    int bitPos = 8;
    int maxBits = size * 8;

    try {
        // first_mb_in_slice
        info.firstMbInSlice = readUE(data, bitPos, maxBits);
        if (bitPos >= maxBits) return;

        // slice_type (0-9)
        info.sliceTypeValue = readUE(data, bitPos, maxBits);
        if (bitPos >= maxBits) return;

        // Map slice_type to I/P/B
        // 0,5=P  1,6=B  2,7=I  3,8=SP  4,9=SI
        int baseType = info.sliceTypeValue % 5;
        switch (baseType) {
            case 0: info.sliceType = "P"; break;
            case 1: info.sliceType = "B"; break;
            case 2: info.sliceType = "I"; break;
            case 3: info.sliceType = "SP"; break;
            case 4: info.sliceType = "SI"; break;
            default: info.sliceType = "Unknown"; break;
        }

        // pic_parameter_set_id
        info.ppsId = readUE(data, bitPos, maxBits);
        if (bitPos >= maxBits) return;

        // Try to parse QP with simplified assumptions
        // This is a best-effort parse - may not work for all videos
        // Assumes: frame_mbs_only_flag=1 (progressive), no field coding

        // colour_plane_id (only if separate_colour_plane_flag in SPS, rare)
        // Skip for now

        // frame_num (variable length, depends on log2_max_frame_num_minus4 in SPS)
        // Assume typical value of 4 bits
        if (bitPos + 4 < maxBits) {
            info.frameNum = readBits(data, bitPos, 4);
        }

        // For IDR slices, parse idr_pic_id
        if (info.nalUnitType == H264_NAL_IDR_SLICE && bitPos < maxBits) {
            info.idrPicId = readUE(data, bitPos, maxBits);
            if (bitPos >= maxBits) return;
        }

        // pic_order_cnt_lsb (if pic_order_cnt_type == 0, typical case)
        // Assume 8 bits (typical log2_max_pic_order_cnt_lsb_minus4 = 4)
        if (bitPos + 8 < maxBits) {
            info.picOrderCntLsb = readBits(data, bitPos, 8);
        }

        // num_ref_idx_active_override_flag
        if (bitPos < maxBits && info.sliceType != "I") {
            bool override_flag = readBits(data, bitPos, 1);
            if (override_flag && bitPos < maxBits) {
                info.numRefIdxL0ActiveMinus1 = readUE(data, bitPos, maxBits);
                if (info.sliceType == "B" && bitPos < maxBits) {
                    info.numRefIdxL1ActiveMinus1 = readUE(data, bitPos, maxBits);
                }
            }
        }

        // Parse ref_pic_list_modification for L0
        if (bitPos < maxBits && (info.sliceType == "P" || info.sliceType == "B")) {
            info.refPicListModificationFlagL0 = readBits(data, bitPos, 1);
            if (info.refPicListModificationFlagL0 && bitPos < maxBits) {
                // Parse reference list modifications
                while (bitPos < maxBits) {
                    uint32_t modification_of_pic_nums_idc = readUE(data, bitPos, maxBits);
                    if (modification_of_pic_nums_idc == 3) {
                        // End of modification commands
                        break;
                    }
                    if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1) {
                        // abs_diff_pic_num_minus1
                        uint32_t abs_diff = readUE(data, bitPos, maxBits);
                        info.refPicListL0.append(abs_diff);
                    } else if (modification_of_pic_nums_idc == 2) {
                        // long_term_pic_num
                        uint32_t long_term = readUE(data, bitPos, maxBits);
                        info.refPicListL0.append(long_term);
                    }
                    if (bitPos >= maxBits) break;
                }
            }
        }

        // Parse ref_pic_list_modification for L1 (B slices only)
        if (bitPos < maxBits && info.sliceType == "B") {
            info.refPicListModificationFlagL1 = readBits(data, bitPos, 1);
            if (info.refPicListModificationFlagL1 && bitPos < maxBits) {
                while (bitPos < maxBits) {
                    uint32_t modification_of_pic_nums_idc = readUE(data, bitPos, maxBits);
                    if (modification_of_pic_nums_idc == 3) {
                        break;
                    }
                    if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1) {
                        uint32_t abs_diff = readUE(data, bitPos, maxBits);
                        info.refPicListL1.append(abs_diff);
                    } else if (modification_of_pic_nums_idc == 2) {
                        uint32_t long_term = readUE(data, bitPos, maxBits);
                        info.refPicListL1.append(long_term);
                    }
                    if (bitPos >= maxBits) break;
                }
            }
        }

        // For weighted prediction (check if enabled in PPS, assume not for now)

        // dec_ref_pic_marking (for reference frames, skip)

        // cabac_init_idc (only if CABAC enabled and not I slice)
        // Skip for now

        // slice_qp_delta - this is what we want!
        if (bitPos + 16 < maxBits) {
            int32_t slice_qp_delta = readSE(data, bitPos, maxBits);
            // Typically pic_init_qp_minus26 is 0, so base QP = 26
            info.sliceQP = 26 + slice_qp_delta;
            info.sliceQpDeltaValid = true;

            // Clamp to valid range [0, 51]
            if (info.sliceQP < 0) info.sliceQP = 0;
            if (info.sliceQP > 51) info.sliceQP = 51;
        }

    } catch (...) {
        // Parsing error - keep what we have
    }
}

void NALUnitParser::parseH265SliceHeader(const uint8_t* data, int size, NALUnitInfo& info) {
    if (size < 3) {
        return;
    }

    // Skip NAL header (2 bytes)
    int bitPos = 16;
    int maxBits = size * 8;

    // first_slice_segment_in_pic_flag
    bool first_slice = readBits(data, bitPos, 1);

    // For IRAP pictures, skip no_output_of_prior_pics_flag
    if (info.isKeyFrame) {
        bitPos++;
    }

    // slice_pic_parameter_set_id
    info.ppsId = readUE(data, bitPos, maxBits);

    if (!first_slice) {
        // Skip dependent_slice_segment_flag and slice_segment_address
        // This requires knowledge of PicSizeInCtbsY from SPS
        return;  // Simplified: only parse first slice in picture
    }

    // slice_type (0=B, 1=P, 2=I)
    uint32_t hevc_slice_type = readUE(data, bitPos, maxBits);
    switch (hevc_slice_type) {
        case 0: info.sliceType = "B"; break;
        case 1: info.sliceType = "P"; break;
        case 2: info.sliceType = "I"; break;
        default: info.sliceType = "Unknown"; break;
    }
    info.sliceTypeValue = hevc_slice_type;

    // Skip output_flag, colour_plane_id, etc.
    // Parsing HEVC slice QP requires knowing many SPS/PPS parameters
    // Simplified implementation

    info.sliceQP = -1;  // Would require full HEVC slice header parsing
}

} // namespace VideoStudio
