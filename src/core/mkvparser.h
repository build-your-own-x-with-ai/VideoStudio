#ifndef MKVPARSER_H
#define MKVPARSER_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QFile>
#include <cstdint>

namespace VideoStudio {

// EBML Element IDs (common ones)
enum class EBMLElementID : uint32_t {
    // EBML Header
    EBML = 0x1A45DFA3,
    EBMLVersion = 0x4286,
    EBMLReadVersion = 0x42F7,
    EBMLMaxIDLength = 0x42F2,
    EBMLMaxSizeLength = 0x42F3,
    DocType = 0x4282,
    DocTypeVersion = 0x4287,
    DocTypeReadVersion = 0x4285,

    // Segment
    Segment = 0x18538067,

    // Meta Seek Information
    SeekHead = 0x114D9B74,
    Seek = 0x4DBB,
    SeekID = 0x53AB,
    SeekPosition = 0x53AC,

    // Segment Information
    Info = 0x1549A966,
    TimecodeScale = 0x2AD7B1,
    Duration = 0x4489,
    MuxingApp = 0x4D80,
    WritingApp = 0x5741,
    Title = 0x7BA9,
    DateUTC = 0x4461,

    // Track
    Tracks = 0x1654AE6B,
    TrackEntry = 0xAE,
    TrackNumber = 0xD7,
    TrackUID = 0x73C5,
    TrackType = 0x83,
    FlagEnabled = 0xB9,
    FlagDefault = 0x88,
    FlagForced = 0x55AA,
    FlagLacing = 0x9C,
    DefaultDuration = 0x23E383,
    Name = 0x536E,
    Language = 0x22B59C,
    CodecID = 0x86,
    CodecName = 0x258688,
    CodecPrivate = 0x63A2,

    // Video
    Video = 0xE0,
    PixelWidth = 0xB0,
    PixelHeight = 0xBA,
    DisplayWidth = 0x54B0,
    DisplayHeight = 0x54BA,
    FrameRate = 0x2383E3,

    // Audio
    Audio = 0xE1,
    SamplingFrequency = 0xB5,
    Channels = 0x9F,
    BitDepth = 0x6264,

    // Cluster
    Cluster = 0x1F43B675,
    Timecode = 0xE7,
    SimpleBlock = 0xA3,
    BlockGroup = 0xA0,
    Block = 0xA1,

    // Cues
    Cues = 0x1C53BB6B,
    CuePoint = 0xBB,
    CueTime = 0xB3,
    CueTrackPositions = 0xB7,
    CueTrack = 0xF7,
    CueClusterPosition = 0xF1,

    // Attachments
    Attachments = 0x1941A469,
    AttachedFile = 0x61A7,
    FileName = 0x466E,
    FileMimeType = 0x4660,
    FileData = 0x465C,

    // Chapters
    Chapters = 0x1043A770,

    // Tags
    Tags = 0x1254C367,

    // Unknown
    Unknown = 0x00000000
};

struct EBMLElement {
    uint32_t id;                // Element ID
    QString name;               // Element name
    int64_t offset;             // Offset in file
    int64_t headerSize;         // Size of element header (ID + size field)
    int64_t dataSize;           // Size of element data
    int64_t totalSize;          // Total size (header + data)
    double percentage;          // Percentage of total file size
    int level;                  // Nesting level (0 = root)
    QVector<EBMLElement> children; // Child elements

    // Parsed data for specific element types
    QString stringValue;        // For string elements
    uint64_t uintValue;         // For unsigned int elements
    double floatValue;          // For float elements
    QByteArray binaryValue;     // For binary elements

    // Track-specific info
    uint64_t trackNumber;
    uint64_t trackUID;
    uint8_t trackType;          // 1=video, 2=audio, 3=complex, 0x10=logo, 0x11=subtitle, 0x12=buttons, 0x20=control
    QString codecID;
    QString codecName;
    uint64_t pixelWidth;
    uint64_t pixelHeight;
    double samplingFrequency;
    uint64_t channels;
};

class MKVParser {
public:
    MKVParser();
    ~MKVParser();

    // Parse MKV file
    bool parseFile(const QString& filePath);

    // Get parsed elements
    const QVector<EBMLElement>& getElements() const { return m_elements; }

    // Get file info
    int64_t getFileSize() const { return m_fileSize; }
    QString getFilePath() const { return m_filePath; }

private:
    // Parse EBML element recursively
    bool parseElement(QFile& file, int64_t offset, int64_t maxSize, int level, QVector<EBMLElement>& elements);

    // Read EBML variable-length integer
    uint64_t readEBMLVarInt(QFile& file, int& length);
    uint32_t readElementID(QFile& file, int& length);

    // Parse specific element types
    void parseEBMLHeader(QFile& file, EBMLElement& element);
    void parseSegmentInfo(QFile& file, EBMLElement& element);
    void parseTrackEntry(QFile& file, EBMLElement& element);
    void parseVideo(QFile& file, EBMLElement& element);
    void parseAudio(QFile& file, EBMLElement& element);

    // Read data types
    uint64_t readUInt(QFile& file, int size);
    double readFloat(QFile& file, int size);
    QString readString(QFile& file, int size);
    QByteArray readBinary(QFile& file, int size);

    // Get element name from ID
    QString getElementName(uint32_t id) const;

    // Check if element is a master (has children)
    bool isMasterElement(uint32_t id) const;

    QString m_filePath;
    int64_t m_fileSize;
    QVector<EBMLElement> m_elements;
};

} // namespace VideoStudio

#endif // MKVPARSER_H
