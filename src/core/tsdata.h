#ifndef TSDATA_H
#define TSDATA_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <cstdint>

namespace VideoStudio {

// Transport Stream packet structure (188 bytes)
struct TSPacket {
    int64_t offset;                     // File offset
    uint8_t syncByte;                   // Should be 0x47
    bool transportErrorIndicator;
    bool payloadUnitStartIndicator;
    bool transportPriority;
    uint16_t pid;                       // Packet ID
    uint8_t scramblingControl;
    uint8_t adaptationFieldControl;
    uint8_t continuityCounter;

    // Adaptation field
    bool hasAdaptationField;
    bool discontinuityIndicator;
    bool randomAccessIndicator;
    bool elementaryStreamPriorityIndicator;
    bool hasPCR;
    int64_t pcr;                        // Program Clock Reference
    bool hasPTS;
    int64_t pts;                        // Presentation Time Stamp
    bool hasDTS;
    int64_t dts;                        // Decode Time Stamp

    QByteArray payload;

    TSPacket()
        : offset(0), syncByte(0x47), transportErrorIndicator(false),
          payloadUnitStartIndicator(false), transportPriority(false),
          pid(0), scramblingControl(0), adaptationFieldControl(0),
          continuityCounter(0), hasAdaptationField(false),
          discontinuityIndicator(false), randomAccessIndicator(false),
          elementaryStreamPriorityIndicator(false), hasPCR(false), pcr(0),
          hasPTS(false), pts(0), hasDTS(false), dts(0) {}
};

// PID information
struct PIDInfo {
    uint16_t pid;
    QString type;                       // Video, Audio, PMT, PAT, etc.
    QString codec;                      // H.264, AAC, etc.
    int packetCount;
    double percentage;
    int64_t totalBytes;
    uint8_t lastContinuityCounter;

    PIDInfo()
        : pid(0), packetCount(0), percentage(0.0),
          totalBytes(0), lastContinuityCounter(0xFF) {}
};

// PSI/SI Table types
enum class PSITableType {
    PAT,        // Program Association Table
    PMT,        // Program Map Table
    CAT,        // Conditional Access Table
    NIT,        // Network Information Table
    SDT,        // Service Description Table
    EIT,        // Event Information Table
    TOT,        // Time Offset Table
    TDT,        // Time and Date Table
    BAT,        // Bouquet Association Table
    RST,        // Running Status Table
    Unknown
};

// PSI/SI Table
struct PSITable {
    PSITableType type;
    uint16_t pid;
    int64_t offset;
    uint8_t tableId;
    uint16_t sectionLength;
    QByteArray data;

    PSITable()
        : type(PSITableType::Unknown), pid(0), offset(0),
          tableId(0), sectionLength(0) {}
};

// Program information
struct ProgramInfo {
    uint16_t programNumber;
    uint16_t pmtPid;
    QString serviceName;
    QString providerName;
    QVector<uint16_t> elementaryPIDs;

    ProgramInfo()
        : programNumber(0), pmtPid(0) {}
};

// Elementary stream info
struct ElementaryStreamInfo {
    uint8_t streamType;
    uint16_t elementaryPID;
    QString codecName;

    ElementaryStreamInfo()
        : streamType(0), elementaryPID(0) {}
};

// Timing point for charts (PTS/DTS/PCR over time)
struct TimingPoint {
    int64_t offset;         // File offset
    int packetIndex;        // Packet index
    int64_t timestamp;      // PTS/DTS/PCR value (in 90kHz units)
    double timeSeconds;     // Time in seconds

    TimingPoint()
        : offset(0), packetIndex(0), timestamp(0), timeSeconds(0.0) {}

    TimingPoint(int64_t off, int pktIdx, int64_t ts)
        : offset(off), packetIndex(pktIdx), timestamp(ts),
          timeSeconds(ts / 90000.0) {}
};

// PCR accuracy measurement
struct PCRAccuracy {
    int64_t offset;
    int packetIndex;
    int64_t actualPCR;
    int64_t expectedPCR;
    int64_t difference;     // In 27MHz units (actualPCR - expectedPCR)
    double differenceNs;    // In nanoseconds

    PCRAccuracy()
        : offset(0), packetIndex(0), actualPCR(0), expectedPCR(0),
          difference(0), differenceNs(0.0) {}
};

// Timing statistics for a PID
struct TimingStats {
    uint16_t pid;
    int ptsCount;
    int dtsCount;
    int pcrCount;
    int64_t minPTS;
    int64_t maxPTS;
    int64_t minDTS;
    int64_t maxDTS;
    int64_t minPCR;
    int64_t maxPCR;
    double avgPTSInterval;  // Average interval between PTS (in seconds)
    double avgDTSInterval;
    double avgPCRInterval;
    double pcrJitter;       // PCR jitter (standard deviation)

    TimingStats()
        : pid(0), ptsCount(0), dtsCount(0), pcrCount(0),
          minPTS(INT64_MAX), maxPTS(INT64_MIN),
          minDTS(INT64_MAX), maxDTS(INT64_MIN),
          minPCR(INT64_MAX), maxPCR(INT64_MIN),
          avgPTSInterval(0.0), avgDTSInterval(0.0), avgPCRInterval(0.0),
          pcrJitter(0.0) {}
};

} // namespace VideoStudio

#endif // TSDATA_H
