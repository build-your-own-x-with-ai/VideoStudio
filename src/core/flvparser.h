#ifndef FLVPARSER_H
#define FLVPARSER_H

#include <QString>
#include <QVector>
#include <QFile>
#include <QVariantMap>
#include <cstdint>

namespace VideoStudio {

// FLV Tag Types
enum class FLVTagType : uint8_t {
    Audio = 8,
    Video = 9,
    ScriptData = 18
};

// FLV Video Frame Types
enum class FLVVideoFrameType : uint8_t {
    KeyFrame = 1,
    InterFrame = 2,
    DisposableInterFrame = 3,
    GeneratedKeyFrame = 4,
    VideoInfoFrame = 5
};

// FLV Video Codec IDs
enum class FLVVideoCodec : uint8_t {
    JPEG = 1,
    SorensonH263 = 2,
    ScreenVideo = 3,
    VP6 = 4,
    VP6Alpha = 5,
    ScreenVideoV2 = 6,
    AVC = 7,      // H.264
    HEVC = 12     // H.265
};

// FLV Audio Format
enum class FLVAudioFormat : uint8_t {
    PCM = 0,
    ADPCM = 1,
    MP3 = 2,
    PCM_LE = 3,
    Nellymoser16kHz = 4,
    Nellymoser8kHz = 5,
    Nellymoser = 6,
    G711ALaw = 7,
    G711MuLaw = 8,
    AAC = 10,
    Speex = 11,
    MP3_8kHz = 14,
    DeviceSpecific = 15
};

// FLV Tag structure
struct FLVTag {
    FLVTagType type;
    uint32_t dataSize;
    uint32_t timestamp;
    uint32_t timestampExtended;
    uint32_t streamID;
    int64_t offset;           // File offset of this tag
    int64_t totalSize;        // Total size including header
    double percentage;        // Percentage of file

    // Video-specific fields
    FLVVideoFrameType frameType;
    FLVVideoCodec videoCodec;
    uint8_t avcPacketType;    // 0=sequence header, 1=NALU, 2=end of sequence
    int32_t compositionTime;

    // Audio-specific fields
    FLVAudioFormat audioFormat;
    uint8_t soundRate;        // 0=5.5kHz, 1=11kHz, 2=22kHz, 3=44kHz
    uint8_t soundSize;        // 0=8bit, 1=16bit
    uint8_t soundType;        // 0=mono, 1=stereo
    uint8_t aacPacketType;    // 0=sequence header, 1=raw

    // Script data fields
    QString scriptName;
    QVariantMap scriptData;

    FLVTag()
        : type(FLVTagType::Video)
        , dataSize(0)
        , timestamp(0)
        , timestampExtended(0)
        , streamID(0)
        , offset(0)
        , totalSize(0)
        , percentage(0.0)
        , frameType(FLVVideoFrameType::KeyFrame)
        , videoCodec(FLVVideoCodec::AVC)
        , avcPacketType(0)
        , compositionTime(0)
        , audioFormat(FLVAudioFormat::AAC)
        , soundRate(3)
        , soundSize(1)
        , soundType(1)
        , aacPacketType(0)
    {}
};

// FLV Header structure
struct FLVHeader {
    uint8_t signature[3];     // "FLV"
    uint8_t version;          // Usually 1
    uint8_t flags;            // bit 0: video, bit 2: audio
    uint32_t dataOffset;      // Usually 9

    bool hasVideo() const { return (flags & 0x01) != 0; }
    bool hasAudio() const { return (flags & 0x04) != 0; }
};

class FLVParser {
public:
    FLVParser();
    ~FLVParser();

    bool parseFile(const QString& filePath);

    const FLVHeader& getHeader() const { return m_header; }
    const QVector<FLVTag>& getTags() const { return m_tags; }

    int64_t getFileSize() const { return m_fileSize; }
    QString getFilePath() const { return m_filePath; }

    // Statistics
    int getVideoTagCount() const;
    int getAudioTagCount() const;
    int getScriptDataTagCount() const;
    int getKeyFrameCount() const;

    // Helper functions
    static QString tagTypeToString(FLVTagType type);
    static QString videoCodecToString(FLVVideoCodec codec);
    static QString audioFormatToString(FLVAudioFormat format);
    static QString frameTypeToString(FLVVideoFrameType frameType);

private:
    bool parseHeader(QFile& file);
    bool parseTag(QFile& file, int64_t offset);
    bool parseVideoTag(QFile& file, FLVTag& tag);
    bool parseAudioTag(QFile& file, FLVTag& tag);
    bool parseScriptDataTag(QFile& file, FLVTag& tag);

    // AMF (Action Message Format) parsing for script data
    QVariant parseAMFValue(QFile& file, uint8_t type);
    QString parseAMFString(QFile& file);
    double parseAMFNumber(QFile& file);
    bool parseAMFBoolean(QFile& file);
    QVariantMap parseAMFObject(QFile& file);
    QVariantList parseAMFArray(QFile& file);

    // Helper functions
    uint8_t readUInt8(QFile& file);
    uint16_t readUInt16(QFile& file);
    uint32_t readUInt24(QFile& file);
    uint32_t readUInt32(QFile& file);
    int32_t readInt24(QFile& file);
    double readDouble(QFile& file);

    QString m_filePath;
    int64_t m_fileSize;
    FLVHeader m_header;
    QVector<FLVTag> m_tags;
};

} // namespace VideoStudio

#endif // FLVPARSER_H
