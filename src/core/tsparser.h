#ifndef TSPARSER_H
#define TSPARSER_H

#include "tsdata.h"
#include <QObject>
#include <QFile>
#include <QMap>

namespace VideoStudio {

class TSParser : public QObject {
    Q_OBJECT

public:
    explicit TSParser(QObject* parent = nullptr);
    ~TSParser();

    // Parse TS file
    bool parseFile(const QString& filePath);
    void clear();

    // Getters
    const QVector<TSPacket>& getPackets() const { return m_packets; }
    const QMap<uint16_t, PIDInfo>& getPIDs() const { return m_pids; }
    const QVector<PSITable>& getPSITables() const { return m_psiTables; }
    const QVector<ProgramInfo>& getPrograms() const { return m_programs; }

    int64_t getTotalPackets() const { return m_totalPackets; }
    int64_t getFileSize() const { return m_fileSize; }
    QString getFilePath() const { return m_filePath; }

    // Get packet at index
    const TSPacket* getPacket(int index) const;

    // Find packet by offset
    int findPacketByOffset(int64_t offset) const;

signals:
    void parseProgress(int percentage);
    void parseComplete();
    void parseError(const QString& error);

private:
    // Parse single TS packet
    bool parsePacket(const QByteArray& data, int64_t offset, TSPacket& packet);

    // Parse adaptation field
    void parseAdaptationField(const uint8_t* data, int length, TSPacket& packet);

    // Parse PSI tables
    void parsePSITable(const TSPacket& packet);
    void parsePAT(const QByteArray& data, int64_t offset);
    void parsePMT(const QByteArray& data, int64_t offset, uint16_t pid);
    void parseSDT(const QByteArray& data, int64_t offset);

    // Parse PES header for PTS/DTS
    void parsePESHeader(TSPacket& packet);

    // Calculate PID percentages
    void calculatePercentages();

    // Get stream type name
    QString getStreamTypeName(uint8_t streamType) const;
    QString getCodecName(uint8_t streamType) const;

private:
    QString m_filePath;
    QVector<TSPacket> m_packets;
    QMap<uint16_t, PIDInfo> m_pids;
    QVector<PSITable> m_psiTables;
    QVector<ProgramInfo> m_programs;

    int64_t m_totalPackets;
    int64_t m_fileSize;

    // PSI table buffers (for multi-packet tables)
    QMap<uint16_t, QByteArray> m_psiBuffers;
};

} // namespace VideoStudio

#endif // TSPARSER_H
