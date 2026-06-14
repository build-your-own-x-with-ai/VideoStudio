#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include "core/videodecoder.h"
#include "core/framedata.h"
#include "core/compliancevalidator.h"
#include "core/bufferanalyzer.h"
#include "plugins/pluginmanager.h"

extern "C" {
#include <libavutil/frame.h>
}

namespace VideoStudio {

class CLIProcessor : public QObject {
    Q_OBJECT

public:
    CLIProcessor(QCoreApplication* app) : QObject(nullptr), m_app(app) {}

    int run(const QStringList& args);

private:
    int processInfo(const QString& videoFile, const QString& format);
    int processFrames(const QString& videoFile, const QString& outputFile,
                      int startFrame, int endFrame);
    int processGOP(const QString& videoFile, const QString& outputFile);
    int processCompliance(const QString& videoFile, const QString& outputFile);
    int processBuffer(const QString& videoFile, const QString& outputFile);
    int processPluginList();
    int processPluginRun(const QString& pluginId, const QString& videoFile, const QString& outputFile);

    void printUsage();
    void printError(const QString& message);
    void printInfo(const QString& message);

    QJsonObject videoInfoToJson(VideoDecoder* decoder);
    QString videoInfoToText(VideoDecoder* decoder);
    QString videoInfoToCSV(VideoDecoder* decoder);

    QCoreApplication* m_app;
};

int CLIProcessor::run(const QStringList& args) {
    QCommandLineParser parser;
    parser.setApplicationDescription("VideoStudio Command Line Tool - Professional video stream analysis");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("command",
        "Command to execute: info, frames, gop, compliance, buffer, plugin");
    parser.addPositionalArgument("file", "Input video file");

    QCommandLineOption outputOption(QStringList() << "o" << "output",
        "Output file path", "file");
    parser.addOption(outputOption);

    QCommandLineOption formatOption(QStringList() << "f" << "format",
        "Output format: json, csv, text (default: json)", "format", "json");
    parser.addOption(formatOption);

    QCommandLineOption startFrameOption("start-frame",
        "Start frame number (default: 0)", "frame", "0");
    parser.addOption(startFrameOption);

    QCommandLineOption endFrameOption("end-frame",
        "End frame number (default: end of video)", "frame", "0");
    parser.addOption(endFrameOption);

    parser.process(args);

    const QStringList positionalArgs = parser.positionalArguments();
    if (positionalArgs.isEmpty()) {
        printUsage();
        return 1;
    }

    QString command = positionalArgs.at(0).toLower();

    if (command == "info") {
        if (positionalArgs.size() < 2) {
            printError("Missing video file argument");
            return 1;
        }
        QString videoFile = positionalArgs.at(1);
        QString format = parser.value(formatOption);
        return processInfo(videoFile, format);
    }
    else if (command == "frames") {
        if (positionalArgs.size() < 2) {
            printError("Missing video file argument");
            return 1;
        }
        QString videoFile = positionalArgs.at(1);
        QString outputFile = parser.value(outputOption);
        int startFrame = parser.value(startFrameOption).toInt();
        int endFrame = parser.value(endFrameOption).toInt();

        return processFrames(videoFile, outputFile, startFrame, endFrame);
    }
    else if (command == "gop") {
        if (positionalArgs.size() < 2) {
            printError("Missing video file argument");
            return 1;
        }
        QString videoFile = positionalArgs.at(1);
        QString outputFile = parser.value(outputOption);

        return processGOP(videoFile, outputFile);
    }
    else if (command == "compliance") {
        if (positionalArgs.size() < 2) {
            printError("Missing video file argument");
            return 1;
        }
        QString videoFile = positionalArgs.at(1);
        QString outputFile = parser.value(outputOption);

        return processCompliance(videoFile, outputFile);
    }
    else if (command == "buffer") {
        if (positionalArgs.size() < 2) {
            printError("Missing video file argument");
            return 1;
        }
        QString videoFile = positionalArgs.at(1);
        QString outputFile = parser.value(outputOption);

        return processBuffer(videoFile, outputFile);
    }
    else if (command == "plugin") {
        // plugin list OR plugin run <id> <file>
        if (positionalArgs.size() < 2) {
            printError("Missing plugin subcommand (list or run)");
            return 1;
        }

        QString subcommand = positionalArgs.at(1);
        if (subcommand == "list") {
            return processPluginList();
        } else if (subcommand == "run") {
            if (positionalArgs.size() < 4) {
                printError("Usage: plugin run <plugin-id> <video-file>");
                return 1;
            }
            QString pluginId = positionalArgs.at(2);
            QString videoFile = positionalArgs.at(3);
            QString outputFile = parser.value(outputOption);
            return processPluginRun(pluginId, videoFile, outputFile);
        } else {
            printError(QString("Unknown plugin subcommand: %1").arg(subcommand));
            return 1;
        }
    }
    else {
        printError(QString("Unknown command: %1").arg(command));
        printUsage();
        return 1;
    }

    return 0;
}

void CLIProcessor::printUsage() {
    QTextStream out(stdout);
    out << "VideoStudio CLI - Professional video stream analysis\n\n";
    out << "Usage: videostudio-cli <command> [options]\n\n";
    out << "Commands:\n";
    out << "  info <file>        Extract video information\n";
    out << "  frames <file>      Export frame-level metrics to CSV\n";
    out << "  gop <file>         Analyze GOP structure (JSON output)\n";
    out << "  compliance <file>  Validate H.264/H.265 compliance\n";
    out << "  buffer <file>      Analyze HRD/VBV buffer behavior\n";
    out << "  plugin list        List available plugins\n";
    out << "  plugin run <id> <file>  Run a plugin on video file\n\n";
    out << "Options:\n";
    out << "  -o, --output <file>      Output file (default: stdout)\n";
    out << "  -f, --format <format>    Output format for info: json|csv|text\n";
    out << "  --start-frame <N>        Start frame for frames command\n";
    out << "  --end-frame <N>          End frame for frames command\n";
}

void CLIProcessor::printError(const QString& message) {
    QTextStream err(stderr);
    err << "Error: " << message << "\n";
}

void CLIProcessor::printInfo(const QString& message) {
    QTextStream out(stdout);
    out << message << "\n";
}

int CLIProcessor::processInfo(const QString& videoFile, const QString& format) {
    QFileInfo fileInfo(videoFile);
    if (!fileInfo.exists()) {
        printError(QString("File not found: %1").arg(videoFile));
        return 1;
    }

    auto decoder = std::make_unique<VideoDecoder>();
    if (!decoder->openFile(videoFile)) {
        printError("Failed to open video file");
        return 1;
    }

    // openFile() already calls buildFrameIndex() internally

    QString output;
    if (format == "json") {
        QJsonObject json = videoInfoToJson(decoder.get());
        QJsonDocument doc(json);
        output = doc.toJson(QJsonDocument::Indented);
    } else if (format == "csv") {
        output = videoInfoToCSV(decoder.get());
    } else {
        output = videoInfoToText(decoder.get());
    }

    QTextStream out(stdout);
    out << output;

    return 0;
}

int CLIProcessor::processFrames(const QString& videoFile, const QString& outputFile,
                                 int startFrame, int endFrame) {
    QFileInfo fileInfo(videoFile);
    if (!fileInfo.exists()) {
        printError(QString("File not found: %1").arg(videoFile));
        return 1;
    }

    auto decoder = std::make_unique<VideoDecoder>();
    if (!decoder->openFile(videoFile)) {
        printError("Failed to open video file");
        return 1;
    }

    // openFile() already calls buildFrameIndex() internally

    const FrameIndex& frameIndex = decoder->getFrameIndex();
    int totalFrames = frameIndex.frameCount();

    if (endFrame == 0 || endFrame > totalFrames) {
        endFrame = totalFrames;
    }

    QStringList lines;
    lines << "frame,type,size,pts,dts,qp,keyframe";

    for (int i = startFrame; i < endFrame; ++i) {
        const FrameInfo* frame = frameIndex.getFrame(i);
        if (!frame) continue;

        QString frameType;
        switch (frame->frameType) {
            case AV_PICTURE_TYPE_I: frameType = "I"; break;
            case AV_PICTURE_TYPE_P: frameType = "P"; break;
            case AV_PICTURE_TYPE_B: frameType = "B"; break;
            default: frameType = "?"; break;
        }

        QStringList cols;
        cols << QString::number(i);
        cols << frameType;
        cols << QString::number(frame->size);
        cols << QString::number(frame->pts);
        cols << QString::number(frame->dts);
        cols << QString::number(frame->qp);
        cols << (frame->isKeyFrame ? "1" : "0");

        lines << cols.join(",");
    }

    QString output = lines.join("\n") + "\n";

    if (outputFile.isEmpty()) {
        QTextStream out(stdout);
        out << output;
    } else {
        QFile file(outputFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            printError(QString("Failed to write to: %1").arg(outputFile));
            return 1;
        }
        QTextStream out(&file);
        out << output;
        printInfo(QString("Exported %1 frames to: %2").arg(endFrame - startFrame).arg(outputFile));
    }

    return 0;
}

int CLIProcessor::processGOP(const QString& videoFile, const QString& outputFile) {
    QFileInfo fileInfo(videoFile);
    if (!fileInfo.exists()) {
        printError(QString("File not found: %1").arg(videoFile));
        return 1;
    }

    auto decoder = std::make_unique<VideoDecoder>();
    if (!decoder->openFile(videoFile)) {
        printError("Failed to open video file");
        return 1;
    }

    // openFile() already calls buildFrameIndex() internally

    const FrameIndex& frameIndex = decoder->getFrameIndex();
    QJsonArray gopsArray;
    int gopNumber = 0;
    int gopStartFrame = 0;
    int iFrameCount = 0, pFrameCount = 0, bFrameCount = 0;

    for (int i = 0; i < frameIndex.frameCount(); ++i) {
        const FrameInfo* frame = frameIndex.getFrame(i);
        if (!frame) continue;

        if (frame->isKeyFrame && i > 0) {
            QJsonObject gopObj;
            gopObj["gop_number"] = gopNumber;
            gopObj["start_frame"] = gopStartFrame;
            gopObj["end_frame"] = i - 1;
            gopObj["length"] = i - gopStartFrame;
            gopObj["i_frames"] = iFrameCount;
            gopObj["p_frames"] = pFrameCount;
            gopObj["b_frames"] = bFrameCount;
            gopsArray.append(gopObj);

            gopNumber++;
            gopStartFrame = i;
            iFrameCount = 0;
            pFrameCount = 0;
            bFrameCount = 0;
        }

        switch (frame->frameType) {
            case AV_PICTURE_TYPE_I: iFrameCount++; break;
            case AV_PICTURE_TYPE_P: pFrameCount++; break;
            case AV_PICTURE_TYPE_B: bFrameCount++; break;
            default: break;
        }
    }

    if (gopStartFrame < frameIndex.frameCount()) {
        QJsonObject gopObj;
        gopObj["gop_number"] = gopNumber;
        gopObj["start_frame"] = gopStartFrame;
        gopObj["end_frame"] = frameIndex.frameCount() - 1;
        gopObj["length"] = frameIndex.frameCount() - gopStartFrame;
        gopObj["i_frames"] = iFrameCount;
        gopObj["p_frames"] = pFrameCount;
        gopObj["b_frames"] = bFrameCount;
        gopsArray.append(gopObj);
    }

    QJsonObject root;
    root["file"] = videoFile;
    root["total_frames"] = frameIndex.frameCount();
    root["total_gops"] = gopsArray.size();
    root["gops"] = gopsArray;

    QJsonDocument doc(root);
    QString output = doc.toJson(QJsonDocument::Indented);

    if (outputFile.isEmpty()) {
        QTextStream out(stdout);
        out << output;
    } else {
        QFile file(outputFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            printError(QString("Failed to write to: %1").arg(outputFile));
            return 1;
        }
        QTextStream out(&file);
        out << output;
        printInfo(QString("GOP analysis saved to: %1").arg(outputFile));
    }

    return 0;
}

QJsonObject CLIProcessor::videoInfoToJson(VideoDecoder* decoder) {
    QJsonObject root;
    root["file_name"] = decoder->getFileName();
    root["codec"] = decoder->getCodecName();
    root["width"] = decoder->getWidth();
    root["height"] = decoder->getHeight();
    root["frame_rate"] = decoder->getFrameRate();
    root["duration"] = decoder->getDuration();
    root["total_frames"] = decoder->getFrameCount();
    root["bitrate"] = decoder->getBitrate();
    root["pixel_format"] = decoder->getPixelFormat();
    return root;
}

QString CLIProcessor::videoInfoToText(VideoDecoder* decoder) {
    QString text;
    QTextStream stream(&text);
    stream << "Video Information\n";
    stream << "=================\n";
    stream << "File:         " << decoder->getFileName() << "\n";
    stream << "Codec:        " << decoder->getCodecName() << "\n";
    stream << "Resolution:   " << decoder->getWidth() << "x" << decoder->getHeight() << "\n";
    stream << "Frame Rate:   " << QString::number(decoder->getFrameRate(), 'f', 2) << " fps\n";
    stream << "Duration:     " << QString::number(decoder->getDuration(), 'f', 2) << " seconds\n";
    stream << "Total Frames: " << decoder->getFrameCount() << "\n";
    stream << "Bitrate:      " << decoder->getBitrate() << " kb/s\n";
    stream << "Pixel Format: " << decoder->getPixelFormat() << "\n";
    return text;
}

QString CLIProcessor::videoInfoToCSV(VideoDecoder* decoder) {
    QStringList lines;
    lines << "property,value";
    lines << QString("file_name,%1").arg(decoder->getFileName());
    lines << QString("codec,%1").arg(decoder->getCodecName());
    lines << QString("width,%1").arg(decoder->getWidth());
    lines << QString("height,%1").arg(decoder->getHeight());
    lines << QString("frame_rate,%1").arg(decoder->getFrameRate());
    lines << QString("duration,%1").arg(decoder->getDuration());
    lines << QString("total_frames,%1").arg(decoder->getFrameCount());
    lines << QString("bitrate,%1").arg(decoder->getBitrate());
    lines << QString("pixel_format,%1").arg(decoder->getPixelFormat());
    return lines.join("\n") + "\n";
}

int CLIProcessor::processCompliance(const QString& videoFile, const QString& outputFile) {
    QFileInfo fileInfo(videoFile);
    if (!fileInfo.exists()) {
        printError(QString("File not found: %1").arg(videoFile));
        return 1;
    }

    printInfo(QString("Validating H.264/H.265 compliance: %1").arg(videoFile));

    auto validator = std::make_unique<ComplianceValidator>();

    if (!validator->validateFile(videoFile)) {
        printError("Validation failed");
        return 1;
    }

    // Generate report
    QString output;
    if (outputFile.isEmpty() || outputFile.endsWith(".txt")) {
        output = validator->toTextReport();
    } else {
        QJsonDocument doc(validator->toJson());
        output = doc.toJson(QJsonDocument::Indented);
    }

    if (outputFile.isEmpty()) {
        QTextStream out(stdout);
        out << output;
    } else {
        QFile file(outputFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            printError(QString("Failed to write to: %1").arg(outputFile));
            return 1;
        }
        QTextStream out(&file);
        out << output;
        printInfo(QString("Compliance report saved to: %1").arg(outputFile));
    }

    // Print summary to stderr
    int errors = validator->getErrorCount();
    int warnings = validator->getWarningCount();

    QTextStream err(stderr);
    err << "\n";
    err << "Validation Summary:\n";
    err << "  Errors:   " << errors << "\n";
    err << "  Warnings: " << warnings << "\n";
    err << "  Info:     " << validator->getInfoCount() << "\n";

    return (errors > 0) ? 1 : 0;
}

int CLIProcessor::processBuffer(const QString& videoFile, const QString& outputFile) {
    QFileInfo fileInfo(videoFile);
    if (!fileInfo.exists()) {
        printError(QString("File not found: %1").arg(videoFile));
        return 1;
    }

    printInfo(QString("Analyzing HRD/VBV buffer: %1").arg(videoFile));

    auto analyzer = std::make_unique<BufferAnalyzer>();

    if (!analyzer->analyzeFile(videoFile)) {
        printError("Buffer analysis failed");
        return 1;
    }

    // Generate report
    QString output;
    if (outputFile.isEmpty() || outputFile.endsWith(".txt")) {
        output = analyzer->toTextReport();
    } else {
        QJsonDocument doc(analyzer->toJson());
        output = doc.toJson(QJsonDocument::Indented);
    }

    if (outputFile.isEmpty()) {
        QTextStream out(stdout);
        out << output;
    } else {
        QFile file(outputFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            printError(QString("Failed to write to: %1").arg(outputFile));
            return 1;
        }
        QTextStream out(&file);
        out << output;
        printInfo(QString("Buffer analysis report saved to: %1").arg(outputFile));
    }

    // Check for violations
    const auto& result = analyzer->getResult();
    int violations = result.overflowCount + result.underflowCount;

    QTextStream err(stderr);
    err << "\n";
    err << "Buffer Analysis Summary:\n";
    err << "  Overflows:  " << result.overflowCount << "\n";
    err << "  Underflows: " << result.underflowCount << "\n";
    if (violations == 0) {
        err << "  Status: PASS\n";
    } else {
        err << "  Status: FAIL (" << violations << " violations)\n";
    }

    return (violations > 0) ? 1 : 0;
}

int CLIProcessor::processPluginList() {
    PluginManager& manager = PluginManager::instance();

    // Scan for plugins
    QString pluginsDir = manager.getPluginsDirectory();
    printInfo(QString("Scanning for plugins in: %1").arg(pluginsDir));
    manager.scanPlugins(pluginsDir);

    QVector<PluginMetadata> plugins = manager.getPluginMetadataList();

    if (plugins.isEmpty()) {
        printInfo("No plugins found.");
        return 0;
    }

    QTextStream out(stdout);
    out << "\nAvailable Plugins:\n";
    out << "==================\n\n";

    for (const PluginMetadata& metadata : plugins) {
        out << "ID:          " << metadata.id << "\n";
        out << "Name:        " << metadata.name << "\n";
        out << "Version:     " << metadata.version << "\n";
        out << "Author:      " << metadata.author << "\n";
        out << "Category:    " << metadata.category << "\n";
        out << "Description: " << metadata.description << "\n";
        if (!metadata.tags.isEmpty()) {
            out << "Tags:        " << metadata.tags.join(", ") << "\n";
        }
        out << "\n";
    }

    out << "Total plugins: " << plugins.size() << "\n";

    return 0;
}

int CLIProcessor::processPluginRun(const QString& pluginId, const QString& videoFile, const QString& outputFile) {
    QFileInfo fileInfo(videoFile);
    if (!fileInfo.exists()) {
        printError(QString("File not found: %1").arg(videoFile));
        return 1;
    }

    PluginManager& manager = PluginManager::instance();

    // Scan for plugins if not already done
    if (manager.getPluginCount() == 0) {
        QString pluginsDir = manager.getPluginsDirectory();
        manager.scanPlugins(pluginsDir);
    }

    if (!manager.hasPlugin(pluginId)) {
        printError(QString("Plugin not found: %1").arg(pluginId));
        printInfo("Use 'plugin list' to see available plugins");
        return 1;
    }

    printInfo(QString("Running plugin: %1").arg(pluginId));
    printInfo(QString("Analyzing: %1").arg(videoFile));

    // Run plugin
    AnalysisResult result = manager.runPlugin(pluginId, videoFile);

    if (!result.success) {
        printError(QString("Plugin execution failed: %1").arg(result.error));
        return 1;
    }

    // Output results
    QString output;
    if (outputFile.isEmpty() || outputFile.endsWith(".txt")) {
        output = result.textReport;
    } else {
        QJsonDocument doc(result.data);
        output = doc.toJson(QJsonDocument::Indented);
    }

    if (outputFile.isEmpty()) {
        QTextStream out(stdout);
        out << output;
    } else {
        QFile file(outputFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            printError(QString("Failed to write to: %1").arg(outputFile));
            return 1;
        }
        QTextStream out(&file);
        out << output;
        printInfo(QString("Plugin report saved to: %1").arg(outputFile));
    }

    return 0;
}

} // namespace VideoStudio

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("VideoStudio CLI");
    app.setApplicationVersion("1.2.0");
    app.setOrganizationName("VideoStudio");

    VideoStudio::CLIProcessor processor(&app);
    return processor.run(app.arguments());
}

#include "main_cli.moc"
