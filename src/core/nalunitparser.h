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
    void parseAudioFrames(const QString& filePath, AVFormatContext* formatContext, int audioStreamIndex);

    // Format detection
    enum BitstreamFormat {
        FORMAT_AVCC,    // Length-prefixed (MP4)
        FORMAT_ANNEXB   // Start code-prefixed (0x000001)
    };
    BitstreamFormat detectFormat(const QByteArray& data) const;

    // NAL header parsing
    void parseH264NALHeader(const uint8_t* data, int size, NALUnitInfo& info);
    void parseH265NALHeader(const uint8_t* data, int size, NALUnitInfo& info);

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
