#include "audioanalyzer.h"
#include <QDebug>
#include <cmath>
#include <algorithm>

namespace VideoStudio {

AudioAnalyzer::AudioAnalyzer()
    : m_formatCtx(nullptr)
    , m_codecCtx(nullptr)
    , m_codec(nullptr)
    , m_swrCtx(nullptr)
    , m_audioStream(nullptr)
    , m_audioStreamIndex(-1)
    , m_packet(nullptr)
    , m_frame(nullptr)
{
    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
}

AudioAnalyzer::~AudioAnalyzer() {
    close();
    av_packet_free(&m_packet);
    av_frame_free(&m_frame);
}

bool AudioAnalyzer::openFile(const QString& filename, int audioStreamIndex) {
    close();

    // Open input file
    int ret = avformat_open_input(&m_formatCtx, filename.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) {
        qWarning() << "AudioAnalyzer: Failed to open file:" << filename;
        return false;
    }

    // Retrieve stream information
    ret = avformat_find_stream_info(m_formatCtx, nullptr);
    if (ret < 0) {
        qWarning() << "AudioAnalyzer: Failed to find stream info";
        avformat_close_input(&m_formatCtx);
        return false;
    }

    // Find audio stream
    if (audioStreamIndex >= 0 && audioStreamIndex < (int)m_formatCtx->nb_streams) {
        if (m_formatCtx->streams[audioStreamIndex]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioStreamIndex = audioStreamIndex;
        }
    }

    if (m_audioStreamIndex < 0) {
        // Find first audio stream
        for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
            if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                m_audioStreamIndex = i;
                break;
            }
        }
    }

    if (m_audioStreamIndex < 0) {
        qWarning() << "AudioAnalyzer: No audio stream found";
        avformat_close_input(&m_formatCtx);
        return false;
    }

    m_audioStream = m_formatCtx->streams[m_audioStreamIndex];
    AVCodecParameters* codecpar = m_audioStream->codecpar;

    // Find decoder
    m_codec = avcodec_find_decoder(codecpar->codec_id);
    if (!m_codec) {
        qWarning() << "AudioAnalyzer: Codec not found";
        avformat_close_input(&m_formatCtx);
        return false;
    }

    // Allocate codec context
    m_codecCtx = avcodec_alloc_context3(m_codec);
    if (!m_codecCtx) {
        qWarning() << "AudioAnalyzer: Failed to allocate codec context";
        avformat_close_input(&m_formatCtx);
        return false;
    }

    // Copy codec parameters to context
    ret = avcodec_parameters_to_context(m_codecCtx, codecpar);
    if (ret < 0) {
        qWarning() << "AudioAnalyzer: Failed to copy codec parameters";
        avcodec_free_context(&m_codecCtx);
        avformat_close_input(&m_formatCtx);
        return false;
    }

    // Open codec
    ret = avcodec_open2(m_codecCtx, m_codec, nullptr);
    if (ret < 0) {
        qWarning() << "AudioAnalyzer: Failed to open codec";
        avcodec_free_context(&m_codecCtx);
        avformat_close_input(&m_formatCtx);
        return false;
    }

    // Initialize resampler
    if (!initResampler()) {
        qWarning() << "AudioAnalyzer: Failed to initialize resampler";
        avcodec_free_context(&m_codecCtx);
        avformat_close_input(&m_formatCtx);
        return false;
    }

    qDebug() << "AudioAnalyzer: Opened audio stream" << m_audioStreamIndex;
    qDebug() << "  Codec:" << m_codec->name;
    qDebug() << "  Sample rate:" << m_codecCtx->sample_rate;
#if LIBAVUTIL_VERSION_MAJOR >= 57
    qDebug() << "  Channels:" << m_codecCtx->ch_layout.nb_channels;
#else
    qDebug() << "  Channels:" << m_codecCtx->channels;
#endif

    return true;
}

void AudioAnalyzer::close() {
    freeResources();
}

void AudioAnalyzer::freeResources() {
    if (m_swrCtx) {
        swr_free(&m_swrCtx);
        m_swrCtx = nullptr;
    }

    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
        m_codecCtx = nullptr;
    }

    if (m_formatCtx) {
        avformat_close_input(&m_formatCtx);
        m_formatCtx = nullptr;
    }

    m_codec = nullptr;
    m_audioStream = nullptr;
    m_audioStreamIndex = -1;
}

bool AudioAnalyzer::initResampler() {
    if (!m_codecCtx) {
        return false;
    }

#if LIBAVUTIL_VERSION_MAJOR >= 57
    // Allocate resampler context (new API)
    int ret = swr_alloc_set_opts2(&m_swrCtx,
                                  &m_codecCtx->ch_layout,
                                  AV_SAMPLE_FMT_FLT,
                                  m_codecCtx->sample_rate,
                                  &m_codecCtx->ch_layout,
                                  m_codecCtx->sample_fmt,
                                  m_codecCtx->sample_rate,
                                  0, nullptr);
#else
    // Allocate resampler context (old API)
    m_swrCtx = swr_alloc_set_opts(nullptr,
                                  m_codecCtx->channel_layout,
                                  AV_SAMPLE_FMT_FLT,
                                  m_codecCtx->sample_rate,
                                  m_codecCtx->channel_layout,
                                  m_codecCtx->sample_fmt,
                                  m_codecCtx->sample_rate,
                                  0, nullptr);
    int ret = m_swrCtx ? 0 : -1;
#endif

    if (ret < 0) {
        qWarning() << "AudioAnalyzer: Failed to allocate resampler";
        return false;
    }

    ret = swr_init(m_swrCtx);
    if (ret < 0) {
        qWarning() << "AudioAnalyzer: Failed to initialize resampler";
        swr_free(&m_swrCtx);
        return false;
    }

    return true;
}

AudioStreamInfo AudioAnalyzer::getStreamInfo() const {
    AudioStreamInfo info;

    if (!m_audioStream || !m_codecCtx) {
        return info;
    }

    info.streamIndex = m_audioStreamIndex;
    info.codecName = QString(m_codec->name);
    info.codecLongName = QString(m_codec->long_name);
    info.sampleRate = m_codecCtx->sample_rate;
#if LIBAVUTIL_VERSION_MAJOR >= 57
    info.channels = m_codecCtx->ch_layout.nb_channels;
#else
    info.channels = m_codecCtx->channels;
#endif
    info.bitrate = m_codecCtx->bit_rate;

    // Channel layout
    char layout[64];
#if LIBAVUTIL_VERSION_MAJOR >= 57
    av_channel_layout_describe(&m_codecCtx->ch_layout, layout, sizeof(layout));
#else
    av_get_channel_layout_string(layout, sizeof(layout), m_codecCtx->channels, m_codecCtx->channel_layout);
#endif
    info.channelLayout = QString(layout);

    // Sample format
    info.sampleFormat = QString(av_get_sample_fmt_name(m_codecCtx->sample_fmt));

    // Duration
    if (m_audioStream->duration != AV_NOPTS_VALUE) {
        info.duration = av_rescale_q(m_audioStream->duration,
                                     m_audioStream->time_base,
                                     AV_TIME_BASE_Q);
    } else if (m_formatCtx->duration != AV_NOPTS_VALUE) {
        info.duration = m_formatCtx->duration;
    }

    // Profile
#ifndef FF_PROFILE_UNKNOWN
#define FF_PROFILE_UNKNOWN -99
#endif
    if (m_codecCtx->profile != FF_PROFILE_UNKNOWN) {
        const char* profile = avcodec_profile_name(m_codecCtx->codec_id, m_codecCtx->profile);
        if (profile) {
            info.profile = QString(profile);
        }
    }

    // Bits per sample
    info.bitsPerSample = av_get_bytes_per_sample(m_codecCtx->sample_fmt) * 8;

    return info;
}

QVector<AudioStreamInfo> AudioAnalyzer::getAllAudioStreams() const {
    QVector<AudioStreamInfo> streams;

    if (!m_formatCtx) {
        return streams;
    }

    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            // Create a temporary analyzer to get info
            // (This is a simplified approach; in production you might cache this)
            AudioStreamInfo info;
            info.streamIndex = i;

            AVCodecParameters* codecpar = m_formatCtx->streams[i]->codecpar;
            const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
            if (codec) {
                info.codecName = QString(codec->name);
                info.codecLongName = QString(codec->long_name);
            }

            info.sampleRate = codecpar->sample_rate;
#if LIBAVUTIL_VERSION_MAJOR >= 57
            info.channels = codecpar->ch_layout.nb_channels;
#else
            info.channels = codecpar->channels;
#endif
            info.bitrate = codecpar->bit_rate;

            char layout[64];
#if LIBAVUTIL_VERSION_MAJOR >= 57
            av_channel_layout_describe(&codecpar->ch_layout, layout, sizeof(layout));
#else
            av_get_channel_layout_string(layout, sizeof(layout), codecpar->channels, codecpar->channel_layout);
#endif
            info.channelLayout = QString(layout);

            info.sampleFormat = QString(av_get_sample_fmt_name((AVSampleFormat)codecpar->format));

            streams.append(info);
        }
    }

    return streams;
}

int AudioAnalyzer::getAudioStreamCount() const {
    if (!m_formatCtx) {
        return 0;
    }

    int count = 0;
    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            count++;
        }
    }
    return count;
}

bool AudioAnalyzer::decodeNextFrame(AudioFrameData& frameData) {
    if (!m_formatCtx || !m_codecCtx) {
        return false;
    }

    while (true) {
        // Read packet
        int ret = av_read_frame(m_formatCtx, m_packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                // End of file
                return false;
            }
            qWarning() << "AudioAnalyzer: Error reading frame";
            return false;
        }

        // Skip non-audio packets
        if (m_packet->stream_index != m_audioStreamIndex) {
            av_packet_unref(m_packet);
            continue;
        }

        // Send packet to decoder
        ret = avcodec_send_packet(m_codecCtx, m_packet);
        av_packet_unref(m_packet);

        if (ret < 0) {
            qWarning() << "AudioAnalyzer: Error sending packet to decoder";
            continue;
        }

        // Receive frame from decoder
        ret = avcodec_receive_frame(m_codecCtx, m_frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            qWarning() << "AudioAnalyzer: Error receiving frame from decoder";
            return false;
        }

        // We have a frame!
        frameData.pts = m_frame->pts;
        frameData.dts = m_frame->pkt_dts;
        frameData.timestamp = m_frame->pts * av_q2d(m_audioStream->time_base);
        frameData.sampleCount = m_frame->nb_samples;
#if LIBAVUTIL_VERSION_MAJOR < 58
        frameData.size = m_frame->pkt_size;
#else
        // pkt_size removed in FFmpeg 5.1+, calculate from sample count
        int channels;
#if LIBAVUTIL_VERSION_MAJOR >= 57
        channels = m_frame->ch_layout.nb_channels;
#else
        channels = m_frame->channels;
#endif
        frameData.size = m_frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)m_frame->format) * channels;
#endif

        // Convert to float samples
        frameData.samples = convertToFloat(m_frame);

        av_frame_unref(m_frame);
        return true;
    }

    return false;
}

QVector<float> AudioAnalyzer::convertToFloat(AVFrame* frame) {
    QVector<float> samples;

    if (!frame || !m_swrCtx) {
        return samples;
    }

#if LIBAVUTIL_VERSION_MAJOR >= 57
    int channels = frame->ch_layout.nb_channels;
#else
    int channels = frame->channels;
#endif
    int sampleCount = frame->nb_samples;

    // Allocate output buffer
    uint8_t* output[1];
    int outSamples = sampleCount;

    int ret = av_samples_alloc(output, nullptr, channels, outSamples,
                               AV_SAMPLE_FMT_FLT, 0);
    if (ret < 0) {
        qWarning() << "AudioAnalyzer: Failed to allocate output buffer";
        return samples;
    }

    // Convert
    ret = swr_convert(m_swrCtx, output, outSamples,
                      (const uint8_t**)frame->data, sampleCount);

    if (ret < 0) {
        qWarning() << "AudioAnalyzer: Failed to convert samples";
        av_freep(&output[0]);
        return samples;
    }

    // Copy to QVector (interleaved)
    float* floatData = (float*)output[0];
    int totalSamples = ret * channels;
    samples.resize(totalSamples);
    for (int i = 0; i < totalSamples; i++) {
        samples[i] = floatData[i];
    }

    av_freep(&output[0]);
    return samples;
}

bool AudioAnalyzer::seekToTime(double timeInSeconds) {
    if (!m_formatCtx || !m_audioStream) {
        return false;
    }

    int64_t timestamp = (int64_t)(timeInSeconds / av_q2d(m_audioStream->time_base));

    int ret = av_seek_frame(m_formatCtx, m_audioStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        qWarning() << "AudioAnalyzer: Seek failed";
        return false;
    }

    avcodec_flush_buffers(m_codecCtx);
    return true;
}

AudioLevelInfo AudioAnalyzer::calculateLevel(const QVector<float>& samples) {
    AudioLevelInfo level;

    if (samples.isEmpty()) {
        return level;
    }

    float sumSquares = 0.0f;
    float peak = 0.0f;

    for (float sample : samples) {
        float abs = std::abs(sample);
        if (abs > peak) {
            peak = abs;
        }
        sumSquares += sample * sample;
    }

    level.peakLevel = peak;
    level.rmsLevel = std::sqrt(sumSquares / samples.size());

    // Convert to dBFS (decibels relative to full scale)
    if (peak > 0.0f) {
        level.dbFS = 20.0f * std::log10(peak);
    } else {
        level.dbFS = -std::numeric_limits<float>::infinity();
    }

    return level;
}

QVector<float> AudioAnalyzer::calculateSpectrum(const QVector<float>& samples, int fftSize) {
    QVector<float> spectrum;

    qDebug() << "calculateSpectrum: samples.size()=" << samples.size() << "fftSize=" << fftSize;

    if (samples.isEmpty() || fftSize < 2) {
        qDebug() << "calculateSpectrum: early return - samples empty or fftSize < 2";
        return spectrum;
    }

    // Ensure fftSize is power of 2
    int actualFFTSize = 1;
    while (actualFFTSize < fftSize) {
        actualFFTSize *= 2;
    }
    fftSize = actualFFTSize;

    qDebug() << "calculateSpectrum: adjusted fftSize=" << fftSize;

    // Need at least fftSize samples
    if (samples.size() < fftSize) {
        qDebug() << "calculateSpectrum: not enough samples - need" << fftSize << "have" << samples.size();
        return spectrum;
    }

    qDebug() << "calculateSpectrum: performing FFT...";

    // Prepare FFT input (with Hanning window)
    QVector<float> real(fftSize);
    QVector<float> imag(fftSize);

    for (int i = 0; i < fftSize; ++i) {
        // Hanning window
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (fftSize - 1)));
        real[i] = samples[i] * window;
        imag[i] = 0.0f;
    }

    // Perform FFT (Cooley-Tukey algorithm)
    performFFT(real, imag, fftSize);

    // Calculate magnitude spectrum in dB
    spectrum.resize(fftSize / 2);
    for (int i = 0; i < fftSize / 2; ++i) {
        float magnitude = std::sqrt(real[i] * real[i] + imag[i] * imag[i]);
        // Convert to dB (with floor at -80 dB)
        float db = 20.0f * std::log10(magnitude + 1e-10f);
        spectrum[i] = qMax(-80.0f, db);
    }

    return spectrum;
}

void AudioAnalyzer::performFFT(QVector<float>& real, QVector<float>& imag, int n) {
    // Bit-reversal permutation
    int j = 0;
    for (int i = 0; i < n - 1; ++i) {
        if (i < j) {
            std::swap(real[i], real[j]);
            std::swap(imag[i], imag[j]);
        }
        int k = n / 2;
        while (k <= j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }

    // Cooley-Tukey FFT
    for (int len = 2; len <= n; len *= 2) {
        float angle = -2.0f * M_PI / len;
        float wlenReal = std::cos(angle);
        float wlenImag = std::sin(angle);

        for (int i = 0; i < n; i += len) {
            float wReal = 1.0f;
            float wImag = 0.0f;

            for (int j = 0; j < len / 2; ++j) {
                float uReal = real[i + j];
                float uImag = imag[i + j];
                float vReal = real[i + j + len / 2];
                float vImag = imag[i + j + len / 2];

                float tReal = wReal * vReal - wImag * vImag;
                float tImag = wReal * vImag + wImag * vReal;

                real[i + j] = uReal + tReal;
                imag[i + j] = uImag + tImag;
                real[i + j + len / 2] = uReal - tReal;
                imag[i + j + len / 2] = uImag - tImag;

                float tempReal = wReal * wlenReal - wImag * wlenImag;
                wImag = wReal * wlenImag + wImag * wlenReal;
                wReal = tempReal;
            }
        }
    }
}

QVector<float> AudioAnalyzer::getWaveformData(double startTime, double endTime, int numSamples) {
    QVector<float> waveform;

    if (!m_formatCtx || !m_codecCtx) {
        return waveform;
    }

    // Seek to start time
    if (!seekToTime(startTime)) {
        return waveform;
    }

    double duration = endTime - startTime;
    double timePerSample = duration / numSamples;

    AudioFrameData frameData;
    double currentTime = startTime;
    int sampleIndex = 0;

    waveform.resize(numSamples);

    while (sampleIndex < numSamples && decodeNextFrame(frameData)) {
        if (frameData.timestamp >= endTime) {
            break;
        }

        if (frameData.timestamp >= currentTime) {
            // Calculate RMS for this time slice
            AudioLevelInfo level = calculateLevel(frameData.samples);
            waveform[sampleIndex] = level.rmsLevel;
            sampleIndex++;
            currentTime += timePerSample;
        }
    }

    // Fill remaining samples with zero if needed
    for (int i = sampleIndex; i < numSamples; i++) {
        waveform[i] = 0.0f;
    }

    return waveform;
}

} // namespace VideoStudio
