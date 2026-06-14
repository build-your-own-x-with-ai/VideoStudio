# Plugin Development Guide

VideoStudio supports an extensible plugin system that allows you to create custom video analyzers without modifying the core application.

## Overview

Plugins are dynamically loaded shared libraries that implement the `IAnalyzerPlugin` interface. They can perform custom analysis on video files and return structured results.

## Plugin Architecture

### Key Components

1. **IAnalyzerPlugin Interface** - Base interface all plugins must implement
2. **PluginManager** - Handles plugin discovery, loading, and lifecycle
3. **PluginMetadata** - Describes plugin properties (ID, name, version, etc.)
4. **AnalysisResult** - Structure for returning analysis results

### Plugin Lifecycle

1. **Discovery**: PluginManager scans the plugins directory
2. **Loading**: Qt's QPluginLoader loads the shared library
3. **Initialization**: `initialize()` is called once after loading
4. **Analysis**: `analyze()` is called for each video file
5. **Cleanup**: `cleanup()` is called before unloading

## Creating a Plugin

### 1. Implement the Interface

Create a header file for your plugin:

```cpp
#ifndef MYPLUGIN_H
#define MYPLUGIN_H

#include <QObject>
#include "plugins/ianalyzerplugin.h"

namespace VideoStudio {

class MyPlugin : public QObject, public IAnalyzerPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.videostudio.IAnalyzerPlugin/1.0")
    Q_INTERFACES(VideoStudio::IAnalyzerPlugin)

public:
    MyPlugin();
    ~MyPlugin() override;

    // IAnalyzerPlugin interface
    PluginMetadata getMetadata() const override;
    bool initialize() override;
    void cleanup() override;
    AnalysisResult analyze(const QString& videoFile, 
                          const QVariantMap& options) override;
    QVariantMap getDefaultOptions() const override;
};

} // namespace VideoStudio

#endif // MYPLUGIN_H
```

### 2. Implement the Functions

Create the implementation file:

```cpp
#include "myplugin.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace VideoStudio {

MyPlugin::MyPlugin() {}
MyPlugin::~MyPlugin() { cleanup(); }

PluginMetadata MyPlugin::getMetadata() const {
    PluginMetadata metadata;
    metadata.id = "com.example.myplugin";  // Unique ID (reverse domain)
    metadata.name = "My Custom Analyzer";
    metadata.version = "1.0.0";
    metadata.author = "Your Name";
    metadata.description = "Custom video analysis plugin";
    metadata.category = "Custom";
    metadata.tags << "custom" << "analysis";
    return metadata;
}

bool MyPlugin::initialize() {
    // Initialize resources, load config, etc.
    qDebug() << "MyPlugin: Initializing...";
    return true;
}

void MyPlugin::cleanup() {
    // Release resources
    qDebug() << "MyPlugin: Cleaning up...";
}

AnalysisResult MyPlugin::analyze(const QString& videoFile, 
                                 const QVariantMap& options) {
    AnalysisResult result;

    // Check file exists
    QFileInfo fileInfo(videoFile);
    if (!fileInfo.exists()) {
        result.success = false;
        result.error = "File not found";
        return result;
    }

    // Open video file with FFmpeg
    AVFormatContext* formatCtx = nullptr;
    if (avformat_open_input(&formatCtx, videoFile.toUtf8().constData(), 
                           nullptr, nullptr) < 0) {
        result.success = false;
        result.error = "Failed to open video file";
        return result;
    }

    avformat_find_stream_info(formatCtx, nullptr);

    // Perform your custom analysis here
    QJsonObject data;
    data["file"] = videoFile;
    data["format"] = QString(formatCtx->iformat->name);
    data["duration"] = formatCtx->duration / (double)AV_TIME_BASE;
    
    // Your custom metrics
    data["my_custom_metric"] = 42;

    avformat_close_input(&formatCtx);

    // Generate text report
    QString report;
    report += "My Custom Analysis\n";
    report += "==================\n\n";
    report += QString("File: %1\n").arg(videoFile);
    report += QString("Custom metric: %1\n").arg(data["my_custom_metric"].toInt());

    result.success = true;
    result.data = data;
    result.textReport = report;

    return result;
}

QVariantMap MyPlugin::getDefaultOptions() const {
    QVariantMap options;
    options["threshold"] = 50;
    return options;
}

} // namespace VideoStudio
```

### 3. Build Configuration

Add to CMakeLists.txt:

```cmake
# My Plugin
add_library(myplugin MODULE
    src/plugins/myplugin.cpp
)

target_link_libraries(myplugin PRIVATE
    Qt6::Core
)

# Link FFmpeg
if(WIN32)
    target_link_libraries(myplugin PRIVATE ${FFMPEG_LIBRARIES})
else()
    target_link_libraries(myplugin PRIVATE ${FFMPEG_LDFLAGS})
endif()

target_compile_options(myplugin PRIVATE
    ${FFMPEG_CFLAGS_OTHER}
)

target_include_directories(myplugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}
)

set_target_properties(myplugin PROPERTIES
    AUTOMOC ON
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    PREFIX ""
    SUFFIX ".dylib"  # .so on Linux, .dll on Windows
)

install(TARGETS myplugin
    LIBRARY DESTINATION plugins
)
```

### 4. Build and Test

```bash
# Build
cmake --build . --target myplugin

# List plugins
./videostudio-cli plugin list

# Run plugin
./videostudio-cli plugin run com.example.myplugin video.mp4

# JSON output
./videostudio-cli plugin run com.example.myplugin video.mp4 -o report.json
```

## Advanced Features

### Options and Configuration

```cpp
QVariantMap MyPlugin::getDefaultOptions() const {
    QVariantMap options;
    options["threshold"] = 100;
    options["mode"] = "fast";
    options["enable_feature"] = true;
    return options;
}

bool MyPlugin::validateOptions(const QVariantMap& options, 
                               QString& error) const {
    if (options.contains("threshold")) {
        int threshold = options["threshold"].toInt();
        if (threshold < 0 || threshold > 100) {
            error = "Threshold must be between 0 and 100";
            return false;
        }
    }
    return true;
}

AnalysisResult MyPlugin::analyze(const QString& videoFile, 
                                const QVariantMap& options) {
    // Use options
    int threshold = options.value("threshold", 50).toInt();
    QString mode = options.value("mode", "normal").toString();
    
    // ... perform analysis
}
```

### Codec/Format Filtering

```cpp
QStringList MyPlugin::getSupportedCodecs() const {
    return QStringList() << "h264" << "hevc";
}

QStringList MyPlugin::getSupportedFormats() const {
    return QStringList() << "mp4" << "mov";
}
```

### Configuration UI (GUI only)

```cpp
QWidget* MyPlugin::createConfigWidget(QWidget* parent) {
    QWidget* widget = new QWidget(parent);
    QFormLayout* layout = new QFormLayout(widget);
    
    QSpinBox* thresholdSpin = new QSpinBox();
    thresholdSpin->setRange(0, 100);
    thresholdSpin->setValue(50);
    layout->addRow("Threshold:", thresholdSpin);
    
    return widget;
}
```

## Best Practices

1. **Unique Plugin IDs**: Use reverse domain notation (e.g., `com.company.pluginname`)
2. **Error Handling**: Always set `result.success` and `result.error` appropriately
3. **Resource Cleanup**: Release all resources in `cleanup()`
4. **Progress Updates**: For long operations, emit progress signals
5. **Thread Safety**: Ensure plugin is thread-safe if used concurrently
6. **Documentation**: Provide clear descriptions and tag your plugin appropriately
7. **Versioning**: Follow semantic versioning (MAJOR.MINOR.PATCH)

## Example: Custom Quality Analyzer

See the included `sampleanalyzerplugin.cpp` for a complete working example that:
- Opens video files with FFmpeg
- Analyzes streams and packets
- Calculates custom metrics
- Returns both JSON and text reports
- Supports custom options

## Plugin Distribution

Compiled plugins can be distributed as standalone files:

1. **macOS**: `.dylib` file
2. **Linux**: `.so` file
3. **Windows**: `.dll` file

Users simply place the plugin file in the `plugins/` directory next to the VideoStudio executable.

## Debugging Plugins

Enable debug output to see plugin loading details:

```bash
export QT_LOGGING_RULES="*.debug=true"
./videostudio-cli plugin list
```

This will show:
- Plugin discovery process
- Loading attempts and errors
- Initialization status
- Cleanup operations

## API Reference

### PluginMetadata
- `id`: Unique identifier (string)
- `name`: Display name (string)
- `version`: Version string (string)
- `author`: Author name (string)
- `description`: Brief description (string)
- `category`: Category (string)
- `tags`: Search tags (QStringList)

### AnalysisResult
- `success`: Whether analysis succeeded (bool)
- `error`: Error message if failed (QString)
- `data`: Structured result data (QJsonObject)
- `textReport`: Human-readable report (QString)

### IAnalyzerPlugin Methods
- `getMetadata()`: Return plugin metadata
- `initialize()`: Initialize plugin (called once)
- `cleanup()`: Cleanup resources (called before unload)
- `analyze(videoFile, options)`: Perform analysis
- `getDefaultOptions()`: Return default options
- `validateOptions(options, error)`: Validate options
- `getSupportedCodecs()`: Return supported codecs
- `getSupportedFormats()`: Return supported formats
- `createConfigWidget(parent)`: Create config UI (optional)

## Further Examples

Check the `examples/plugins/` directory for more plugin examples:
- Quality metrics analyzer
- Scene change detector
- Audio level analyzer
- Custom bitrate profiler
