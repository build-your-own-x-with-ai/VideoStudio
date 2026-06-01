#ifndef AVIPARSER_H
#define AVIPARSER_H

#include <QString>
#include <QVector>
#include <QFile>
#include <cstdint>

namespace VideoStudio {

struct AVIChunk {
    QString fourCC;         // 4-character chunk ID (e.g., "RIFF", "LIST", "avih")
    int64_t offset;         // Offset in file
    int64_t size;           // Chunk data size (not including 8-byte header)
    int64_t totalSize;      // Total size including header
    double percentage;      // Percentage of total file size
    int level;              // Nesting level (0 = root)
    QVector<AVIChunk> children; // Child chunks (for LIST chunks)

    // Parsed data for specific chunk types
    QString listType;       // For LIST chunks (hdrl, strl, movi, etc.)
    uint32_t microSecPerFrame;  // For avih
    uint32_t maxBytesPerSec;    // For avih
    uint32_t totalFrames;       // For avih
    uint32_t width;             // For avih
    uint32_t height;            // For avih
    uint32_t streams;           // For avih

    // Stream header (strh)
    QString streamType;     // vids, auds, txts, etc.
    QString codecFourCC;    // Codec identifier
    uint32_t scale;         // For frame rate calculation
    uint32_t rate;          // For frame rate calculation
    uint32_t length;        // Stream length in scale units
    uint32_t sampleSize;    // For audio
};

class AVIParser {
public:
    AVIParser();
    ~AVIParser();

    // Parse AVI file
    bool parseFile(const QString& filePath);

    // Get parsed chunks
    const QVector<AVIChunk>& getChunks() const { return m_chunks; }

    // Get file info
    int64_t getFileSize() const { return m_fileSize; }
    QString getFilePath() const { return m_filePath; }

private:
    // Parse chunk recursively
    bool parseChunk(QFile& file, int64_t offset, int64_t maxSize, int level, QVector<AVIChunk>& chunks);

    // Parse specific chunk types
    void parseAvih(QFile& file, AVIChunk& chunk);
    void parseStrh(QFile& file, AVIChunk& chunk);
    void parseStrf(QFile& file, AVIChunk& chunk);

    // Check if chunk is a LIST (has children)
    bool isListChunk(const QString& fourCC) const;

    // Read little-endian integers
    uint32_t readUInt32(QFile& file);
    uint16_t readUInt16(QFile& file);
    QString readFourCC(QFile& file);

    QString m_filePath;
    int64_t m_fileSize;
    QVector<AVIChunk> m_chunks;
};

} // namespace VideoStudio

#endif // AVIPARSER_H
