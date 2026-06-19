#include "sampleanalyzerplugin.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace VideoStudio {

SampleAnalyzerPlugin::SampleAnalyzerPlugin()
    : m_initialized(false)
{
}

SampleAnalyzerPlugin::~SampleAnalyzerPlugin() {
    cleanup();
}

PluginMetadata SampleAnalyzerPlugin::getMetadata() const {
    PluginMetadata metadata;
    metadata.id = "com.videostudio.sampleanalyzer";
    metadata.name = "Sample Analyzer";
    metadata.version = "1.0.0";
    metadata.author = "VideoStudio Team";
    metadata.description = "Sample plugin demonstrating custom analysis capabilities";
    metadata.category = "Demo";
    metadata.tags << "sample" << "demo" << "example";
    return metadata;
}

bool SampleAnalyzerPlugin::initialize() {
    qDebug() << "SampleAnalyzerPlugin: Initializing...";
    m_initialized = true;
    return true;
}

void SampleAnalyzerPlugin::cleanup() {
    qDebug() << "SampleAnalyzerPlugin: Cleaning up...";
    m_initialized = false;
}

AnalysisResult SampleAnalyzerPlugin::analyze(const QString& videoFile, const QVariantMap& options) {
    AnalysisResult result;

    if (!m_initialized) {
        result.success = false;
        result.error = "Plugin not initialized";
        return result;
    }

    QFileInfo fileInfo(videoFile);
    if (!fileInfo.exists()) {
        result.success = false;
        result.error = QString("File not found: %1").arg(videoFile);
        return result;
    }

    // Open video file
    AVFormatContext* formatCtx = nullptr;
    int ret = avformat_open_input(&formatCtx, videoFile.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) {
        result.success = false;
        result.error = "Failed to open video file";
        return result;
    }

    ret = avformat_find_stream_info(formatCtx, nullptr);
    if (ret < 0) {
        avformat_close_input(&formatCtx);
        result.success = false;
        result.error = "Failed to find stream info";
        return result;
    }

    // Perform custom analysis
    QJsonObject data;
    data["file_name"] = videoFile;
    data["file_size_bytes"] = fileInfo.size();
    data["format"] = QString(formatCtx->iformat->name);
    data["duration_seconds"] = formatCtx->duration / (double)AV_TIME_BASE;
    data["num_streams"] = (int)formatCtx->nb_streams;

    // Analyze streams
    QJsonArray streams;
    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        AVStream* stream = formatCtx->streams[i];
        AVCodecParameters* codecpar = stream->codecpar;

        QJsonObject streamObj;
        streamObj["index"] = (int)i;
        streamObj["type"] = av_get_media_type_string(codecpar->codec_type);

        const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
        if (codec) {
            streamObj["codec"] = QString(codec->name);
        }

        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            streamObj["width"] = codecpar->width;
            streamObj["height"] = codecpar->height;
            streamObj["fps"] = av_q2d(stream->avg_frame_rate);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            streamObj["sample_rate"] = codecpar->sample_rate;
#if LIBAVCODEC_VERSION_MAJOR >= 59
            streamObj["channels"] = codecpar->ch_layout.nb_channels;
#else
            streamObj["channels"] = codecpar->channels;
#endif
        }

        streams.append(streamObj);
    }
    data["streams"] = streams;

    // Custom metric: Calculate average packet size
    int64_t totalSize = 0;
    int packetCount = 0;
    AVPacket* packet = av_packet_alloc();

    while (av_read_frame(formatCtx, packet) >= 0 && packetCount < 1000) {
        totalSize += packet->size;
        packetCount++;
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    avformat_close_input(&formatCtx);

    if (packetCount > 0) {
        data["avg_packet_size"] = (double)totalSize / packetCount;
        data["packets_analyzed"] = packetCount;
    }

    // Check options
    if (options.contains("custom_threshold")) {
        int threshold = options["custom_threshold"].toInt();
        data["custom_threshold"] = threshold;
        data["threshold_check"] = (packetCount > threshold) ? "PASS" : "FAIL";
    }

    result.success = true;
    result.data = data;

    // Generate text report
    QString report;
    report += "Sample Analysis Report\n";
    report += "======================\n\n";
    report += QString("File: %1\n").arg(videoFile);
    report += QString("Format: %1\n").arg(data["format"].toString());
    report += QString("Duration: %1 seconds\n").arg(data["duration_seconds"].toDouble());
    report += QString("Streams: %1\n").arg(data["num_streams"].toInt());
    report += QString("Packets analyzed: %1\n").arg(packetCount);
    report += QString("Average packet size: %1 bytes\n").arg(data["avg_packet_size"].toDouble(), 0, 'f', 2);

    result.textReport = report;

    return result;
}

QVariantMap SampleAnalyzerPlugin::getDefaultOptions() const {
    QVariantMap options;
    options["custom_threshold"] = 100;
    return options;
}

} // namespace VideoStudio
