#ifndef AUDIOANALYZER_H
#define AUDIOANALYZER_H

#include <QString>
#include <QVector>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

namespace VideoStudio {

// Audio stream information
struct AudioStreamInfo {
    int streamIndex;
    QString codecName;
    QString codecLongName;
    int sampleRate;
    int channels;
    QString channelLayout;
    int bitrate;
    QString sampleFormat;
    int64_t duration;  // in microseconds
    int64_t frameCount;

    // Codec-specific info
    QString profile;
    int bitsPerSample;

    AudioStreamInfo()
        : streamIndex(-1)
        , sampleRate(0)
        , channels(0)
        , bitrate(0)
        , duration(0)
        , frameCount(0)
        , bitsPerSample(0)
    {}
};

// Audio frame data
struct AudioFrameData {
    int64_t pts;           // Presentation timestamp
    int64_t dts;           // Decode timestamp
    double timestamp;      // In seconds
    int sampleCount;       // Number of samples in this frame
    int size;              // Frame size in bytes
    QVector<float> samples; // Decoded PCM samples (interleaved)

    AudioFrameData()
        : pts(0)
        , dts(0)
        , timestamp(0.0)
        , sampleCount(0)
        , size(0)
    {}
};

// Audio level information
struct AudioLevelInfo {
    float peakLevel;       // Peak level (0.0 - 1.0)
    float rmsLevel;        // RMS (Root Mean Square) level
    float dbFS;            // Decibels relative to full scale

    AudioLevelInfo()
        : peakLevel(0.0f)
        , rmsLevel(0.0f)
        , dbFS(-std::numeric_limits<float>::infinity())
    {}
};

class AudioAnalyzer {
public:
    AudioAnalyzer();
    ~AudioAnalyzer();

    // Open audio stream from file
    bool openFile(const QString& filename, int audioStreamIndex = -1);
    void close();

    // Get audio stream information
    AudioStreamInfo getStreamInfo() const;
    QVector<AudioStreamInfo> getAllAudioStreams() const;

    // Decode audio frames
    bool decodeNextFrame(AudioFrameData& frameData);
    bool seekToTime(double timeInSeconds);

    // Audio analysis
    AudioLevelInfo calculateLevel(const QVector<float>& samples);
    QVector<float> calculateSpectrum(const QVector<float>& samples, int fftSize = 2048);

    // Get waveform data (downsampled for visualization)
    QVector<float> getWaveformData(double startTime, double endTime, int numSamples = 1000);

    bool isOpen() const { return m_formatCtx != nullptr; }
    int getAudioStreamCount() const;

private:
    AVFormatContext* m_formatCtx;
    AVCodecContext* m_codecCtx;
    const AVCodec* m_codec;
    SwrContext* m_swrCtx;
    AVStream* m_audioStream;
    int m_audioStreamIndex;

    AVPacket* m_packet;
    AVFrame* m_frame;

    bool initResampler();
    void freeResources();

    // Convert planar audio to interleaved float
    QVector<float> convertToFloat(AVFrame* frame);
};

} // namespace VideoStudio

#endif // AUDIOANALYZER_H
