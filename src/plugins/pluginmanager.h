#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QPluginLoader>
#include "ianalyzerplugin.h"

namespace VideoStudio {

class PluginManager : public QObject {
    Q_OBJECT

public:
    static PluginManager& instance();

    // Plugin discovery and loading
    void scanPlugins(const QString& directory);
    bool loadPlugin(const QString& filePath);
    void unloadPlugin(const QString& pluginId);
    void unloadAllPlugins();

    // Plugin access
    IAnalyzerPlugin* getPlugin(const QString& pluginId);
    QVector<IAnalyzerPlugin*> getAllPlugins() const;
    QVector<PluginMetadata> getPluginMetadataList() const;

    // Plugin queries
    bool hasPlugin(const QString& pluginId) const;
    int getPluginCount() const;
    QStringList getPluginIds() const;

    // Plugin execution
    AnalysisResult runPlugin(const QString& pluginId, const QString& videoFile,
                            const QVariantMap& options = QVariantMap());

    // Get plugins directory
    QString getPluginsDirectory() const;
    void setPluginsDirectory(const QString& directory);

signals:
    void pluginLoaded(const QString& pluginId);
    void pluginUnloaded(const QString& pluginId);
    void pluginError(const QString& pluginId, const QString& error);

private:
    explicit PluginManager(QObject* parent = nullptr);
    ~PluginManager();
    Q_DISABLE_COPY(PluginManager)

    struct PluginInfo {
        IAnalyzerPlugin* plugin;
        QPluginLoader* loader;
        QString filePath;
        PluginMetadata metadata;
    };

    QMap<QString, PluginInfo> m_plugins;
    QString m_pluginsDirectory;
};

} // namespace VideoStudio

#endif // PLUGINMANAGER_H
