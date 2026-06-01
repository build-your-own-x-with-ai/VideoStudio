#ifndef TIMINGANALYZER_H
#define TIMINGANALYZER_H

#include "tsdata.h"
#include <QObject>
#include <QVector>
#include <QMap>

namespace VideoStudio {

class TSParser;

class TimingAnalyzer : public QObject {
    Q_OBJECT

public:
    explicit TimingAnalyzer(QObject* parent = nullptr);
    ~TimingAnalyzer();

    // Set the TS parser to analyze
    void setTSParser(TSParser* parser);

    // Analyze timing for a specific PID
    void analyzePID(uint16_t pid);

    // Get timing data
    QVector<TimingPoint> getPTSData(uint16_t pid) const;
    QVector<TimingPoint> getDTSData(uint16_t pid) const;
    QVector<TimingPoint> getPCRData(uint16_t pid) const;
    QVector<PCRAccuracy> getPCRAccuracyData(uint16_t pid) const;

    // Get timing statistics
    TimingStats getTimingStats(uint16_t pid) const;

    // Get list of PIDs with timing information
    QVector<uint16_t> getPIDsWithPTS() const;
    QVector<uint16_t> getPIDsWithDTS() const;
    QVector<uint16_t> getPIDsWithPCR() const;

    // Clear all analysis data
    void clear();

signals:
    void analysisProgress(int percentage);
    void analysisComplete();

private:
    void extractTimingData(uint16_t pid);
    void calculateStatistics(uint16_t pid);
    void calculatePCRAccuracy(uint16_t pid);

    TSParser* m_parser;

    // Timing data per PID
    QMap<uint16_t, QVector<TimingPoint>> m_ptsData;
    QMap<uint16_t, QVector<TimingPoint>> m_dtsData;
    QMap<uint16_t, QVector<TimingPoint>> m_pcrData;
    QMap<uint16_t, QVector<PCRAccuracy>> m_pcrAccuracyData;

    // Statistics per PID
    QMap<uint16_t, TimingStats> m_stats;
};

} // namespace VideoStudio

#endif // TIMINGANALYZER_H
