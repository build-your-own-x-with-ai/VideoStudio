#include "Exporter.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>

bool Exporter::exportFrameListToCSV(const QString& filePath, const QVector<FrameInfo>& frames) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // CSV 头部
    out << "Frame Number,Frame Type,Size (bytes),Timestamp (s),PTS,DTS,Key Frame\n";

    // 数据行
    for (const auto& frame : frames) {
        out << frame.frameNumber << ","
            << frame.frameType << ","
            << frame.size << ","
            << QString::number(frame.timestamp, 'f', 6) << ","
            << frame.pts << ","
            << frame.dts << ","
            << (frame.isKeyFrame ? "Yes" : "No") << "\n";
    }

    file.close();
    return true;
}

bool Exporter::exportBitrateToCSV(const QString& filePath, const QVector<BitratePoint>& points) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // CSV 头部
    out << "Timestamp (s),Bitrate (bps)\n";

    // 数据行
    for (const auto& point : points) {
        out << QString::number(point.timestamp, 'f', 6) << ","
            << QString::number(point.bitrate, 'f', 2) << "\n";
    }

    file.close();
    return true;
}

bool Exporter::exportGOPToCSV(const QString& filePath, const QVector<GOP>& gops) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // CSV 头部
    out << "GOP Number,Start Frame,End Frame,Size (frames),I Frames,P Frames,B Frames\n";

    // 数据行
    for (int i = 0; i < gops.size(); ++i) {
        const auto& gop = gops[i];
        int iCount = 0, pCount = 0, bCount = 0;

        for (const auto& frame : gop.frames) {
            if (frame.frameType == 'I') iCount++;
            else if (frame.frameType == 'P') pCount++;
            else if (frame.frameType == 'B') bCount++;
        }

        out << (i + 1) << ","
            << gop.startFrame << ","
            << gop.endFrame << ","
            << gop.size << ","
            << iCount << ","
            << pCount << ","
            << bCount << "\n";
    }

    file.close();
    return true;
}

bool Exporter::exportHTMLReport(const QString& filePath,
                                 const QString& videoPath,
                                 const StreamInfo& streamInfo,
                                 MetricsCollector* metricsCollector,
                                 const BitrateStats& bitrateStats,
                                 const GOPStats& gopStats) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // HTML 头部
    out << "<!DOCTYPE html>\n";
    out << "<html lang=\"zh-CN\">\n";
    out << "<head>\n";
    out << "    <meta charset=\"UTF-8\">\n";
    out << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    out << "    <title>VideoStudio 分析报告</title>\n";
    out << "    <style>\n";
    out << "        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }\n";
    out << "        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    out << "        h1 { color: #333; border-bottom: 3px solid #4CAF50; padding-bottom: 10px; }\n";
    out << "        h2 { color: #555; margin-top: 30px; border-bottom: 2px solid #ddd; padding-bottom: 8px; }\n";
    out << "        .info-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; margin: 20px 0; }\n";
    out << "        .info-item { background: #f9f9f9; padding: 15px; border-radius: 5px; border-left: 4px solid #4CAF50; }\n";
    out << "        .info-label { font-weight: bold; color: #666; font-size: 14px; }\n";
    out << "        .info-value { color: #333; font-size: 16px; margin-top: 5px; }\n";
    out << "        .stats-box { background: #e8f5e9; padding: 20px; border-radius: 5px; margin: 15px 0; }\n";
    out << "        .footer { margin-top: 40px; text-align: center; color: #999; font-size: 14px; border-top: 1px solid #ddd; padding-top: 20px; }\n";
    out << "        table { width: 100%; border-collapse: collapse; margin: 15px 0; }\n";
    out << "        th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }\n";
    out << "        th { background: #4CAF50; color: white; font-weight: bold; }\n";
    out << "        tr:hover { background: #f5f5f5; }\n";
    out << "    </style>\n";
    out << "</head>\n";
    out << "<body>\n";
    out << "    <div class=\"container\">\n";

    // 标题
    out << "        <h1>VideoStudio 视频分析报告</h1>\n";
    out << "        <p><strong>视频文件:</strong> " << escapeHTML(videoPath) << "</p>\n";
    out << "        <p><strong>生成时间:</strong> " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "</p>\n";

    // 流信息
    out << "        <h2>流信息</h2>\n";
    out << "        <div class=\"info-grid\">\n";
    out << "            <div class=\"info-item\"><div class=\"info-label\">编码格式</div><div class=\"info-value\">" << escapeHTML(streamInfo.codecName) << "</div></div>\n";
    out << "            <div class=\"info-item\"><div class=\"info-label\">分辨率</div><div class=\"info-value\">" << streamInfo.width << " x " << streamInfo.height << "</div></div>\n";
    out << "            <div class=\"info-item\"><div class=\"info-label\">帧率</div><div class=\"info-value\">" << QString::number(streamInfo.frameRate, 'f', 2) << " fps</div></div>\n";
    out << "            <div class=\"info-item\"><div class=\"info-label\">比特率</div><div class=\"info-value\">" << formatBitrate(streamInfo.bitrate) << "</div></div>\n";
    out << "            <div class=\"info-item\"><div class=\"info-label\">时长</div><div class=\"info-value\">" << QString::number(streamInfo.duration, 'f', 2) << " 秒</div></div>\n";
    out << "            <div class=\"info-item\"><div class=\"info-label\">总帧数</div><div class=\"info-value\">" << streamInfo.numFrames << " 帧</div></div>\n";
    out << "            <div class=\"info-item\"><div class=\"info-label\">像素格式</div><div class=\"info-value\">" << escapeHTML(streamInfo.pixelFormat) << "</div></div>\n";
    out << "            <div class=\"info-item\"><div class=\"info-label\">容器格式</div><div class=\"info-value\">" << escapeHTML(streamInfo.containerFormat) << "</div></div>\n";
    out << "        </div>\n";

    // 帧统计
    out << "        <h2>帧统计</h2>\n";
    out << "        <div class=\"stats-box\">\n";
    out << "            <p><strong>总帧数:</strong> " << metricsCollector->getFrameCount() << " 帧</p>\n";
    out << "            <p><strong>I 帧:</strong> " << metricsCollector->getIFrameCount() << " 帧</p>\n";
    out << "            <p><strong>P 帧:</strong> " << metricsCollector->getPFrameCount() << " 帧</p>\n";
    out << "            <p><strong>B 帧:</strong> " << metricsCollector->getBFrameCount() << " 帧</p>\n";
    out << "        </div>\n";

    // 比特率统计
    out << "        <h2>比特率分析</h2>\n";
    out << "        <div class=\"stats-box\">\n";
    out << "            <p><strong>平均比特率:</strong> " << formatBitrate(bitrateStats.averageBitrate) << "</p>\n";
    out << "            <p><strong>峰值比特率:</strong> " << formatBitrate(bitrateStats.peakBitrate) << "</p>\n";
    out << "            <p><strong>最小比特率:</strong> " << formatBitrate(bitrateStats.minBitrate) << "</p>\n";
    out << "        </div>\n";

    // GOP 统计
    out << "        <h2>GOP 结构分析</h2>\n";
    out << "        <div class=\"stats-box\">\n";
    out << "            <p><strong>总 GOP 数:</strong> " << gopStats.totalGOPs << "</p>\n";
    out << "            <p><strong>平均 GOP 大小:</strong> " << QString::number(gopStats.averageGOPSize, 'f', 1) << " 帧</p>\n";
    out << "            <p><strong>最大 GOP:</strong> " << gopStats.maxGOPSize << " 帧</p>\n";
    out << "            <p><strong>最小 GOP:</strong> " << gopStats.minGOPSize << " 帧</p>\n";
    out << "            <p><strong>关键帧间隔:</strong> " << QString::number(gopStats.averageKeyFrameInterval, 'f', 1) << " 帧</p>\n";
    out << "        </div>\n";

    // 页脚
    out << "        <div class=\"footer\">\n";
    out << "            <p>由 VideoStudio 生成 | 专业视频编解码分析工具</p>\n";
    out << "        </div>\n";
    out << "    </div>\n";
    out << "</body>\n";
    out << "</html>\n";

    file.close();
    return true;
}

QString Exporter::escapeHTML(const QString& text) {
    QString result = text;
    result.replace("&", "&amp;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    result.replace("\"", "&quot;");
    result.replace("'", "&#39;");
    return result;
}

QString Exporter::formatSize(int64_t size) {
    if (size < 1024) {
        return QString("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        return QString("%1 KB").arg(size / 1024.0, 0, 'f', 2);
    } else if (size < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 2);
    } else {
        return QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
}

QString Exporter::formatBitrate(double bitrate) {
    if (bitrate < 1000) {
        return QString("%1 bps").arg(bitrate, 0, 'f', 0);
    } else if (bitrate < 1000000) {
        return QString("%1 kbps").arg(bitrate / 1000.0, 0, 'f', 1);
    } else {
        return QString("%1 Mbps").arg(bitrate / 1000000.0, 0, 'f', 2);
    }
}
