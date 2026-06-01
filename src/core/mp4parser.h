#ifndef MP4PARSER_H
#define MP4PARSER_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QFile>
#include <QObject>
#include <cstdint>

namespace VideoStudio {

struct MP4Atom {
    QString type;           // 4-character atom type (e.g., "ftyp", "moov", "mdat")
    int64_t offset;         // Offset in file
    int64_t size;           // Total size including header
    int64_t headerSize;     // Size of atom header (8 or 16 bytes)
    int64_t dataSize;       // Size of atom data (size - headerSize)
    double percentage;      // Percentage of total file size
    int level;              // Nesting level (0 = root)
    QVector<MP4Atom> children; // Child atoms (for container atoms)

    // Parsed data for specific atom types
    QString majorBrand;     // For ftyp
    QStringList compatibleBrands; // For ftyp
    uint32_t trackId;       // For trak
    QString handlerType;    // For hdlr (vide, soun, hint, meta)
    QString codecType;      // For stsd (avc1, hvc1, mp4a, etc.)
    uint32_t width;         // For video tracks
    uint32_t height;        // For video tracks
    uint32_t sampleRate;    // For audio tracks
    uint16_t channelCount;  // For audio tracks
};

class MP4Parser : public QObject {
    Q_OBJECT

public:
    explicit MP4Parser(QObject* parent = nullptr);
    ~MP4Parser();

    // Parse MP4 file
    bool parseFile(const QString& filePath);

    // Get parsed atoms
    const QVector<MP4Atom>& getAtoms() const { return m_atoms; }

    // Get file info
    int64_t getFileSize() const { return m_fileSize; }
    QString getFilePath() const { return m_filePath; }

signals:
    void logMessage(const QString& message);

private:
    // Parse atom recursively
    bool parseAtom(QFile& file, int64_t offset, int64_t maxSize, int level, QVector<MP4Atom>& atoms);

    // Parse specific atom types
    void parseFtyp(QFile& file, MP4Atom& atom);
    void parseMvhd(QFile& file, MP4Atom& atom);
    void parseTkhd(QFile& file, MP4Atom& atom);
    void parseHdlr(QFile& file, MP4Atom& atom);
    void parseStsd(QFile& file, MP4Atom& atom);

    // Check if atom is a container (has children)
    bool isContainerAtom(const QString& type) const;

    // Read big-endian integers
    uint32_t readUInt32(QFile& file);
    uint64_t readUInt64(QFile& file);
    uint16_t readUInt16(QFile& file);
    QString readString(QFile& file, int length);

    QString m_filePath;
    int64_t m_fileSize;
    QVector<MP4Atom> m_atoms;
};

} // namespace VideoStudio

#endif // MP4PARSER_H
