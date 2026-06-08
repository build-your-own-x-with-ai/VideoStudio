#ifndef NALUNITPARSER_H
#define NALUNITPARSER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QFile>
#include "naldata.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace VideoStudio {

class VideoDecoder;

class NALUnitParser : public QObject {
    Q_OBJECT

public:
    explicit NALUnitParser(QObject* parent = nullptr);
    ~NALUnitParser();

    // Parse all NAL units from video file
    bool parseFile(const QString& filePath, VideoDecoder* decoder, const QString& containerType);

    // Getters
    const QVector<NALUnitInfo>& getNALUnits() const { return m_nalUnits; }
    const NALUnitInfo* getNALUnit(int index) const;
    int getNALUnitCount() const { return m_nalUnits.size(); }

    const QVector<AudioFrameInfo>& getAudioFrames() const { return m_audioFrames; }
    const AudioFrameInfo* getAudioFrame(int index) const;
    int getAudioFrameCount() const { return m_audioFrames.size(); }

    QString getFilePath() const { return m_filePath; }

    // Clear parsed data
    void clear();

signals:
    void parseProgress(int current, int total, const QString& status);
    void parseComplete();
    void parseError(const QString& error);

private:
    // Extract NAL units from a single frame
    QVector<NALUnitInfo> extractNALUnits(const QByteArray& frameData, AVCodecID codecId,
                                          int frameNumber, int64_t frameOffset);

    // Parse audio frames
    void parseAudioFrames(const QString& filePath, AVFormatContext* formatContext, int audioStreamIndex, AVCodecID codecId);
    void parseAudioFramesFromFile(const QString& filePath);

    // Parse audio codec headers
    void parseADTSHeader(const uint8_t* data, int size, AudioFrameInfo& info);
    void parseAC3Header(const uint8_t* data, int size, AudioFrameInfo& info);
    void parseMP3Header(const uint8_t* data, int size, AudioFrameInfo& info);

    // Format detection
    enum BitstreamFormat {
        FORMAT_AVCC,    // Length-prefixed (MP4)
        FORMAT_ANNEXB   // Start code-prefixed (0x000001)
    };
    BitstreamFormat detectFormat(const QByteArray& data) const;

    // NAL header parsing
    void parseH264NALHeader(const uint8_t* data, int size, NALUnitInfo& info);
    void parseH265NALHeader(const uint8_t* data, int size, NALUnitInfo& info);

    // Detailed NAL unit parsing
    void parseH264SPS(const uint8_t* data, int size, NALUnitInfo& info);
    void parseH264PPS(const uint8_t* data, int size, NALUnitInfo& info);
    void parseH265VPS(const uint8_t* data, int size, NALUnitInfo& info);
    void parseH265SPS(const uint8_t* data, int size, NALUnitInfo& info);

    // Slice header parsing
    void parseH264SliceHeader(const uint8_t* data, int size, NALUnitInfo& info);
    void parseH265SliceHeader(const uint8_t* data, int size, NALUnitInfo& info);

    // Extradata parsing (avcC/hvcC from MP4/MKV)
    QVector<NALUnitInfo> parseAVCCExtradata(const uint8_t* data, int size, AVCodecID codecId);
    QVector<NALUnitInfo> parseHVCCExtradata(const uint8_t* data, int size, AVCodecID codecId);

    // Exp-Golomb decoding helpers
    uint32_t readUE(const uint8_t* data, int& bitPos, int maxBits);  // Read unsigned Exp-Golomb
    int32_t readSE(const uint8_t* data, int& bitPos, int maxBits);   // Read signed Exp-Golomb
    uint32_t readBits(const uint8_t* data, int& bitPos, int numBits); // Read n bits

    // Type name helpers
    QString getH264NALTypeName(int type) const;
    QString getH265NALTypeName(int type) const;

    // Data
    QVector<NALUnitInfo> m_nalUnits;
    QVector<AudioFrameInfo> m_audioFrames;
    QString m_filePath;
};

} // namespace VideoStudio

#endif // NALUNITPARSER_H
