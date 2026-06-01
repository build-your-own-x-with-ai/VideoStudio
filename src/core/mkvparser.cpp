#include "core/mkvparser.h"
#include <QDebug>
#include <cmath>

namespace VideoStudio {

MKVParser::MKVParser()
    : m_fileSize(0)
{
}

MKVParser::~MKVParser() {
}

bool MKVParser::parseFile(const QString& filePath) {
    m_filePath = filePath;
    m_elements.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open MKV file:" << filePath;
        return false;
    }

    m_fileSize = file.size();
    qDebug() << "Parsing MKV file:" << filePath << "Size:" << m_fileSize << "bytes";

    // Parse all top-level elements
    int64_t offset = 0;
    while (offset < m_fileSize) {
        int64_t beforeSize = m_elements.size();
        if (!parseElement(file, offset, m_fileSize - offset, 0, m_elements)) {
            break;
        }
        if (m_elements.size() > beforeSize) {
            offset = m_elements.last().offset + m_elements.last().totalSize;
        } else {
            break;
        }
    }

    file.close();

    qDebug() << "Parsed" << m_elements.size() << "top-level elements";
    return !m_elements.isEmpty();
}

bool MKVParser::parseElement(QFile& file, int64_t offset, int64_t maxSize, int level, QVector<EBMLElement>& elements) {
    if (maxSize < 2) {
        return false;
    }

    file.seek(offset);

    EBMLElement element;
    element.offset = offset;
    element.level = level;

    // Read element ID
    int idLength = 0;
    element.id = readElementID(file, idLength);
    if (idLength == 0) {
        return false;
    }

    // Read element size
    int sizeLength = 0;
    element.dataSize = readEBMLVarInt(file, sizeLength);
    if (sizeLength == 0) {
        return false;
    }

    element.headerSize = idLength + sizeLength;
    element.totalSize = element.headerSize + element.dataSize;
    element.percentage = (double)element.totalSize / m_fileSize * 100.0;
    element.name = getElementName(element.id);

    qDebug() << QString("  ").repeated(level) << "Element:" << element.name
             << QString("(0x%1)").arg(element.id, 0, 16)
             << "Offset:" << QString("0x%1").arg(element.offset, 0, 16)
             << "Size:" << element.totalSize << "bytes"
             << QString("(%1%)").arg(element.percentage, 0, 'f', 2);

    // Parse specific element types
    if (element.id == static_cast<uint32_t>(EBMLElementID::EBML)) {
        parseEBMLHeader(file, element);
    } else if (element.id == static_cast<uint32_t>(EBMLElementID::Info)) {
        parseSegmentInfo(file, element);
    } else if (element.id == static_cast<uint32_t>(EBMLElementID::TrackEntry)) {
        parseTrackEntry(file, element);
    }

    // Parse children if this is a master element
    if (isMasterElement(element.id) && element.dataSize > 0) {
        int64_t childOffset = offset + element.headerSize;
        int64_t childMaxSize = element.dataSize;

        while (childMaxSize > 2) {
            int64_t beforeSize = element.children.size();
            if (!parseElement(file, childOffset, childMaxSize, level + 1, element.children)) {
                break;
            }
            if (element.children.size() > beforeSize) {
                const EBMLElement& lastChild = element.children.last();
                int64_t childSize = lastChild.totalSize;
                childOffset += childSize;
                childMaxSize -= childSize;
            } else {
                break;
            }
        }
    } else if (element.dataSize > 0 && element.dataSize < 1024 * 1024) {
        // Parse data for non-master elements (limit to 1MB to avoid huge binary data)
        int64_t savedPos = file.pos();

        // Determine data type based on element ID
        switch (static_cast<EBMLElementID>(element.id)) {
        // String elements
        case EBMLElementID::DocType:
        case EBMLElementID::MuxingApp:
        case EBMLElementID::WritingApp:
        case EBMLElementID::Title:
        case EBMLElementID::Name:
        case EBMLElementID::Language:
        case EBMLElementID::CodecID:
        case EBMLElementID::CodecName:
        case EBMLElementID::FileName:
        case EBMLElementID::FileMimeType:
            element.stringValue = readString(file, element.dataSize);
            break;

        // Unsigned integer elements
        case EBMLElementID::EBMLVersion:
        case EBMLElementID::EBMLReadVersion:
        case EBMLElementID::EBMLMaxIDLength:
        case EBMLElementID::EBMLMaxSizeLength:
        case EBMLElementID::DocTypeVersion:
        case EBMLElementID::DocTypeReadVersion:
        case EBMLElementID::TimecodeScale:
        case EBMLElementID::TrackNumber:
        case EBMLElementID::TrackUID:
        case EBMLElementID::TrackType:
        case EBMLElementID::FlagEnabled:
        case EBMLElementID::FlagDefault:
        case EBMLElementID::FlagForced:
        case EBMLElementID::FlagLacing:
        case EBMLElementID::DefaultDuration:
        case EBMLElementID::PixelWidth:
        case EBMLElementID::PixelHeight:
        case EBMLElementID::DisplayWidth:
        case EBMLElementID::DisplayHeight:
        case EBMLElementID::Channels:
        case EBMLElementID::BitDepth:
        case EBMLElementID::Timecode:
        case EBMLElementID::CueTime:
        case EBMLElementID::CueTrack:
        case EBMLElementID::CueClusterPosition:
            element.uintValue = readUInt(file, element.dataSize);
            break;

        // Float elements
        case EBMLElementID::Duration:
        case EBMLElementID::SamplingFrequency:
        case EBMLElementID::FrameRate:
            element.floatValue = readFloat(file, element.dataSize);
            break;

        // Binary elements (don't read large ones)
        case EBMLElementID::CodecPrivate:
        case EBMLElementID::FileData:
            if (element.dataSize < 10240) { // Only read small binary data (< 10KB)
                element.binaryValue = readBinary(file, element.dataSize);
            }
            break;

        default:
            // Unknown element type, skip
            break;
        }

        file.seek(savedPos + element.dataSize);
    }

    elements.append(element);
    return true;
}

uint64_t MKVParser::readEBMLVarInt(QFile& file, int& length) {
    uint8_t firstByte = 0;
    if (file.read(reinterpret_cast<char*>(&firstByte), 1) != 1) {
        length = 0;
        return 0;
    }

    // Find the length marker (first 1 bit)
    int numBytes = 0;
    uint8_t mask = 0x80;
    for (int i = 0; i < 8; ++i) {
        if (firstByte & mask) {
            numBytes = i + 1;
            break;
        }
        mask >>= 1;
    }

    if (numBytes == 0) {
        length = 0;
        return 0;
    }

    // Remove the length marker
    uint64_t value = firstByte & (mask - 1);

    // Read remaining bytes
    for (int i = 1; i < numBytes; ++i) {
        uint8_t byte = 0;
        if (file.read(reinterpret_cast<char*>(&byte), 1) != 1) {
            length = 0;
            return 0;
        }
        value = (value << 8) | byte;
    }

    length = numBytes;
    return value;
}

uint32_t MKVParser::readElementID(QFile& file, int& length) {
    uint8_t firstByte = 0;
    if (file.read(reinterpret_cast<char*>(&firstByte), 1) != 1) {
        length = 0;
        return 0;
    }

    // Find the length (number of bytes in ID)
    int numBytes = 0;
    uint8_t mask = 0x80;
    for (int i = 0; i < 4; ++i) {
        if (firstByte & mask) {
            numBytes = i + 1;
            break;
        }
        mask >>= 1;
    }

    if (numBytes == 0) {
        length = 0;
        return 0;
    }

    // Build the ID (keep the length marker)
    uint32_t id = firstByte;

    // Read remaining bytes
    for (int i = 1; i < numBytes; ++i) {
        uint8_t byte = 0;
        if (file.read(reinterpret_cast<char*>(&byte), 1) != 1) {
            length = 0;
            return 0;
        }
        id = (id << 8) | byte;
    }

    length = numBytes;
    return id;
}

void MKVParser::parseEBMLHeader(QFile& file, EBMLElement& element) {
    // EBML header is already parsed as a master element with children
    // Extract DocType from children
    for (const EBMLElement& child : element.children) {
        if (child.id == static_cast<uint32_t>(EBMLElementID::DocType)) {
            element.stringValue = child.stringValue;
            qDebug() << "  DocType:" << element.stringValue;
        }
    }
}

void MKVParser::parseSegmentInfo(QFile& file, EBMLElement& element) {
    // Segment info is parsed as a master element with children
    // Children contain duration, timecode scale, etc.
}

void MKVParser::parseTrackEntry(QFile& file, EBMLElement& element) {
    // Parse track entry children
    for (const EBMLElement& child : element.children) {
        if (child.id == static_cast<uint32_t>(EBMLElementID::TrackNumber)) {
            element.trackNumber = child.uintValue;
        } else if (child.id == static_cast<uint32_t>(EBMLElementID::TrackUID)) {
            element.trackUID = child.uintValue;
        } else if (child.id == static_cast<uint32_t>(EBMLElementID::TrackType)) {
            element.trackType = static_cast<uint8_t>(child.uintValue);
        } else if (child.id == static_cast<uint32_t>(EBMLElementID::CodecID)) {
            element.codecID = child.stringValue;
        } else if (child.id == static_cast<uint32_t>(EBMLElementID::CodecName)) {
            element.codecName = child.stringValue;
        } else if (child.id == static_cast<uint32_t>(EBMLElementID::Video)) {
            parseVideo(file, const_cast<EBMLElement&>(child));
            element.pixelWidth = child.pixelWidth;
            element.pixelHeight = child.pixelHeight;
        } else if (child.id == static_cast<uint32_t>(EBMLElementID::Audio)) {
            parseAudio(file, const_cast<EBMLElement&>(child));
            element.samplingFrequency = child.samplingFrequency;
            element.channels = child.channels;
        }
    }

    qDebug() << "  Track:" << element.trackNumber
             << "Type:" << element.trackType
             << "Codec:" << element.codecID;
}

void MKVParser::parseVideo(QFile& file, EBMLElement& element) {
    for (const EBMLElement& child : element.children) {
        if (child.id == static_cast<uint32_t>(EBMLElementID::PixelWidth)) {
            element.pixelWidth = child.uintValue;
        } else if (child.id == static_cast<uint32_t>(EBMLElementID::PixelHeight)) {
            element.pixelHeight = child.uintValue;
        }
    }
}

void MKVParser::parseAudio(QFile& file, EBMLElement& element) {
    for (const EBMLElement& child : element.children) {
        if (child.id == static_cast<uint32_t>(EBMLElementID::SamplingFrequency)) {
            element.samplingFrequency = child.floatValue;
        } else if (child.id == static_cast<uint32_t>(EBMLElementID::Channels)) {
            element.channels = child.uintValue;
        }
    }
}

uint64_t MKVParser::readUInt(QFile& file, int size) {
    uint64_t value = 0;
    for (int i = 0; i < size && i < 8; ++i) {
        uint8_t byte = 0;
        if (file.read(reinterpret_cast<char*>(&byte), 1) == 1) {
            value = (value << 8) | byte;
        }
    }
    return value;
}

double MKVParser::readFloat(QFile& file, int size) {
    if (size == 4) {
        uint32_t bits = static_cast<uint32_t>(readUInt(file, 4));
        float value;
        memcpy(&value, &bits, sizeof(float));
        return value;
    } else if (size == 8) {
        uint64_t bits = readUInt(file, 8);
        double value;
        memcpy(&value, &bits, sizeof(double));
        return value;
    }
    return 0.0;
}

QString MKVParser::readString(QFile& file, int size) {
    QByteArray data = file.read(size);
    return QString::fromUtf8(data);
}

QByteArray MKVParser::readBinary(QFile& file, int size) {
    return file.read(size);
}

QString MKVParser::getElementName(uint32_t id) const {
    switch (static_cast<EBMLElementID>(id)) {
    case EBMLElementID::EBML: return "EBML";
    case EBMLElementID::EBMLVersion: return "EBMLVersion";
    case EBMLElementID::EBMLReadVersion: return "EBMLReadVersion";
    case EBMLElementID::EBMLMaxIDLength: return "EBMLMaxIDLength";
    case EBMLElementID::EBMLMaxSizeLength: return "EBMLMaxSizeLength";
    case EBMLElementID::DocType: return "DocType";
    case EBMLElementID::DocTypeVersion: return "DocTypeVersion";
    case EBMLElementID::DocTypeReadVersion: return "DocTypeReadVersion";
    case EBMLElementID::Segment: return "Segment";
    case EBMLElementID::SeekHead: return "SeekHead";
    case EBMLElementID::Seek: return "Seek";
    case EBMLElementID::SeekID: return "SeekID";
    case EBMLElementID::SeekPosition: return "SeekPosition";
    case EBMLElementID::Info: return "Info";
    case EBMLElementID::TimecodeScale: return "TimecodeScale";
    case EBMLElementID::Duration: return "Duration";
    case EBMLElementID::MuxingApp: return "MuxingApp";
    case EBMLElementID::WritingApp: return "WritingApp";
    case EBMLElementID::Title: return "Title";
    case EBMLElementID::DateUTC: return "DateUTC";
    case EBMLElementID::Tracks: return "Tracks";
    case EBMLElementID::TrackEntry: return "TrackEntry";
    case EBMLElementID::TrackNumber: return "TrackNumber";
    case EBMLElementID::TrackUID: return "TrackUID";
    case EBMLElementID::TrackType: return "TrackType";
    case EBMLElementID::FlagEnabled: return "FlagEnabled";
    case EBMLElementID::FlagDefault: return "FlagDefault";
    case EBMLElementID::FlagForced: return "FlagForced";
    case EBMLElementID::FlagLacing: return "FlagLacing";
    case EBMLElementID::DefaultDuration: return "DefaultDuration";
    case EBMLElementID::Name: return "Name";
    case EBMLElementID::Language: return "Language";
    case EBMLElementID::CodecID: return "CodecID";
    case EBMLElementID::CodecName: return "CodecName";
    case EBMLElementID::CodecPrivate: return "CodecPrivate";
    case EBMLElementID::Video: return "Video";
    case EBMLElementID::PixelWidth: return "PixelWidth";
    case EBMLElementID::PixelHeight: return "PixelHeight";
    case EBMLElementID::DisplayWidth: return "DisplayWidth";
    case EBMLElementID::DisplayHeight: return "DisplayHeight";
    case EBMLElementID::FrameRate: return "FrameRate";
    case EBMLElementID::Audio: return "Audio";
    case EBMLElementID::SamplingFrequency: return "SamplingFrequency";
    case EBMLElementID::Channels: return "Channels";
    case EBMLElementID::BitDepth: return "BitDepth";
    case EBMLElementID::Cluster: return "Cluster";
    case EBMLElementID::Timecode: return "Timecode";
    case EBMLElementID::SimpleBlock: return "SimpleBlock";
    case EBMLElementID::BlockGroup: return "BlockGroup";
    case EBMLElementID::Block: return "Block";
    case EBMLElementID::Cues: return "Cues";
    case EBMLElementID::CuePoint: return "CuePoint";
    case EBMLElementID::CueTime: return "CueTime";
    case EBMLElementID::CueTrackPositions: return "CueTrackPositions";
    case EBMLElementID::CueTrack: return "CueTrack";
    case EBMLElementID::CueClusterPosition: return "CueClusterPosition";
    case EBMLElementID::Attachments: return "Attachments";
    case EBMLElementID::AttachedFile: return "AttachedFile";
    case EBMLElementID::FileName: return "FileName";
    case EBMLElementID::FileMimeType: return "FileMimeType";
    case EBMLElementID::FileData: return "FileData";
    case EBMLElementID::Chapters: return "Chapters";
    case EBMLElementID::Tags: return "Tags";
    default: return QString("Unknown_0x%1").arg(id, 0, 16);
    }
}

bool MKVParser::isMasterElement(uint32_t id) const {
    switch (static_cast<EBMLElementID>(id)) {
    case EBMLElementID::EBML:
    case EBMLElementID::Segment:
    case EBMLElementID::SeekHead:
    case EBMLElementID::Seek:
    case EBMLElementID::Info:
    case EBMLElementID::Tracks:
    case EBMLElementID::TrackEntry:
    case EBMLElementID::Video:
    case EBMLElementID::Audio:
    case EBMLElementID::Cluster:
    case EBMLElementID::BlockGroup:
    case EBMLElementID::Cues:
    case EBMLElementID::CuePoint:
    case EBMLElementID::CueTrackPositions:
    case EBMLElementID::Attachments:
    case EBMLElementID::AttachedFile:
    case EBMLElementID::Chapters:
    case EBMLElementID::Tags:
        return true;
    default:
        return false;
    }
}

} // namespace VideoStudio

