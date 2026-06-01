#include "core/aviparser.h"
#include <QDebug>

namespace VideoStudio {

AVIParser::AVIParser()
    : m_fileSize(0)
{
}

AVIParser::~AVIParser() {
}

bool AVIParser::parseFile(const QString& filePath) {
    m_filePath = filePath;
    m_chunks.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open AVI file:" << filePath;
        return false;
    }

    m_fileSize = file.size();
    qDebug() << "Parsing AVI file:" << filePath << "Size:" << m_fileSize << "bytes";

    // Parse from beginning
    bool success = parseChunk(file, 0, m_fileSize, 0, m_chunks);

    file.close();

    if (success) {
        qDebug() << "Parsed" << m_chunks.size() << "top-level chunks";
    }

    return success;
}

bool AVIParser::parseChunk(QFile& file, int64_t offset, int64_t maxSize, int level, QVector<AVIChunk>& chunks) {
    if (offset + 8 > m_fileSize) {
        return true; // End of file
    }

    file.seek(offset);

    AVIChunk chunk;
    chunk.fourCC = readFourCC(file);
    chunk.size = readUInt32(file);
    chunk.offset = offset;
    chunk.totalSize = chunk.size + 8; // Include 8-byte header
    chunk.level = level;
    chunk.percentage = (chunk.totalSize * 100.0) / m_fileSize;

    qDebug() << QString(level * 2, ' ') << "Chunk:" << chunk.fourCC
             << QString("(0x%1)").arg(chunk.fourCC.toLatin1().toHex().constData())
             << "Offset:" << QString("0x%1").arg(offset, 0, 16)
             << "Size:" << chunk.size << "bytes"
             << QString("(%1%)").arg(chunk.percentage, 0, 'f', 2);

    // Check if this is a RIFF or LIST chunk
    if (chunk.fourCC == "RIFF" || chunk.fourCC == "LIST") {
        // Read list type (4 bytes)
        chunk.listType = readFourCC(file);
        qDebug() << QString(level * 2, ' ') << "  Type:" << chunk.listType;

        // Parse children
        int64_t childOffset = offset + 12; // Skip fourCC (4) + size (4) + listType (4)
        int64_t endOffset = offset + 8 + chunk.size;

        while (childOffset < endOffset && childOffset < m_fileSize) {
            if (!parseChunk(file, childOffset, endOffset - childOffset, level + 1, chunk.children)) {
                break;
            }
            if (chunk.children.isEmpty()) {
                break;
            }
            childOffset += chunk.children.last().totalSize;
            // Align to word boundary
            if (childOffset % 2 != 0) {
                childOffset++;
            }
        }
    } else {
        // Parse specific chunk types
        if (chunk.fourCC == "avih") {
            parseAvih(file, chunk);
        } else if (chunk.fourCC == "strh") {
            parseStrh(file, chunk);
        } else if (chunk.fourCC == "strf") {
            parseStrf(file, chunk);
        }
    }

    chunks.append(chunk);
    return true;
}

void AVIParser::parseAvih(QFile& file, AVIChunk& chunk) {
    int64_t startPos = file.pos();

    chunk.microSecPerFrame = readUInt32(file);
    chunk.maxBytesPerSec = readUInt32(file);
    file.seek(startPos + 12); // Skip padding
    file.seek(startPos + 16); // Skip flags
    chunk.totalFrames = readUInt32(file);
    file.seek(startPos + 24); // Skip initial frames
    chunk.streams = readUInt32(file);
    file.seek(startPos + 32); // Skip suggested buffer size
    chunk.width = readUInt32(file);
    chunk.height = readUInt32(file);

    qDebug() << "  MainAVIHeader: " << chunk.width << "x" << chunk.height
             << "Frames:" << chunk.totalFrames
             << "Streams:" << chunk.streams
             << "FPS:" << (1000000.0 / chunk.microSecPerFrame);
}

void AVIParser::parseStrh(QFile& file, AVIChunk& chunk) {
    int64_t startPos = file.pos();

    chunk.streamType = readFourCC(file);
    chunk.codecFourCC = readFourCC(file);
    file.seek(startPos + 12); // Skip flags
    file.seek(startPos + 16); // Skip priority
    file.seek(startPos + 20); // Skip language
    file.seek(startPos + 24); // Skip initial frames
    chunk.scale = readUInt32(file);
    chunk.rate = readUInt32(file);
    file.seek(startPos + 32); // Skip start
    chunk.length = readUInt32(file);
    file.seek(startPos + 40); // Skip suggested buffer size
    file.seek(startPos + 44); // Skip quality
    chunk.sampleSize = readUInt32(file);

    qDebug() << "  StreamHeader: Type:" << chunk.streamType
             << "Codec:" << chunk.codecFourCC
             << "Rate:" << chunk.rate << "/" << chunk.scale
             << "Length:" << chunk.length;
}

void AVIParser::parseStrf(QFile& file, AVIChunk& chunk) {
    // Stream format - varies by stream type
    // For video: BITMAPINFOHEADER
    // For audio: WAVEFORMATEX
    // We'll just note that it exists
    qDebug() << "  StreamFormat: Size:" << chunk.size << "bytes";
}

bool AVIParser::isListChunk(const QString& fourCC) const {
    return fourCC == "RIFF" || fourCC == "LIST";
}

uint32_t AVIParser::readUInt32(QFile& file) {
    uint8_t bytes[4];
    file.read(reinterpret_cast<char*>(bytes), 4);
    return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

uint16_t AVIParser::readUInt16(QFile& file) {
    uint8_t bytes[2];
    file.read(reinterpret_cast<char*>(bytes), 2);
    return bytes[0] | (bytes[1] << 8);
}

QString AVIParser::readFourCC(QFile& file) {
    char fourCC[5] = {0};
    file.read(fourCC, 4);
    return QString::fromLatin1(fourCC, 4);
}

} // namespace VideoStudio
