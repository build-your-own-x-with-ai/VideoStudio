#include "flvparser.h"
#include <QDebug>
#include <QVariant>
#include <QtEndian>

namespace VideoStudio {

FLVParser::FLVParser()
    : m_fileSize(0)
{
}

FLVParser::~FLVParser() {
}

bool FLVParser::parseFile(const QString& filePath) {
    m_filePath = filePath;
    m_tags.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open FLV file:" << filePath;
        return false;
    }

    m_fileSize = file.size();

    // Parse FLV header
    if (!parseHeader(file)) {
        qWarning() << "Failed to parse FLV header";
        file.close();
        return false;
    }

    qDebug() << "FLV Header:";
    qDebug() << "  Version:" << m_header.version;
    qDebug() << "  Has Video:" << m_header.hasVideo();
    qDebug() << "  Has Audio:" << m_header.hasAudio();
    qDebug() << "  Data Offset:" << m_header.dataOffset;

    // Seek to first tag (skip header and first PreviousTagSize)
    file.seek(m_header.dataOffset + 4);

    // Parse all tags
    int tagCount = 0;
    while (!file.atEnd() && tagCount < 10000) { // Limit to prevent infinite loops
        int64_t tagOffset = file.pos();

        if (file.bytesAvailable() < 11) {
            break; // Not enough data for tag header
        }

        if (!parseTag(file, tagOffset)) {
            break;
        }

        tagCount++;

        // Skip PreviousTagSize (4 bytes)
        if (file.bytesAvailable() >= 4) {
            file.seek(file.pos() + 4);
        }
    }

    file.close();

    qDebug() << "Parsed" << m_tags.size() << "FLV tags";
    qDebug() << "  Video tags:" << getVideoTagCount();
    qDebug() << "  Audio tags:" << getAudioTagCount();
    qDebug() << "  Script data tags:" << getScriptDataTagCount();
    qDebug() << "  Key frames:" << getKeyFrameCount();

    return true;
}

bool FLVParser::parseHeader(QFile& file) {
    if (file.size() < 9) {
        return false;
    }

    // Read signature
    file.read(reinterpret_cast<char*>(m_header.signature), 3);
    if (m_header.signature[0] != 'F' ||
        m_header.signature[1] != 'L' ||
        m_header.signature[2] != 'V') {
        qWarning() << "Invalid FLV signature";
        return false;
    }

    // Read version
    m_header.version = readUInt8(file);

    // Read flags
    m_header.flags = readUInt8(file);

    // Read data offset
    m_header.dataOffset = readUInt32(file);

    return true;
}

bool FLVParser::parseTag(QFile& file, int64_t offset) {
    FLVTag tag;
    tag.offset = offset;

    // Read tag type
    uint8_t tagType = readUInt8(file);
    tag.type = static_cast<FLVTagType>(tagType);

    // Read data size (24-bit)
    tag.dataSize = readUInt24(file);

    // Read timestamp (24-bit)
    uint32_t timestamp = readUInt24(file);

    // Read timestamp extended (8-bit)
    tag.timestampExtended = readUInt8(file);

    // Combine timestamp
    tag.timestamp = (tag.timestampExtended << 24) | timestamp;

    // Read stream ID (24-bit, always 0)
    tag.streamID = readUInt24(file);

    // Calculate total size (11 byte header + data size)
    tag.totalSize = 11 + tag.dataSize;
    tag.percentage = (tag.totalSize * 100.0) / m_fileSize;

    // Parse tag data based on type
    bool success = false;
    switch (tag.type) {
        case FLVTagType::Video:
            success = parseVideoTag(file, tag);
            break;
        case FLVTagType::Audio:
            success = parseAudioTag(file, tag);
            break;
        case FLVTagType::ScriptData:
            success = parseScriptDataTag(file, tag);
            break;
        default:
            qWarning() << "Unknown FLV tag type:" << static_cast<int>(tagType);
            // Skip unknown tag data
            file.seek(file.pos() + tag.dataSize);
            success = true;
            break;
    }

    if (success) {
        m_tags.append(tag);
    }

    return success;
}

bool FLVParser::parseVideoTag(QFile& file, FLVTag& tag) {
    if (tag.dataSize < 1) {
        return false;
    }

    // Read video flags
    uint8_t videoFlags = readUInt8(file);

    // Extract frame type (upper 4 bits)
    tag.frameType = static_cast<FLVVideoFrameType>((videoFlags >> 4) & 0x0F);

    // Extract codec ID (lower 4 bits)
    tag.videoCodec = static_cast<FLVVideoCodec>(videoFlags & 0x0F);

    // For AVC/HEVC, read additional fields
    if (tag.videoCodec == FLVVideoCodec::AVC || tag.videoCodec == FLVVideoCodec::HEVC) {
        if (tag.dataSize >= 5) {
            tag.avcPacketType = readUInt8(file);
            tag.compositionTime = readInt24(file);
        }
    }

    // Skip remaining data
    int64_t remainingData = tag.dataSize - (file.pos() - tag.offset - 11);
    if (remainingData > 0) {
        file.seek(file.pos() + remainingData);
    }

    return true;
}

bool FLVParser::parseAudioTag(QFile& file, FLVTag& tag) {
    if (tag.dataSize < 1) {
        return false;
    }

    // Read audio flags
    uint8_t audioFlags = readUInt8(file);

    // Extract audio format (upper 4 bits)
    tag.audioFormat = static_cast<FLVAudioFormat>((audioFlags >> 4) & 0x0F);

    // Extract sound rate (bits 2-3)
    tag.soundRate = (audioFlags >> 2) & 0x03;

    // Extract sound size (bit 1)
    tag.soundSize = (audioFlags >> 1) & 0x01;

    // Extract sound type (bit 0)
    tag.soundType = audioFlags & 0x01;

    // For AAC, read packet type
    if (tag.audioFormat == FLVAudioFormat::AAC) {
        if (tag.dataSize >= 2) {
            tag.aacPacketType = readUInt8(file);
        }
    }

    // Skip remaining data
    int64_t remainingData = tag.dataSize - (file.pos() - tag.offset - 11);
    if (remainingData > 0) {
        file.seek(file.pos() + remainingData);
    }

    return true;
}

bool FLVParser::parseScriptDataTag(QFile& file, FLVTag& tag) {
    if (tag.dataSize < 2) {
        return false;
    }

    int64_t dataStart = file.pos();
    int64_t dataEnd = dataStart + tag.dataSize;

    // Read script name (AMF string)
    uint8_t nameType = readUInt8(file);
    if (nameType == 2) { // AMF String type
        tag.scriptName = parseAMFString(file);
    }

    // Read script data (AMF value)
    if (file.pos() < dataEnd) {
        uint8_t valueType = readUInt8(file);
        QVariant value = parseAMFValue(file, valueType);

        if (value.metaType().id() == QMetaType::QVariantMap) {
            tag.scriptData = value.toMap();
        }
    }

    // Skip to end of tag
    file.seek(dataEnd);

    return true;
}

// AMF parsing functions
QVariant FLVParser::parseAMFValue(QFile& file, uint8_t type) {
    switch (type) {
        case 0: // Number
            return parseAMFNumber(file);
        case 1: // Boolean
            return parseAMFBoolean(file);
        case 2: // String
            return parseAMFString(file);
        case 3: // Object
            return parseAMFObject(file);
        case 8: // ECMA Array
            return parseAMFArray(file);
        case 10: // Strict Array
            return parseAMFArray(file);
        default:
            qWarning() << "Unsupported AMF type:" << type;
            return QVariant();
    }
}

QString FLVParser::parseAMFString(QFile& file) {
    uint16_t length = readUInt16(file);
    if (length == 0) {
        return QString();
    }

    QByteArray data = file.read(length);
    return QString::fromUtf8(data);
}

double FLVParser::parseAMFNumber(QFile& file) {
    return readDouble(file);
}

bool FLVParser::parseAMFBoolean(QFile& file) {
    return readUInt8(file) != 0;
}

QVariantMap FLVParser::parseAMFObject(QFile& file) {
    QVariantMap map;

    while (true) {
        // Read property name
        QString name = parseAMFString(file);

        // Read property type
        uint8_t type = readUInt8(file);

        // Check for object end marker
        if (type == 9) { // Object End
            break;
        }

        // Read property value
        QVariant value = parseAMFValue(file, type);
        map[name] = value;
    }

    return map;
}

QVariantList FLVParser::parseAMFArray(QFile& file) {
    QVariantList list;

    // Read array length
    uint32_t length = readUInt32(file);

    // Read array elements (as object properties)
    for (uint32_t i = 0; i < length; ++i) {
        QString name = parseAMFString(file);
        uint8_t type = readUInt8(file);

        if (type == 9) { // Object End
            break;
        }

        QVariant value = parseAMFValue(file, type);
        list.append(value);
    }

    return list;
}

// Statistics functions
int FLVParser::getVideoTagCount() const {
    int count = 0;
    for (const FLVTag& tag : m_tags) {
        if (tag.type == FLVTagType::Video) {
            count++;
        }
    }
    return count;
}

int FLVParser::getAudioTagCount() const {
    int count = 0;
    for (const FLVTag& tag : m_tags) {
        if (tag.type == FLVTagType::Audio) {
            count++;
        }
    }
    return count;
}

int FLVParser::getScriptDataTagCount() const {
    int count = 0;
    for (const FLVTag& tag : m_tags) {
        if (tag.type == FLVTagType::ScriptData) {
            count++;
        }
    }
    return count;
}

int FLVParser::getKeyFrameCount() const {
    int count = 0;
    for (const FLVTag& tag : m_tags) {
        if (tag.type == FLVTagType::Video &&
            tag.frameType == FLVVideoFrameType::KeyFrame) {
            count++;
        }
    }
    return count;
}

// Helper string conversion functions
QString FLVParser::tagTypeToString(FLVTagType type) {
    switch (type) {
        case FLVTagType::Audio: return "Audio";
        case FLVTagType::Video: return "Video";
        case FLVTagType::ScriptData: return "Script Data";
        default: return "Unknown";
    }
}

QString FLVParser::videoCodecToString(FLVVideoCodec codec) {
    switch (codec) {
        case FLVVideoCodec::JPEG: return "JPEG";
        case FLVVideoCodec::SorensonH263: return "Sorenson H.263";
        case FLVVideoCodec::ScreenVideo: return "Screen Video";
        case FLVVideoCodec::VP6: return "VP6";
        case FLVVideoCodec::VP6Alpha: return "VP6 Alpha";
        case FLVVideoCodec::ScreenVideoV2: return "Screen Video V2";
        case FLVVideoCodec::AVC: return "H.264/AVC";
        case FLVVideoCodec::HEVC: return "H.265/HEVC";
        default: return QString("Unknown (%1)").arg(static_cast<int>(codec));
    }
}

QString FLVParser::audioFormatToString(FLVAudioFormat format) {
    switch (format) {
        case FLVAudioFormat::PCM: return "PCM";
        case FLVAudioFormat::ADPCM: return "ADPCM";
        case FLVAudioFormat::MP3: return "MP3";
        case FLVAudioFormat::PCM_LE: return "PCM LE";
        case FLVAudioFormat::Nellymoser16kHz: return "Nellymoser 16kHz";
        case FLVAudioFormat::Nellymoser8kHz: return "Nellymoser 8kHz";
        case FLVAudioFormat::Nellymoser: return "Nellymoser";
        case FLVAudioFormat::G711ALaw: return "G.711 A-law";
        case FLVAudioFormat::G711MuLaw: return "G.711 mu-law";
        case FLVAudioFormat::AAC: return "AAC";
        case FLVAudioFormat::Speex: return "Speex";
        case FLVAudioFormat::MP3_8kHz: return "MP3 8kHz";
        case FLVAudioFormat::DeviceSpecific: return "Device Specific";
        default: return QString("Unknown (%1)").arg(static_cast<int>(format));
    }
}

QString FLVParser::frameTypeToString(FLVVideoFrameType frameType) {
    switch (frameType) {
        case FLVVideoFrameType::KeyFrame: return "Key Frame";
        case FLVVideoFrameType::InterFrame: return "Inter Frame";
        case FLVVideoFrameType::DisposableInterFrame: return "Disposable Inter Frame";
        case FLVVideoFrameType::GeneratedKeyFrame: return "Generated Key Frame";
        case FLVVideoFrameType::VideoInfoFrame: return "Video Info Frame";
        default: return QString("Unknown (%1)").arg(static_cast<int>(frameType));
    }
}

// Binary reading helper functions
uint8_t FLVParser::readUInt8(QFile& file) {
    uint8_t value;
    file.read(reinterpret_cast<char*>(&value), 1);
    return value;
}

uint16_t FLVParser::readUInt16(QFile& file) {
    uint16_t value;
    file.read(reinterpret_cast<char*>(&value), 2);
    return qFromBigEndian(value);
}

uint32_t FLVParser::readUInt24(QFile& file) {
    uint8_t bytes[3];
    file.read(reinterpret_cast<char*>(bytes), 3);
    return (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
}

uint32_t FLVParser::readUInt32(QFile& file) {
    uint32_t value;
    file.read(reinterpret_cast<char*>(&value), 4);
    return qFromBigEndian(value);
}

int32_t FLVParser::readInt24(QFile& file) {
    uint32_t value = readUInt24(file);
    // Sign extend if negative
    if (value & 0x800000) {
        value |= 0xFF000000;
    }
    return static_cast<int32_t>(value);
}

double FLVParser::readDouble(QFile& file) {
    uint64_t value;
    file.read(reinterpret_cast<char*>(&value), 8);
    value = qFromBigEndian(value);
    return *reinterpret_cast<double*>(&value);
}

} // namespace VideoStudio
