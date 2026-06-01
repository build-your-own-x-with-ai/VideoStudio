#include "tsparser.h"
#include <QDebug>
#include <QDataStream>

namespace VideoStudio {

TSParser::TSParser(QObject* parent)
    : QObject(parent)
    , m_totalPackets(0)
    , m_fileSize(0)
{
}

TSParser::~TSParser() {
    clear();
}

void TSParser::clear() {
    m_packets.clear();
    m_pids.clear();
    m_psiTables.clear();
    m_programs.clear();
    m_psiBuffers.clear();
    m_totalPackets = 0;
    m_fileSize = 0;
    m_filePath.clear();
}

bool TSParser::parseFile(const QString& filePath) {
    clear();
    m_filePath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit parseError("Cannot open file: " + filePath);
        return false;
    }

    m_fileSize = file.size();
    const int packetSize = 188;

    // Check if file size is multiple of 188
    if (m_fileSize % packetSize != 0) {
        qWarning() << "File size is not multiple of 188 bytes";
    }

    m_totalPackets = m_fileSize / packetSize;
    int64_t offset = 0;
    int progressCounter = 0;
    const int progressInterval = 1000; // Update progress every 1000 packets

    while (!file.atEnd()) {
        QByteArray data = file.read(packetSize);
        if (data.size() != packetSize) {
            break; // End of file or incomplete packet
        }

        TSPacket packet;
        if (parsePacket(data, offset, packet)) {
            m_packets.append(packet);

            // Update PID info
            PIDInfo& pidInfo = m_pids[packet.pid];
            pidInfo.pid = packet.pid;
            pidInfo.packetCount++;
            pidInfo.totalBytes += packetSize;

            // Set type for standard PIDs if not already set
            if (pidInfo.type.isEmpty()) {
                if (packet.pid == 0x0000) {
                    pidInfo.type = "PAT";
                } else if (packet.pid == 0x0001) {
                    pidInfo.type = "CAT";
                } else if (packet.pid == 0x0010) {
                    pidInfo.type = "NIT";
                } else if (packet.pid == 0x0011) {
                    pidInfo.type = "SDT/BAT";
                } else if (packet.pid == 0x0012) {
                    pidInfo.type = "EIT";
                } else if (packet.pid == 0x0013) {
                    pidInfo.type = "RST";
                } else if (packet.pid == 0x0014) {
                    pidInfo.type = "TDT/TOT";
                } else if (packet.pid == 0x1FFF) {
                    pidInfo.type = "Null";
                }
            }

            // Check continuity counter
            if (pidInfo.lastContinuityCounter != 0xFF) {
                uint8_t expected = (pidInfo.lastContinuityCounter + 1) & 0x0F;
                if (packet.continuityCounter != expected && packet.adaptationFieldControl != 0) {
                    qDebug() << "Continuity counter error at PID" << packet.pid
                             << "offset" << offset
                             << "expected" << expected
                             << "got" << packet.continuityCounter;
                }
            }
            pidInfo.lastContinuityCounter = packet.continuityCounter;

            // Parse PSI tables
            if (packet.pid == 0x0000 || // PAT
                packet.pid == 0x0001 || // CAT
                packet.pid == 0x0010 || // NIT
                packet.pid == 0x0011 || // SDT/BAT
                packet.pid == 0x0012 || // EIT
                m_pids[packet.pid].type == "PMT") {
                parsePSITable(packet);
            }
        }

        offset += packetSize;

        // Update progress
        if (++progressCounter >= progressInterval) {
            int percentage = (offset * 100) / m_fileSize;
            emit parseProgress(percentage);
            progressCounter = 0;
        }
    }

    file.close();

    // Calculate percentages
    calculatePercentages();

    emit parseProgress(100);
    emit parseComplete();

    qDebug() << "Parsed" << m_packets.size() << "packets,"
             << m_pids.size() << "PIDs,"
             << m_programs.size() << "programs";

    return true;
}

bool TSParser::parsePacket(const QByteArray& data, int64_t offset, TSPacket& packet) {
    if (data.size() < 4) {
        return false;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());

    packet.offset = offset;
    packet.syncByte = bytes[0];

    // Check sync byte
    if (packet.syncByte != 0x47) {
        qWarning() << "Invalid sync byte at offset" << offset << ":" << Qt::hex << packet.syncByte;
        return false;
    }

    // Parse header
    packet.transportErrorIndicator = (bytes[1] & 0x80) != 0;
    packet.payloadUnitStartIndicator = (bytes[1] & 0x40) != 0;
    packet.transportPriority = (bytes[1] & 0x20) != 0;
    packet.pid = ((bytes[1] & 0x1F) << 8) | bytes[2];

    packet.scramblingControl = (bytes[3] >> 6) & 0x03;
    packet.adaptationFieldControl = (bytes[3] >> 4) & 0x03;
    packet.continuityCounter = bytes[3] & 0x0F;

    int payloadStart = 4;

    // Parse adaptation field if present
    if (packet.adaptationFieldControl == 2 || packet.adaptationFieldControl == 3) {
        packet.hasAdaptationField = true;
        uint8_t adaptationFieldLength = bytes[4];
        if (adaptationFieldLength > 0 && (5 + adaptationFieldLength) <= data.size()) {
            parseAdaptationField(bytes + 5, adaptationFieldLength, packet);
            payloadStart = 5 + adaptationFieldLength;
        }
    }

    // Extract payload
    if (packet.adaptationFieldControl == 1 || packet.adaptationFieldControl == 3) {
        if (payloadStart < data.size()) {
            packet.payload = data.mid(payloadStart);

            // Parse PES header if this is a video/audio PID with payload unit start
            if (packet.payloadUnitStartIndicator && !packet.payload.isEmpty()) {
                parsePESHeader(packet);
            }
        }
    }

    return true;
}

void TSParser::parseAdaptationField(const uint8_t* data, int length, TSPacket& packet) {
    if (length < 1) return;

    uint8_t flags = data[0];
    packet.discontinuityIndicator = (flags & 0x80) != 0;
    packet.randomAccessIndicator = (flags & 0x40) != 0;
    packet.elementaryStreamPriorityIndicator = (flags & 0x20) != 0;
    bool pcrFlag = (flags & 0x10) != 0;
    bool opcrFlag = (flags & 0x08) != 0;

    int offset = 1;

    // Parse PCR
    if (pcrFlag && (offset + 6) <= length) {
        packet.hasPCR = true;
        uint64_t pcrBase = (static_cast<uint64_t>(data[offset]) << 25) |
                           (static_cast<uint64_t>(data[offset + 1]) << 17) |
                           (static_cast<uint64_t>(data[offset + 2]) << 9) |
                           (static_cast<uint64_t>(data[offset + 3]) << 1) |
                           ((data[offset + 4] >> 7) & 0x01);
        uint16_t pcrExt = ((data[offset + 4] & 0x01) << 8) | data[offset + 5];
        packet.pcr = pcrBase * 300 + pcrExt;
        offset += 6;
    }
}

void TSParser::parsePSITable(const TSPacket& packet) {
    if (packet.payload.isEmpty()) {
        return;
    }

    const uint8_t* data = reinterpret_cast<const uint8_t*>(packet.payload.constData());
    int dataSize = packet.payload.size();

    // Skip pointer field if payload unit start
    int offset = 0;
    if (packet.payloadUnitStartIndicator) {
        uint8_t pointerField = data[0];
        offset = 1 + pointerField;
        if (offset >= dataSize) return;
    }

    // Parse table header
    if ((offset + 3) > dataSize) return;

    uint8_t tableId = data[offset];
    uint16_t sectionLength = ((data[offset + 1] & 0x0F) << 8) | data[offset + 2];

    PSITable table;
    table.type = PSITableType::Unknown;
    table.pid = packet.pid;
    table.offset = packet.offset;
    table.tableId = tableId;
    table.sectionLength = sectionLength;

    // Determine table type
    if (packet.pid == 0x0000) {
        table.type = PSITableType::PAT;
        table.data = packet.payload.mid(offset);
        m_psiTables.append(table);
        parsePAT(table.data, packet.offset);
    } else if (packet.pid == 0x0011 && tableId == 0x42) {
        table.type = PSITableType::SDT;
        table.data = packet.payload.mid(offset);
        m_psiTables.append(table);
        parseSDT(table.data, packet.offset);
    } else if (packet.pid == 0x0012 && (tableId >= 0x4E && tableId <= 0x6F)) {
        // EIT table (table_id 0x4E-0x6F)
        table.type = PSITableType::EIT;
        table.data = packet.payload.mid(offset);
        m_psiTables.append(table);
        // EIT parsing will be done by EPG panel
    } else if (m_pids[packet.pid].type == "PMT") {
        table.type = PSITableType::PMT;
        table.data = packet.payload.mid(offset);
        m_psiTables.append(table);
        parsePMT(table.data, packet.offset, packet.pid);
    }
}

void TSParser::parsePAT(const QByteArray& data, int64_t offset) {
    if (data.size() < 8) return;

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    uint16_t sectionLength = ((bytes[1] & 0x0F) << 8) | bytes[2];

    // Parse program entries
    int numPrograms = (sectionLength - 9) / 4; // Subtract header and CRC
    for (int i = 0; i < numPrograms; i++) {
        int entryOffset = 8 + i * 4;
        if (entryOffset + 4 > data.size()) break;

        uint16_t programNumber = (bytes[entryOffset] << 8) | bytes[entryOffset + 1];
        uint16_t pid = ((bytes[entryOffset + 2] & 0x1F) << 8) | bytes[entryOffset + 3];

        if (programNumber == 0) {
            // NIT PID
            m_pids[pid].type = "NIT";
        } else {
            // PMT PID
            m_pids[pid].type = "PMT";

            // Check if program already exists
            bool programExists = false;
            for (const ProgramInfo& existingProgram : m_programs) {
                if (existingProgram.programNumber == programNumber) {
                    programExists = true;
                    break;
                }
            }

            if (!programExists) {
                ProgramInfo program;
                program.programNumber = programNumber;
                program.pmtPid = pid;
                m_programs.append(program);
            }
        }
    }
}

void TSParser::parsePMT(const QByteArray& data, int64_t offset, uint16_t pid) {
    if (data.size() < 12) return;

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    uint16_t sectionLength = ((bytes[1] & 0x0F) << 8) | bytes[2];
    uint16_t programInfoLength = ((bytes[10] & 0x0F) << 8) | bytes[11];

    int esInfoStart = 12 + programInfoLength;
    int esInfoEnd = 3 + sectionLength - 4; // Subtract CRC

    // Find program
    ProgramInfo* program = nullptr;
    for (auto& prog : m_programs) {
        if (prog.pmtPid == pid) {
            program = &prog;
            break;
        }
    }

    // Parse elementary streams
    int pos = esInfoStart;
    while (pos + 5 <= esInfoEnd && pos < data.size()) {
        uint8_t streamType = bytes[pos];
        uint16_t elementaryPID = ((bytes[pos + 1] & 0x1F) << 8) | bytes[pos + 2];
        uint16_t esInfoLength = ((bytes[pos + 3] & 0x0F) << 8) | bytes[pos + 4];

        // Update PID info
        m_pids[elementaryPID].type = getStreamTypeName(streamType);
        m_pids[elementaryPID].codec = getCodecName(streamType);

        if (program) {
            // Only add if not already present
            if (!program->elementaryPIDs.contains(elementaryPID)) {
                program->elementaryPIDs.append(elementaryPID);
            }
        }

        pos += 5 + esInfoLength;
    }
}

void TSParser::parseSDT(const QByteArray& data, int64_t offset) {
    if (data.size() < 11) return;

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    uint16_t sectionLength = ((bytes[1] & 0x0F) << 8) | bytes[2];

    // Parse service entries
    int pos = 11;
    int endPos = 3 + sectionLength - 4;

    while (pos + 5 <= endPos && pos < data.size()) {
        uint16_t serviceId = (bytes[pos] << 8) | bytes[pos + 1];
        uint16_t descriptorsLoopLength = ((bytes[pos + 3] & 0x0F) << 8) | bytes[pos + 4];

        // Find program with this service ID
        for (auto& program : m_programs) {
            if (program.programNumber == serviceId) {
                // Parse descriptors to get service name
                int descPos = pos + 5;
                int descEnd = descPos + descriptorsLoopLength;
                while (descPos + 2 <= descEnd && descPos < data.size()) {
                    uint8_t descTag = bytes[descPos];
                    uint8_t descLength = bytes[descPos + 1];

                    if (descTag == 0x48 && descPos + 2 + descLength <= data.size()) {
                        // Service descriptor
                        int namePos = descPos + 5;
                        uint8_t providerNameLength = bytes[descPos + 3];
                        namePos += providerNameLength;
                        if (namePos < descPos + 2 + descLength) {
                            uint8_t serviceNameLength = bytes[namePos];
                            namePos++;
                            if (namePos + serviceNameLength <= descPos + 2 + descLength) {
                                program.serviceName = QString::fromUtf8(
                                    reinterpret_cast<const char*>(bytes + namePos),
                                    serviceNameLength);
                            }
                        }
                    }

                    descPos += 2 + descLength;
                }
                break;
            }
        }

        pos += 5 + descriptorsLoopLength;
    }
}

void TSParser::calculatePercentages() {
    int64_t totalBytes = m_totalPackets * 188;
    for (auto& pidInfo : m_pids) {
        pidInfo.percentage = (pidInfo.totalBytes * 100.0) / totalBytes;
    }
}

QString TSParser::getStreamTypeName(uint8_t streamType) const {
    switch (streamType) {
        case 0x01: return "MPEG-1 Video";
        case 0x02: return "MPEG-2 Video";
        case 0x03: return "MPEG-1 Audio";
        case 0x04: return "MPEG-2 Audio";
        case 0x0F: return "AAC Audio";
        case 0x1B: return "H.264 Video";
        case 0x24: return "H.265 Video";
        case 0x81: return "AC-3 Audio";
        default: return "Unknown";
    }
}

QString TSParser::getCodecName(uint8_t streamType) const {
    switch (streamType) {
        case 0x01: return "MPEG-1";
        case 0x02: return "MPEG-2";
        case 0x03: return "MP2";
        case 0x04: return "MP2";
        case 0x0F: return "AAC";
        case 0x1B: return "H.264/AVC";
        case 0x24: return "H.265/HEVC";
        case 0x81: return "AC-3";
        default: return "";
    }
}

const TSPacket* TSParser::getPacket(int index) const {
    if (index >= 0 && index < m_packets.size()) {
        return &m_packets[index];
    }
    return nullptr;
}

int TSParser::findPacketByOffset(int64_t offset) const {
    // Binary search
    int left = 0;
    int right = m_packets.size() - 1;

    while (left <= right) {
        int mid = (left + right) / 2;
        if (m_packets[mid].offset == offset) {
            return mid;
        } else if (m_packets[mid].offset < offset) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return -1;
}

void TSParser::parsePESHeader(TSPacket& packet) {
    if (packet.payload.size() < 9) {
        return; // PES header minimum size is 9 bytes
    }

    const uint8_t* data = reinterpret_cast<const uint8_t*>(packet.payload.constData());

    // Check PES start code (0x000001)
    if (data[0] != 0x00 || data[1] != 0x00 || data[2] != 0x01) {
        return; // Not a PES packet
    }

    // Stream ID
    uint8_t streamId = data[3];

    // Skip non-video/audio streams
    if (streamId == 0xBC || // Program stream map
        streamId == 0xBE || // Padding stream
        streamId == 0xBF || // Private stream 2
        streamId == 0xF0 || // ECM
        streamId == 0xF1 || // EMM
        streamId == 0xFF || // Program stream directory
        streamId == 0xF2 || // DSMCC stream
        streamId == 0xF8) { // ITU-T Rec. H.222.1 type E
        return;
    }

    // PES packet length (bytes 4-5)
    // uint16_t pesPacketLength = (data[4] << 8) | data[5];

    // PES header data (byte 6-8)
    uint8_t ptsDtsFlags = (data[7] >> 6) & 0x03;
    uint8_t pesHeaderDataLength = data[8];

    int offset = 9;

    // Parse PTS (if present)
    if ((ptsDtsFlags == 0x02 || ptsDtsFlags == 0x03) && (offset + 5) <= packet.payload.size()) {
        packet.hasPTS = true;
        int64_t pts = (static_cast<int64_t>((data[offset] >> 1) & 0x07) << 30) |
                      (static_cast<int64_t>(data[offset + 1]) << 22) |
                      (static_cast<int64_t>((data[offset + 2] >> 1) & 0x7F) << 15) |
                      (static_cast<int64_t>(data[offset + 3]) << 7) |
                      (static_cast<int64_t>((data[offset + 4] >> 1) & 0x7F));
        packet.pts = pts;
        offset += 5;
    }

    // Parse DTS (if present)
    if (ptsDtsFlags == 0x03 && (offset + 5) <= packet.payload.size()) {
        packet.hasDTS = true;
        int64_t dts = (static_cast<int64_t>((data[offset] >> 1) & 0x07) << 30) |
                      (static_cast<int64_t>(data[offset + 1]) << 22) |
                      (static_cast<int64_t>((data[offset + 2] >> 1) & 0x7F) << 15) |
                      (static_cast<int64_t>(data[offset + 3]) << 7) |
                      (static_cast<int64_t>((data[offset + 4] >> 1) & 0x7F));
        packet.dts = dts;
    }
}

} // namespace VideoStudio
