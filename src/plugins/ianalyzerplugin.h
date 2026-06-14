#ifndef IANALYZERPLUGIN_H
#define IANALYZERPLUGIN_H

#include <QString>
#include <QVariant>
#include <QJsonObject>
#include <QObject>

namespace VideoStudio {

// Plugin metadata
struct PluginMetadata {
    QString id;             // Unique identifier (e.g., "com.example.customplugin")
    QString name;           // Display name
    QString version;        // Version string (e.g., "1.0.0")
    QString author;         // Author name
    QString description;    // Brief description
    QString category;       // Category (e.g., "Quality", "Bitstream", "Custom")
    QStringList tags;       // Search tags

    PluginMetadata()
        : version("1.0.0"), category("Custom") {}
};

// Analysis result from plugin
struct AnalysisResult {
    bool success;
    QString error;
    QJsonObject data;       // Plugin-specific result data
    QString textReport;     // Human-readable report

    AnalysisResult()
        : success(false) {}
};

// Plugin interface - all plugins must implement this
class IAnalyzerPlugin {
public:
    virtual ~IAnalyzerPlugin() = default;

    // Plugin metadata
    virtual PluginMetadata getMetadata() const = 0;

    // Initialize plugin (called once after loading)
    virtual bool initialize() = 0;

    // Cleanup plugin (called before unloading)
    virtual void cleanup() = 0;

    // Analyze video file
    virtual AnalysisResult analyze(const QString& videoFile, const QVariantMap& options = QVariantMap()) = 0;

    // Get configuration UI (optional, returns null if no UI)
    virtual QWidget* createConfigWidget(QWidget* parent = nullptr) {
        Q_UNUSED(parent);
        return nullptr;
    }

    // Get available options for this plugin
    virtual QVariantMap getDefaultOptions() const {
        return QVariantMap();
    }

    // Validate options
    virtual bool validateOptions(const QVariantMap& options, QString& error) const {
        Q_UNUSED(options);
        Q_UNUSED(error);
        return true;
    }

    // Get plugin capabilities
    virtual QStringList getSupportedCodecs() const {
        return QStringList(); // Empty = all codecs
    }

    virtual QStringList getSupportedFormats() const {
        return QStringList(); // Empty = all formats
    }
};

} // namespace VideoStudio

// Qt plugin interface declaration
Q_DECLARE_INTERFACE(VideoStudio::IAnalyzerPlugin, "com.videostudio.IAnalyzerPlugin/1.0")

#endif // IANALYZERPLUGIN_H
