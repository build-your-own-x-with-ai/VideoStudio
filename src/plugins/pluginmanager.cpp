#include "pluginmanager.h"
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>

namespace VideoStudio {

PluginManager::PluginManager(QObject* parent)
    : QObject(parent)
{
    // Default plugins directory
    QString appPath = QCoreApplication::applicationDirPath();
    m_pluginsDirectory = appPath + "/plugins";

    qDebug() << "PluginManager initialized, plugins directory:" << m_pluginsDirectory;
}

PluginManager::~PluginManager() {
    unloadAllPlugins();
}

PluginManager& PluginManager::instance() {
    static PluginManager instance;
    return instance;
}

void PluginManager::scanPlugins(const QString& directory) {
    QDir pluginsDir(directory);
    if (!pluginsDir.exists()) {
        qWarning() << "Plugins directory does not exist:" << directory;
        return;
    }

    qDebug() << "Scanning for plugins in:" << directory;

    // Look for .dylib (macOS), .so (Linux), .dll (Windows)
    QStringList filters;
#ifdef Q_OS_MAC
    filters << "*.dylib";
#elif defined(Q_OS_WIN)
    filters << "*.dll";
#else
    filters << "*.so";
#endif

    QFileInfoList files = pluginsDir.entryInfoList(filters, QDir::Files);
    qDebug() << "Found" << files.size() << "potential plugin files";

    for (const QFileInfo& fileInfo : files) {
        QString filePath = fileInfo.absoluteFilePath();
        qDebug() << "Attempting to load plugin:" << filePath;
        loadPlugin(filePath);
    }
}

bool PluginManager::loadPlugin(const QString& filePath) {
    QPluginLoader* loader = new QPluginLoader(filePath);
    QObject* pluginObject = loader->instance();

    if (!pluginObject) {
        qWarning() << "Failed to load plugin:" << filePath;
        qWarning() << "Error:" << loader->errorString();
        delete loader;
        return false;
    }

    IAnalyzerPlugin* plugin = qobject_cast<IAnalyzerPlugin*>(pluginObject);
    if (!plugin) {
        qWarning() << "Plugin does not implement IAnalyzerPlugin interface:" << filePath;
        loader->unload();
        delete loader;
        return false;
    }

    // Initialize plugin
    if (!plugin->initialize()) {
        qWarning() << "Plugin initialization failed:" << filePath;
        loader->unload();
        delete loader;
        return false;
    }

    // Get metadata
    PluginMetadata metadata = plugin->getMetadata();

    // Check for duplicate plugin ID
    if (m_plugins.contains(metadata.id)) {
        qWarning() << "Plugin with ID already loaded:" << metadata.id;
        plugin->cleanup();
        loader->unload();
        delete loader;
        return false;
    }

    // Store plugin info
    PluginInfo info;
    info.plugin = plugin;
    info.loader = loader;
    info.filePath = filePath;
    info.metadata = metadata;

    m_plugins.insert(metadata.id, info);

    qDebug() << "Successfully loaded plugin:" << metadata.name << "(" << metadata.id << ")";
    emit pluginLoaded(metadata.id);

    return true;
}

void PluginManager::unloadPlugin(const QString& pluginId) {
    if (!m_plugins.contains(pluginId)) {
        qWarning() << "Plugin not found:" << pluginId;
        return;
    }

    PluginInfo& info = m_plugins[pluginId];

    // Cleanup plugin
    if (info.plugin) {
        info.plugin->cleanup();
    }

    // Unload library
    if (info.loader) {
        info.loader->unload();
        delete info.loader;
    }

    m_plugins.remove(pluginId);
    emit pluginUnloaded(pluginId);

    qDebug() << "Unloaded plugin:" << pluginId;
}

void PluginManager::unloadAllPlugins() {
    QStringList pluginIds = m_plugins.keys();
    for (const QString& pluginId : pluginIds) {
        unloadPlugin(pluginId);
    }
}

IAnalyzerPlugin* PluginManager::getPlugin(const QString& pluginId) {
    if (!m_plugins.contains(pluginId)) {
        return nullptr;
    }
    return m_plugins[pluginId].plugin;
}

QVector<IAnalyzerPlugin*> PluginManager::getAllPlugins() const {
    QVector<IAnalyzerPlugin*> plugins;
    for (const PluginInfo& info : m_plugins) {
        plugins.append(info.plugin);
    }
    return plugins;
}

QVector<PluginMetadata> PluginManager::getPluginMetadataList() const {
    QVector<PluginMetadata> metadataList;
    for (const PluginInfo& info : m_plugins) {
        metadataList.append(info.metadata);
    }
    return metadataList;
}

bool PluginManager::hasPlugin(const QString& pluginId) const {
    return m_plugins.contains(pluginId);
}

int PluginManager::getPluginCount() const {
    return m_plugins.size();
}

QStringList PluginManager::getPluginIds() const {
    return m_plugins.keys();
}

AnalysisResult PluginManager::runPlugin(const QString& pluginId, const QString& videoFile,
                                        const QVariantMap& options) {
    AnalysisResult result;

    IAnalyzerPlugin* plugin = getPlugin(pluginId);
    if (!plugin) {
        result.success = false;
        result.error = QString("Plugin not found: %1").arg(pluginId);
        emit pluginError(pluginId, result.error);
        return result;
    }

    // Validate options
    QString validationError;
    if (!plugin->validateOptions(options, validationError)) {
        result.success = false;
        result.error = QString("Invalid options: %1").arg(validationError);
        emit pluginError(pluginId, result.error);
        return result;
    }

    // Run analysis
    try {
        result = plugin->analyze(videoFile, options);
    } catch (const std::exception& e) {
        result.success = false;
        result.error = QString("Plugin exception: %1").arg(e.what());
        emit pluginError(pluginId, result.error);
    } catch (...) {
        result.success = false;
        result.error = "Unknown plugin exception";
        emit pluginError(pluginId, result.error);
    }

    return result;
}

QString PluginManager::getPluginsDirectory() const {
    return m_pluginsDirectory;
}

void PluginManager::setPluginsDirectory(const QString& directory) {
    m_pluginsDirectory = directory;
}

} // namespace VideoStudio
