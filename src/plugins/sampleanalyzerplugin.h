#ifndef SAMPLEANALYZERPLUGIN_H
#define SAMPLEANALYZERPLUGIN_H

#include <QObject>
#include "../plugins/ianalyzerplugin.h"

namespace VideoStudio {

// Sample plugin demonstrating the plugin interface
class SampleAnalyzerPlugin : public QObject, public IAnalyzerPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.videostudio.IAnalyzerPlugin/1.0")
    Q_INTERFACES(VideoStudio::IAnalyzerPlugin)

public:
    SampleAnalyzerPlugin();
    ~SampleAnalyzerPlugin() override;

    // IAnalyzerPlugin interface
    PluginMetadata getMetadata() const override;
    bool initialize() override;
    void cleanup() override;
    AnalysisResult analyze(const QString& videoFile, const QVariantMap& options) override;
    QVariantMap getDefaultOptions() const override;

private:
    bool m_initialized;
};

} // namespace VideoStudio

#endif // SAMPLEANALYZERPLUGIN_H
