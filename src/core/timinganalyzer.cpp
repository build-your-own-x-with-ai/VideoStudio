#include "core/timinganalyzer.h"
#include "core/tsparser.h"
#include <QDebug>
#include <cmath>

namespace VideoStudio {

TimingAnalyzer::TimingAnalyzer(QObject* parent)
    : QObject(parent)
    , m_parser(nullptr)
{
}

TimingAnalyzer::~TimingAnalyzer() {
}

void TimingAnalyzer::setTSParser(TSParser* parser) {
    m_parser = parser;
    clear();
}

void TimingAnalyzer::clear() {
    m_ptsData.clear();
    m_dtsData.clear();
    m_pcrData.clear();
    m_pcrAccuracyData.clear();
    m_stats.clear();
}

void TimingAnalyzer::analyzePID(uint16_t pid) {
    if (!m_parser) {
        qWarning() << "TimingAnalyzer: No TS parser set";
        return;
    }

    qDebug() << "TimingAnalyzer: Analyzing PID" << QString::number(pid, 16);

    extractTimingData(pid);
    calculateStatistics(pid);
    calculatePCRAccuracy(pid);

    emit analysisComplete();
}

void TimingAnalyzer::extractTimingData(uint16_t pid) {
    const auto& packets = m_parser->getPackets();

    QVector<TimingPoint> ptsPoints;
    QVector<TimingPoint> dtsPoints;
    QVector<TimingPoint> pcrPoints;

    for (int i = 0; i < packets.size(); ++i) {
        const TSPacket& packet = packets[i];

        if (packet.pid != pid) {
            continue;
        }

        // Extract PTS
        if (packet.hasPTS) {
            ptsPoints.append(TimingPoint(packet.offset, i, packet.pts));
        }

        // Extract DTS
        if (packet.hasDTS) {
            dtsPoints.append(TimingPoint(packet.offset, i, packet.dts));
        }

        // Extract PCR
        if (packet.hasPCR) {
            pcrPoints.append(TimingPoint(packet.offset, i, packet.pcr));
        }
    }

    if (!ptsPoints.isEmpty()) {
        m_ptsData[pid] = ptsPoints;
        qDebug() << "  Found" << ptsPoints.size() << "PTS points";
    }

    if (!dtsPoints.isEmpty()) {
        m_dtsData[pid] = dtsPoints;
        qDebug() << "  Found" << dtsPoints.size() << "DTS points";
    }

    if (!pcrPoints.isEmpty()) {
        m_pcrData[pid] = pcrPoints;
        qDebug() << "  Found" << pcrPoints.size() << "PCR points";
    }
}

void TimingAnalyzer::calculateStatistics(uint16_t pid) {
    TimingStats stats;
    stats.pid = pid;

    // PTS statistics
    if (m_ptsData.contains(pid)) {
        const auto& ptsPoints = m_ptsData[pid];
        stats.ptsCount = ptsPoints.size();

        if (stats.ptsCount > 0) {
            stats.minPTS = ptsPoints.first().timestamp;
            stats.maxPTS = ptsPoints.last().timestamp;

            // Calculate average interval
            if (stats.ptsCount > 1) {
                int64_t totalInterval = 0;
                for (int i = 1; i < ptsPoints.size(); ++i) {
                    totalInterval += (ptsPoints[i].timestamp - ptsPoints[i-1].timestamp);
                }
                stats.avgPTSInterval = (totalInterval / (stats.ptsCount - 1)) / 90000.0;
            }
        }
    }

    // DTS statistics
    if (m_dtsData.contains(pid)) {
        const auto& dtsPoints = m_dtsData[pid];
        stats.dtsCount = dtsPoints.size();

        if (stats.dtsCount > 0) {
            stats.minDTS = dtsPoints.first().timestamp;
            stats.maxDTS = dtsPoints.last().timestamp;

            // Calculate average interval
            if (stats.dtsCount > 1) {
                int64_t totalInterval = 0;
                for (int i = 1; i < dtsPoints.size(); ++i) {
                    totalInterval += (dtsPoints[i].timestamp - dtsPoints[i-1].timestamp);
                }
                stats.avgDTSInterval = (totalInterval / (stats.dtsCount - 1)) / 90000.0;
            }
        }
    }

    // PCR statistics
    if (m_pcrData.contains(pid)) {
        const auto& pcrPoints = m_pcrData[pid];
        stats.pcrCount = pcrPoints.size();

        if (stats.pcrCount > 0) {
            stats.minPCR = pcrPoints.first().timestamp;
            stats.maxPCR = pcrPoints.last().timestamp;

            // Calculate average interval and jitter
            if (stats.pcrCount > 1) {
                QVector<double> intervals;
                for (int i = 1; i < pcrPoints.size(); ++i) {
                    double interval = (pcrPoints[i].timestamp - pcrPoints[i-1].timestamp) / 90000.0;
                    intervals.append(interval);
                }

                // Average interval
                double sum = 0;
                for (double interval : intervals) {
                    sum += interval;
                }
                stats.avgPCRInterval = sum / intervals.size();

                // Jitter (standard deviation)
                double variance = 0;
                for (double interval : intervals) {
                    double diff = interval - stats.avgPCRInterval;
                    variance += diff * diff;
                }
                stats.pcrJitter = std::sqrt(variance / intervals.size());
            }
        }
    }

    m_stats[pid] = stats;

    qDebug() << "  Statistics: PTS count=" << stats.ptsCount
             << "DTS count=" << stats.dtsCount
             << "PCR count=" << stats.pcrCount;
}

void TimingAnalyzer::calculatePCRAccuracy(uint16_t pid) {
    if (!m_pcrData.contains(pid)) {
        return;
    }

    const auto& pcrPoints = m_pcrData[pid];
    if (pcrPoints.size() < 2) {
        return;
    }

    QVector<PCRAccuracy> accuracyData;

    // Calculate expected PCR based on bitrate
    // For now, use simple linear interpolation
    for (int i = 1; i < pcrPoints.size(); ++i) {
        PCRAccuracy accuracy;
        accuracy.offset = pcrPoints[i].offset;
        accuracy.packetIndex = pcrPoints[i].packetIndex;
        accuracy.actualPCR = pcrPoints[i].timestamp;

        // Expected PCR based on previous PCR and offset difference
        int64_t offsetDiff = pcrPoints[i].offset - pcrPoints[i-1].offset;
        int64_t pcrDiff = pcrPoints[i].timestamp - pcrPoints[i-1].timestamp;

        // Simple linear extrapolation
        accuracy.expectedPCR = pcrPoints[i-1].timestamp + pcrDiff;

        accuracy.difference = accuracy.actualPCR - accuracy.expectedPCR;

        // Convert to nanoseconds (PCR is in 27MHz units for base, 90kHz for extension)
        // For simplicity, treat as 90kHz units
        accuracy.differenceNs = (accuracy.difference / 90000.0) * 1e9;

        accuracyData.append(accuracy);
    }

    m_pcrAccuracyData[pid] = accuracyData;
    qDebug() << "  Calculated" << accuracyData.size() << "PCR accuracy points";
}

QVector<TimingPoint> TimingAnalyzer::getPTSData(uint16_t pid) const {
    return m_ptsData.value(pid);
}

QVector<TimingPoint> TimingAnalyzer::getDTSData(uint16_t pid) const {
    return m_dtsData.value(pid);
}

QVector<TimingPoint> TimingAnalyzer::getPCRData(uint16_t pid) const {
    return m_pcrData.value(pid);
}

QVector<PCRAccuracy> TimingAnalyzer::getPCRAccuracyData(uint16_t pid) const {
    return m_pcrAccuracyData.value(pid);
}

TimingStats TimingAnalyzer::getTimingStats(uint16_t pid) const {
    return m_stats.value(pid);
}

QVector<uint16_t> TimingAnalyzer::getPIDsWithPTS() const {
    return m_ptsData.keys().toVector();
}

QVector<uint16_t> TimingAnalyzer::getPIDsWithDTS() const {
    return m_dtsData.keys().toVector();
}

QVector<uint16_t> TimingAnalyzer::getPIDsWithPCR() const {
    return m_pcrData.keys().toVector();
}

} // namespace VideoStudio
