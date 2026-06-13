#include "bufferanalyzer.h"
#include <QDebug>
#include <QFile>
#include <cmath>

namespace VideoStudio {

BufferAnalyzer::BufferAnalyzer(QObject* parent)
    : QObject(parent)
    , m_autoDetect(true)
    , m_overrideBufferSize(0)
    , m_overrideBitrate(0.0)
{
}

BufferAnalyzer::~BufferAnalyzer() = default;

bool BufferAnalyzer::analyzeFile(const QString& filePath) {
    clear();
    m_currentFile = filePath;

    AVFormatContext* formatCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;

    // Open input file
    int ret = avformat_open_input(&formatCtx, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) {
        qWarning() << "Failed to open file:" << filePath;
        return false;
    }

    // Find stream info
    ret = avformat_find_stream_info(formatCtx, nullptr);
    if (ret < 0) {
        avformat_close_input(&formatCtx);
        return false;
    }

    // Find video stream
    int videoStreamIdx = -1;
    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIdx = i;
            break;
        }
    }

    if (videoStreamIdx == -1) {
        avformat_close_input(&formatCtx);
        return false;
    }

    // Get codec context
    AVCodecParameters* codecPar = formatCtx->streams[videoStreamIdx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        avformat_close_input(&formatCtx);
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);
    avcodec_open2(codecCtx, codec, nullptr);

    // Extract buffer parameters
    if (m_autoDetect) {
        extractBufferParams(codecCtx);
    } else {
        m_result.bufferSize = m_overrideBufferSize;
        m_result.bitrate = m_overrideBitrate;
    }

    m_result.fillRate = m_result.bitrate;

    // Perform buffer simulation
    simulateBuffer(formatCtx, videoStreamIdx);

    // Cleanup
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);

    emit analysisComplete();
    return true;
}

void BufferAnalyzer::extractBufferParams(AVCodecContext* codecCtx) {
    // Get buffer size from codec context
    if (codecCtx->rc_buffer_size > 0) {
        m_result.bufferSize = codecCtx->rc_buffer_size;
    } else {
        // Use default based on level (for H.264)
        if (codecCtx->codec_id == AV_CODEC_ID_H264) {
            // Level 4.0 default: 25 Mbits = 3.125 MB
            m_result.bufferSize = 3125000;
        } else if (codecCtx->codec_id == AV_CODEC_ID_HEVC) {
            // Level 4.0 default: 30 Mbits = 3.75 MB
            m_result.bufferSize = 3750000;
        } else {
            m_result.bufferSize = 2000000; // 2 MB default
        }
    }

    // Get bitrate
    if (codecCtx->bit_rate > 0) {
        m_result.bitrate = codecCtx->bit_rate;
    } else {
        // Estimate from file
        m_result.bitrate = 5000000; // 5 Mbps default
    }

    qDebug() << "Buffer parameters - Size:" << m_result.bufferSize
             << "bytes, Bitrate:" << m_result.bitrate << "bps";
}

void BufferAnalyzer::simulateBuffer(AVFormatContext* formatCtx, int videoStreamIdx) {
    AVPacket* packet = av_packet_alloc();
    AVStream* stream = formatCtx->streams[videoStreamIdx];

    double currentBufferOccupancy = 0.0;  // bytes
    double lastTimestamp = 0.0;
    int frameCount = 0;

    double totalOccupancy = 0.0;
    double totalBitrate = 0.0;
    m_result.minOccupancy = m_result.bufferSize;
    m_result.minBitrate = 1e9;

    // Simulate buffer filling and draining
    while (av_read_frame(formatCtx, packet) >= 0) {
        if (packet->stream_index != videoStreamIdx) {
            av_packet_unref(packet);
            continue;
        }

        frameCount++;
        emit analysisProgress(frameCount, 1000); // Estimate 1000 frames

        // Calculate timestamp
        double timestamp = 0.0;
        if (packet->pts != AV_NOPTS_VALUE) {
            timestamp = packet->pts * av_q2d(stream->time_base);
        } else if (packet->dts != AV_NOPTS_VALUE) {
            timestamp = packet->dts * av_q2d(stream->time_base);
        }

        double timeDelta = timestamp - lastTimestamp;
        if (lastTimestamp == 0.0 || timeDelta <= 0.0) {
            timeDelta = 1.0 / 30.0; // Assume 30fps if no timestamp
        }

        // Buffer fills at constant rate (bitrate)
        double bitsAdded = m_result.fillRate * timeDelta;
        currentBufferOccupancy += bitsAdded / 8.0;

        // Frame is removed from buffer
        int64_t frameSize = packet->size;
        currentBufferOccupancy -= frameSize;

        // Clamp to valid range
        if (currentBufferOccupancy < 0) {
            currentBufferOccupancy = 0;
        }
        if (currentBufferOccupancy > m_result.bufferSize) {
            currentBufferOccupancy = m_result.bufferSize;
        }

        // Calculate fullness percentage
        double fullness = (currentBufferOccupancy / m_result.bufferSize) * 100.0;

        // Calculate instantaneous bitrate
        double instantBitrate = (frameSize * 8.0) / timeDelta;

        // Update statistics
        if (currentBufferOccupancy > m_result.maxOccupancy) {
            m_result.maxOccupancy = currentBufferOccupancy;
        }
        if (currentBufferOccupancy < m_result.minOccupancy) {
            m_result.minOccupancy = currentBufferOccupancy;
        }
        if (fullness > m_result.maxFullness) {
            m_result.maxFullness = fullness;
        }
        if (fullness < m_result.minFullness || m_result.minFullness == 0.0) {
            m_result.minFullness = fullness;
        }
        if (instantBitrate > m_result.peakBitrate) {
            m_result.peakBitrate = instantBitrate;
        }
        if (instantBitrate < m_result.minBitrate) {
            m_result.minBitrate = instantBitrate;
        }

        totalOccupancy += currentBufferOccupancy;
        totalBitrate += instantBitrate;

        // Check for events
        BufferEventType eventType = checkBufferState(fullness);

        switch (eventType) {
            case BufferEventType::Overflow:
                m_result.overflowCount++;
                emit bufferEvent(eventType, frameCount);
                break;
            case BufferEventType::Underflow:
                m_result.underflowCount++;
                emit bufferEvent(eventType, frameCount);
                break;
            case BufferEventType::NearOverflow:
                m_result.nearOverflowCount++;
                break;
            case BufferEventType::NearUnderflow:
                m_result.nearUnderflowCount++;
                break;
            default:
                break;
        }

        // Store state (sample every 10th frame to reduce memory)
        if (frameCount % 10 == 0) {
            BufferState state;
            state.frameNumber = frameCount;
            state.timestamp = timestamp;
            state.frameSize = frameSize;
            state.bufferOccupancy = currentBufferOccupancy;
            state.bufferFullness = fullness;
            state.eventType = eventType;
            state.bitrate = instantBitrate;
            m_result.states.append(state);
        }

        lastTimestamp = timestamp;
        av_packet_unref(packet);

        // Limit analysis to reasonable frame count
        if (frameCount >= 10000) {
            break;
        }
    }

    av_packet_free(&packet);

    // Calculate averages
    if (frameCount > 0) {
        m_result.avgOccupancy = totalOccupancy / frameCount;
        m_result.avgFullness = (m_result.avgOccupancy / m_result.bufferSize) * 100.0;
        m_result.avgBitrate = totalBitrate / frameCount;
    }

    // Calculate delays (buffer occupancy / drain rate)
    m_result.maxDelay = (m_result.maxOccupancy * 8.0) / m_result.bitrate;
    m_result.avgDelay = (m_result.avgOccupancy * 8.0) / m_result.bitrate;

    qDebug() << "Buffer analysis complete:" << frameCount << "frames analyzed";
    qDebug() << "Overflows:" << m_result.overflowCount
             << "Underflows:" << m_result.underflowCount;
}

BufferEventType BufferAnalyzer::checkBufferState(double fullness) {
    if (fullness >= 100.0) {
        return BufferEventType::Overflow;
    } else if (fullness >= 90.0) {
        return BufferEventType::NearOverflow;
    } else if (fullness <= 0.0) {
        return BufferEventType::Underflow;
    } else if (fullness <= 10.0) {
        return BufferEventType::NearUnderflow;
    }
    return BufferEventType::Normal;
}

QString BufferAnalyzer::eventTypeToString(BufferEventType type) const {
    switch (type) {
        case BufferEventType::Normal: return "Normal";
        case BufferEventType::NearOverflow: return "Near Overflow";
        case BufferEventType::Overflow: return "Overflow";
        case BufferEventType::NearUnderflow: return "Near Underflow";
        case BufferEventType::Underflow: return "Underflow";
        default: return "Unknown";
    }
}

void BufferAnalyzer::setBufferSize(int64_t sizeBytes) {
    m_overrideBufferSize = sizeBytes;
    m_autoDetect = false;
}

void BufferAnalyzer::setTargetBitrate(double bitrate) {
    m_overrideBitrate = bitrate;
    m_autoDetect = false;
}

void BufferAnalyzer::setAutoDetect(bool enable) {
    m_autoDetect = enable;
}

QJsonObject BufferAnalyzer::toJson() const {
    QJsonObject root;
    root["file"] = m_currentFile;
    root["buffer_size_bytes"] = QString::number(m_result.bufferSize);
    root["buffer_size_mbits"] = (m_result.bufferSize * 8.0) / 1000000.0;
    root["target_bitrate_bps"] = m_result.bitrate;
    root["target_bitrate_mbps"] = m_result.bitrate / 1000000.0;

    // Statistics
    QJsonObject stats;
    stats["max_occupancy_bytes"] = m_result.maxOccupancy;
    stats["min_occupancy_bytes"] = m_result.minOccupancy;
    stats["avg_occupancy_bytes"] = m_result.avgOccupancy;
    stats["max_fullness_percent"] = m_result.maxFullness;
    stats["min_fullness_percent"] = m_result.minFullness;
    stats["avg_fullness_percent"] = m_result.avgFullness;
    root["statistics"] = stats;

    // Events
    QJsonObject events;
    events["overflows"] = m_result.overflowCount;
    events["underflows"] = m_result.underflowCount;
    events["near_overflows"] = m_result.nearOverflowCount;
    events["near_underflows"] = m_result.nearUnderflowCount;
    root["events"] = events;

    // Bitrate
    QJsonObject bitrate;
    bitrate["peak_mbps"] = m_result.peakBitrate / 1000000.0;
    bitrate["avg_mbps"] = m_result.avgBitrate / 1000000.0;
    bitrate["min_mbps"] = m_result.minBitrate / 1000000.0;
    root["bitrate"] = bitrate;

    // Delays
    QJsonObject delay;
    delay["max_seconds"] = m_result.maxDelay;
    delay["avg_seconds"] = m_result.avgDelay;
    root["delay"] = delay;

    return root;
}

QString BufferAnalyzer::toTextReport() const {
    QString report;
    QTextStream stream(&report);

    stream << "===========================================\n";
    stream << "      HRD/VBV Buffer Analysis Report\n";
    stream << "===========================================\n\n";
    stream << "File: " << m_currentFile << "\n\n";

    stream << "Buffer Configuration:\n";
    stream << "  Buffer Size: " << QString::number(m_result.bufferSize) << " bytes ("
           << QString::number((m_result.bufferSize * 8.0) / 1000000.0, 'f', 2) << " Mbits)\n";
    stream << "  Target Bitrate: " << QString::number(m_result.bitrate / 1000000.0, 'f', 2)
           << " Mbps\n\n";

    stream << "Buffer Occupancy Statistics:\n";
    stream << "  Maximum: " << QString::number(m_result.maxOccupancy, 'f', 0) << " bytes ("
           << QString::number(m_result.maxFullness, 'f', 1) << "%)\n";
    stream << "  Minimum: " << QString::number(m_result.minOccupancy, 'f', 0) << " bytes ("
           << QString::number(m_result.minFullness, 'f', 1) << "%)\n";
    stream << "  Average: " << QString::number(m_result.avgOccupancy, 'f', 0) << " bytes ("
           << QString::number(m_result.avgFullness, 'f', 1) << "%)\n\n";

    stream << "Bitrate Statistics:\n";
    stream << "  Peak: " << QString::number(m_result.peakBitrate / 1000000.0, 'f', 2) << " Mbps\n";
    stream << "  Average: " << QString::number(m_result.avgBitrate / 1000000.0, 'f', 2) << " Mbps\n";
    stream << "  Minimum: " << QString::number(m_result.minBitrate / 1000000.0, 'f', 2) << " Mbps\n\n";

    stream << "Buffer Events:\n";
    stream << "  Overflows: " << m_result.overflowCount << "\n";
    stream << "  Underflows: " << m_result.underflowCount << "\n";
    stream << "  Near Overflows (>90%): " << m_result.nearOverflowCount << "\n";
    stream << "  Near Underflows (<10%): " << m_result.nearUnderflowCount << "\n\n";

    stream << "Buffering Delays:\n";
    stream << "  Maximum: " << QString::number(m_result.maxDelay, 'f', 3) << " seconds\n";
    stream << "  Average: " << QString::number(m_result.avgDelay, 'f', 3) << " seconds\n\n";

    // Compliance assessment
    stream << "-------------------------------------------\n";
    stream << "Compliance Assessment:\n";
    stream << "-------------------------------------------\n\n";

    if (m_result.overflowCount > 0) {
        stream << "[ERROR] Buffer overflow detected (" << m_result.overflowCount << " times)\n";
        stream << "  This violates HRD/VBV constraints and may cause decoder failures.\n\n";
    }

    if (m_result.underflowCount > 0) {
        stream << "[ERROR] Buffer underflow detected (" << m_result.underflowCount << " times)\n";
        stream << "  This may cause decoder stalls or playback interruptions.\n\n";
    }

    if (m_result.nearOverflowCount > 10) {
        stream << "[WARNING] Frequent near-overflow conditions (" << m_result.nearOverflowCount << " times)\n";
        stream << "  Consider increasing buffer size or reducing peak bitrate.\n\n";
    }

    if (m_result.overflowCount == 0 && m_result.underflowCount == 0) {
        stream << "[PASS] No buffer violations detected.\n";
        stream << "  The stream complies with HRD/VBV constraints.\n\n";
    }

    return report;
}

void BufferAnalyzer::clear() {
    m_result = BufferAnalysisResult();
    m_currentFile.clear();
}

} // namespace VideoStudio
