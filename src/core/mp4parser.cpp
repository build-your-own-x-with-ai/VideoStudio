#include "core/mp4parser.h"
#include <QDebug>

namespace VideoStudio {

MP4Parser::MP4Parser(QObject* parent)
    : QObject(parent)
    , m_fileSize(0)
{
}

MP4Parser::~MP4Parser() {
}

bool MP4Parser::parseFile(const QString& filePath) {
    m_filePath = filePath;
    m_atoms.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit logMessage(QString("Failed to open MP4 file: %1").arg(filePath));
        return false;
    }

    m_fileSize = file.size();
    emit logMessage(QString("Parsing MP4 file: %1 Size: %2 bytes").arg(filePath).arg(m_fileSize));

    // Parse all top-level atoms
    int64_t offset = 0;
    while (offset < m_fileSize) {
        if (!parseAtom(file, offset, m_fileSize - offset, 0, m_atoms)) {
            break;
        }
        if (!m_atoms.isEmpty()) {
            offset = m_atoms.last().offset + m_atoms.last().size;
        } else {
            break;
        }
    }

    file.close();

    emit logMessage(QString("Parsed %1 top-level atoms").arg(m_atoms.size()));
    return !m_atoms.isEmpty();
}

bool MP4Parser::parseAtom(QFile& file, int64_t offset, int64_t maxSize, int level, QVector<MP4Atom>& atoms) {
    if (maxSize < 8) {
        return false;
    }

    file.seek(offset);

    MP4Atom atom;
    atom.offset = offset;
    atom.level = level;

    // Read atom size (4 bytes, big-endian)
    uint32_t size32 = readUInt32(file);

    // Read atom type (4 bytes)
    atom.type = readString(file, 4);

    if (size32 == 1) {
        // Extended size (64-bit)
        atom.size = readUInt64(file);
        atom.headerSize = 16;
    } else if (size32 == 0) {
        // Atom extends to end of file
        atom.size = maxSize;
        atom.headerSize = 8;
    } else {
        atom.size = size32;
        atom.headerSize = 8;
    }

    atom.dataSize = atom.size - atom.headerSize;
    atom.percentage = (double)atom.size / m_fileSize * 100.0;

    emit logMessage(QString("%1Atom: \"%2\" Offset: 0x%3 Size: %4 bytes (%5%)")
        .arg(QString("  ").repeated(level))
        .arg(atom.type)
        .arg(atom.offset, 0, 16)
        .arg(atom.size)
        .arg(atom.percentage, 0, 'f', 2));

    // Parse specific atom types
    if (atom.type == "ftyp") {
        parseFtyp(file, atom);
    } else if (atom.type == "mvhd") {
        parseMvhd(file, atom);
    } else if (atom.type == "tkhd") {
        parseTkhd(file, atom);
    } else if (atom.type == "hdlr") {
        parseHdlr(file, atom);
    } else if (atom.type == "stsd") {
        parseStsd(file, atom);
    }

    // Parse children if this is a container atom
    if (isContainerAtom(atom.type)) {
        int64_t childOffset = offset + atom.headerSize;
        int64_t childMaxSize = atom.dataSize;

        while (childMaxSize > 8) {
            int64_t beforeSize = atom.children.size();
            if (!parseAtom(file, childOffset, childMaxSize, level + 1, atom.children)) {
                break;
            }
            if (atom.children.size() > beforeSize) {
                const MP4Atom& lastChild = atom.children.last();
                int64_t childSize = lastChild.size;
                childOffset += childSize;
                childMaxSize -= childSize;
            } else {
                break;
            }
        }
    }

    atoms.append(atom);
    return true;
}

void MP4Parser::parseFtyp(QFile& file, MP4Atom& atom) {
    // ftyp: major_brand (4), minor_version (4), compatible_brands (4*N)
    atom.majorBrand = readString(file, 4);
    uint32_t minorVersion = readUInt32(file);
    Q_UNUSED(minorVersion);

    int64_t remaining = atom.dataSize - 8;
    while (remaining >= 4) {
        QString brand = readString(file, 4);
        atom.compatibleBrands.append(brand);
        remaining -= 4;
    }

    emit logMessage(QString("  ftyp: major_brand=%1 compatible_brands=%2")
        .arg(atom.majorBrand)
        .arg(atom.compatibleBrands.join(", ")));
}

void MP4Parser::parseMvhd(QFile& file, MP4Atom& atom) {
    // mvhd: version (1), flags (3), ...
    uint8_t version = file.read(1)[0];
    file.read(3); // flags

    if (version == 1) {
        file.read(8); // creation_time
        file.read(8); // modification_time
        uint32_t timescale = readUInt32(file);
        uint64_t duration = readUInt64(file);
        Q_UNUSED(timescale);
        Q_UNUSED(duration);
    } else {
        file.read(4); // creation_time
        file.read(4); // modification_time
        uint32_t timescale = readUInt32(file);
        uint32_t duration = readUInt32(file);
        Q_UNUSED(timescale);
        Q_UNUSED(duration);
    }
}

void MP4Parser::parseTkhd(QFile& file, MP4Atom& atom) {
    // tkhd: version (1), flags (3), ...
    uint8_t version = file.read(1)[0];
    file.read(3); // flags

    if (version == 1) {
        file.read(8); // creation_time
        file.read(8); // modification_time
        atom.trackId = readUInt32(file);
        file.read(4); // reserved
        file.read(8); // duration
    } else {
        file.read(4); // creation_time
        file.read(4); // modification_time
        atom.trackId = readUInt32(file);
        file.read(4); // reserved
        file.read(4); // duration
    }

    // Skip to width/height (at offset 76 for version 0, 88 for version 1)
    int64_t currentPos = file.pos();
    int64_t targetPos = atom.offset + atom.headerSize + (version == 1 ? 88 : 76);
    if (targetPos + 8 <= atom.offset + atom.size) {
        file.seek(targetPos);
        atom.width = readUInt32(file) >> 16;  // Fixed-point 16.16
        atom.height = readUInt32(file) >> 16;
    }

    emit logMessage(QString("  tkhd: track_id=%1 width=%2 height=%3")
        .arg(atom.trackId)
        .arg(atom.width)
        .arg(atom.height));
}

void MP4Parser::parseHdlr(QFile& file, MP4Atom& atom) {
    // hdlr: version (1), flags (3), pre_defined (4), handler_type (4), ...
    file.read(1); // version
    file.read(3); // flags
    file.read(4); // pre_defined
    atom.handlerType = readString(file, 4);

    emit logMessage(QString("  hdlr: handler_type=%1").arg(atom.handlerType));
}

void MP4Parser::parseStsd(QFile& file, MP4Atom& atom) {
    // stsd: version (1), flags (3), entry_count (4), entries...
    file.read(1); // version
    file.read(3); // flags
    uint32_t entryCount = readUInt32(file);

    if (entryCount > 0) {
        // Read first entry
        uint32_t entrySize = readUInt32(file);
        atom.codecType = readString(file, 4);

        emit logMessage(QString("  stsd: entry_count=%1 codec=%2 entry_size=%3")
            .arg(entryCount)
            .arg(atom.codecType)
            .arg(entrySize));

        // For video codecs, try to read width/height
        if (atom.codecType == "avc1" || atom.codecType == "hvc1" ||
            atom.codecType == "mp4v" || atom.codecType == "hev1") {
            file.read(6); // reserved
            file.read(2); // data_reference_index
            file.read(16); // pre_defined + reserved
            atom.width = readUInt16(file);
            atom.height = readUInt16(file);
            emit logMessage(QString("    Video: width=%1 height=%2").arg(atom.width).arg(atom.height));
        }
        // For audio codecs
        else if (atom.codecType == "mp4a" || atom.codecType == "ac-3" ||
                 atom.codecType == "ec-3") {
            file.read(6); // reserved
            file.read(2); // data_reference_index
            file.read(8); // reserved
            atom.channelCount = readUInt16(file);
            file.read(2); // sample_size
            file.read(4); // pre_defined + reserved
            atom.sampleRate = readUInt32(file) >> 16; // Fixed-point 16.16
            emit logMessage(QString("    Audio: channels=%1 sample_rate=%2")
                .arg(atom.channelCount)
                .arg(atom.sampleRate));
        }
    }
}

bool MP4Parser::isContainerAtom(const QString& type) const {
    // Container atoms that have children
    static const QStringList containers = {
        "moov", "trak", "mdia", "minf", "stbl", "edts", "dinf",
        "udta", "mvex", "moof", "traf", "mfra", "skip", "meta",
        "ipro", "sinf", "fiin", "paen", "meco", "mere"
    };
    return containers.contains(type);
}

uint32_t MP4Parser::readUInt32(QFile& file) {
    QByteArray data = file.read(4);
    if (data.size() != 4) return 0;
    return (static_cast<uint8_t>(data[0]) << 24) |
           (static_cast<uint8_t>(data[1]) << 16) |
           (static_cast<uint8_t>(data[2]) << 8) |
           static_cast<uint8_t>(data[3]);
}

uint64_t MP4Parser::readUInt64(QFile& file) {
    QByteArray data = file.read(8);
    if (data.size() != 8) return 0;
    return (static_cast<uint64_t>(static_cast<uint8_t>(data[0])) << 56) |
           (static_cast<uint64_t>(static_cast<uint8_t>(data[1])) << 48) |
           (static_cast<uint64_t>(static_cast<uint8_t>(data[2])) << 40) |
           (static_cast<uint64_t>(static_cast<uint8_t>(data[3])) << 32) |
           (static_cast<uint64_t>(static_cast<uint8_t>(data[4])) << 24) |
           (static_cast<uint64_t>(static_cast<uint8_t>(data[5])) << 16) |
           (static_cast<uint64_t>(static_cast<uint8_t>(data[6])) << 8) |
           static_cast<uint64_t>(static_cast<uint8_t>(data[7]));
}

uint16_t MP4Parser::readUInt16(QFile& file) {
    QByteArray data = file.read(2);
    if (data.size() != 2) return 0;
    return (static_cast<uint8_t>(data[0]) << 8) |
           static_cast<uint8_t>(data[1]);
}

QString MP4Parser::readString(QFile& file, int length) {
    QByteArray data = file.read(length);
    return QString::fromLatin1(data);
}

} // namespace VideoStudio
