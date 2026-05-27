#ifndef EXPORTER_H
#define EXPORTER_H

#include <QString>
#include <QVector>
#include "core/FrameInfo.h"
#include "core/StreamInfo.h"
#include "core/BitrateAnalyzer.h"
#include "core/GOPAnalyzer.h"
#include "core/MetricsCollector.h"

class Exporter {
public:
    static bool exportFrameListToCSV(const QString& filePath, const QVector<FrameInfo>& frames);
    static bool exportBitrateToCSV(const QString& filePath, const QVector<BitratePoint>& points);
    static bool exportGOPToCSV(const QString& filePath, const QVector<GOP>& gops);
    static bool exportHTMLReport(const QString& filePath,
                                  const QString& videoPath,
                                  const StreamInfo& streamInfo,
                                  MetricsCollector* metricsCollector,
                                  const BitrateStats& bitrateStats,
                                  const GOPStats& gopStats);

private:
    static QString escapeHTML(const QString& text);
    static QString formatSize(int64_t size);
    static QString formatBitrate(double bitrate);
};

#endif // EXPORTER_H
