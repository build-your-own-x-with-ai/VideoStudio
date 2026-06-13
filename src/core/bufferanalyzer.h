#ifndef BUFFERANALYZER_H
#define BUFFERANALYZER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace VideoStudio {

// Buffer event types
enum class BufferEventType {
    Normal,         // Normal operation
    NearOverflow,   // Approaching overflow (>90%)
    Overflow,       // Buffer overflow
    NearUnderflow,  // Approaching underflow (<10%)
    Underflow       // Buffer underflow
};

// Buffer state at a specific time
struct BufferState {
    int frameNumber;
    double timestamp;           // seconds
    int64_t frameSize;          // bytes
    double bufferOccupancy;     // bytes
    double bufferFullness;      // percentage (0-100)
    BufferEventType eventType;
    double bitrate;             // instantaneous bitrate (bps)

    BufferState()
        : frameNumber(0), timestamp(0.0), frameSize(0),
          bufferOccupancy(0.0), bufferFullness(0.0),
          eventType(BufferEventType::Normal), bitrate(0.0) {}
};

// Buffer analysis result
struct BufferAnalysisResult {
    // Buffer configuration
    int64_t bufferSize;         // CPB size in bytes
    double bitrate;             // Target bitrate in bps
    double fillRate;            // Buffer fill rate (bps)

    // Statistics
    double maxOccupancy;        // Maximum buffer occupancy (bytes)
    double minOccupancy;        // Minimum buffer occupancy (bytes)
    double avgOccupancy;        // Average buffer occupancy (bytes)
    double maxFullness;         // Maximum fullness percentage
    double minFullness;         // Minimum fullness percentage
    double avgFullness;         // Average fullness percentage

    // Events
    int overflowCount;
    int underflowCount;
    int nearOverflowCount;
    int nearUnderflowCount;

    // Bitrate statistics
    double peakBitrate;         // Peak instantaneous bitrate
    double avgBitrate;          // Average bitrate
    double minBitrate;          // Minimum bitrate

    // Delays
    double maxDelay;            // Maximum buffering delay (seconds)
    double avgDelay;            // Average buffering delay (seconds)

    // Buffer states over time
    QVector<BufferState> states;

    BufferAnalysisResult()
        : bufferSize(0), bitrate(0.0), fillRate(0.0),
          maxOccupancy(0.0), minOccupancy(0.0), avgOccupancy(0.0),
          maxFullness(0.0), minFullness(0.0), avgFullness(0.0),
          overflowCount(0), underflowCount(0),
          nearOverflowCount(0), nearUnderflowCount(0),
          peakBitrate(0.0), avgBitrate(0.0), minBitrate(0.0),
          maxDelay(0.0), avgDelay(0.0) {}
};

class BufferAnalyzer : public QObject {
    Q_OBJECT

public:
    explicit BufferAnalyzer(QObject* parent = nullptr);
    ~BufferAnalyzer();

    // Analyze buffer behavior
    bool analyzeFile(const QString& filePath);

    // Get analysis results
    const BufferAnalysisResult& getResult() const { return m_result; }

    // Configuration
    void setBufferSize(int64_t sizeBytes);      // Override CPB size
    void setTargetBitrate(double bitrate);      // Override target bitrate
    void setAutoDetect(bool enable);            // Auto-detect from stream

    // Export results
    QJsonObject toJson() const;
    QString toTextReport() const;

    // Clear results
    void clear();

signals:
    void analysisProgress(int current, int total);
    void analysisComplete();
    void bufferEvent(BufferEventType type, int frameNumber);

private:
    // Analysis functions
    bool analyzeH264Buffer(AVFormatContext* formatCtx, AVCodecContext* codecCtx);
    bool analyzeH265Buffer(AVFormatContext* formatCtx, AVCodecContext* codecCtx);

    // HRD/VBV simulation
    void simulateBuffer(AVFormatContext* formatCtx, int videoStreamIdx);

    // Get buffer parameters from codec
    void extractBufferParams(AVCodecContext* codecCtx);

    // Helper functions
    BufferEventType checkBufferState(double fullness);
    QString eventTypeToString(BufferEventType type) const;

    BufferAnalysisResult m_result;
    QString m_currentFile;

    // Configuration
    bool m_autoDetect;
    int64_t m_overrideBufferSize;
    double m_overrideBitrate;
};

} // namespace VideoStudio

#endif // BUFFERANALYZER_H
