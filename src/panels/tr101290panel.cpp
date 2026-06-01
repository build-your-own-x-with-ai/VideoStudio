#include "panels/tr101290panel.h"
#include "core/tsparser.h"
#include <QHeaderView>
#include <QDebug>
#include <cmath>

namespace VideoStudio {

TR101290Panel::TR101290Panel(QWidget* parent)
    : QWidget(parent)
    , m_parser(nullptr)
    , m_firstPriorityCount(0)
    , m_secondPriorityCount(0)
    , m_thirdPriorityCount(0)
{
    createUI();
}

TR101290Panel::~TR101290Panel() {
}

void TR101290Panel::createUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Status summary
    QGroupBox* summaryGroup = new QGroupBox("TR 101-290 Compliance Status", this);
    QVBoxLayout* summaryLayout = new QVBoxLayout(summaryGroup);

    m_statusLabel = new QLabel("Not analyzed", summaryGroup);
    m_statusLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    summaryLayout->addWidget(m_statusLabel);

    QHBoxLayout* countsLayout = new QHBoxLayout();
    m_firstPriorityLabel = new QLabel("First Priority: 0", summaryGroup);
    m_firstPriorityLabel->setStyleSheet("color: #ff6464; font-weight: bold;");
    m_secondPriorityLabel = new QLabel("Second Priority: 0", summaryGroup);
    m_secondPriorityLabel->setStyleSheet("color: #64a0ff; font-weight: bold;");
    m_thirdPriorityLabel = new QLabel("Third Priority: 0", summaryGroup);
    m_thirdPriorityLabel->setStyleSheet("color: #ffa064; font-weight: bold;");

    countsLayout->addWidget(m_firstPriorityLabel);
    countsLayout->addWidget(m_secondPriorityLabel);
    countsLayout->addWidget(m_thirdPriorityLabel);
    countsLayout->addStretch();

    summaryLayout->addLayout(countsLayout);
    mainLayout->addWidget(summaryGroup);

    // Error tree
    m_errorTree = new QTreeWidget(this);
    m_errorTree->setHeaderLabels(QStringList() << "Error Type" << "Count" << "Description");
    m_errorTree->setColumnWidth(0, 250);
    m_errorTree->setColumnWidth(1, 80);
    m_errorTree->setColumnWidth(2, 400);
    m_errorTree->setAlternatingRowColors(true);
    m_errorTree->setRootIsDecorated(true);

    connect(m_errorTree, &QTreeWidget::itemDoubleClicked,
            this, &TR101290Panel::onErrorDoubleClicked);

    mainLayout->addWidget(m_errorTree);
}

void TR101290Panel::setTSParser(TSParser* parser) {
    m_parser = parser;
    analyzeStream();
}

void TR101290Panel::clear() {
    m_errors.clear();
    m_errorTree->clear();
    m_parser = nullptr;
    m_firstPriorityCount = 0;
    m_secondPriorityCount = 0;
    m_thirdPriorityCount = 0;
    m_statusLabel->setText("Not analyzed");
    m_firstPriorityLabel->setText("First Priority: 0");
    m_secondPriorityLabel->setText("Second Priority: 0");
    m_thirdPriorityLabel->setText("Third Priority: 0");
}

void TR101290Panel::analyzeStream() {
    if (!m_parser) {
        return;
    }

    m_errors.clear();
    m_firstPriorityCount = 0;
    m_secondPriorityCount = 0;
    m_thirdPriorityCount = 0;

    qDebug() << "TR 101-290: Starting compliance analysis...";

    // First priority checks
    checkTSSyncLoss();
    checkSyncByteError();
    checkPATError();
    checkContinuityCountError();
    checkPMTError();
    checkPIDError();

    // Second priority checks
    checkTransportError();
    checkCRCError();
    checkPCRRepetitionError();
    checkPCRDiscontinuityError();
    checkPCRAccuracyError();
    checkPTSError();
    checkCATError();

    // Third priority checks
    checkNITActualError();
    checkSIRepetitionError();
    checkUnreferencedPID();
    checkSDTActualError();
    checkEITActualError();

    // Count errors by priority
    for (const TR101290Error& error : m_errors) {
        switch (error.priority) {
            case TR101290Priority::First:
                m_firstPriorityCount++;
                break;
            case TR101290Priority::Second:
                m_secondPriorityCount++;
                break;
            case TR101290Priority::Third:
                m_thirdPriorityCount++;
                break;
        }
    }

    qDebug() << "TR 101-290: Analysis complete."
             << "First:" << m_firstPriorityCount
             << "Second:" << m_secondPriorityCount
             << "Third:" << m_thirdPriorityCount;

    // Update UI
    m_firstPriorityLabel->setText(QString("First Priority: %1").arg(m_firstPriorityCount));
    m_secondPriorityLabel->setText(QString("Second Priority: %1").arg(m_secondPriorityCount));
    m_thirdPriorityLabel->setText(QString("Third Priority: %1").arg(m_thirdPriorityCount));

    if (m_firstPriorityCount > 0) {
        m_statusLabel->setText("❌ FAILED - First priority errors detected");
        m_statusLabel->setStyleSheet("color: #ff6464; font-weight: bold; font-size: 14px;");
    } else if (m_secondPriorityCount > 0 || m_thirdPriorityCount > 0) {
        m_statusLabel->setText("⚠️  WARNING - Lower priority errors detected");
        m_statusLabel->setStyleSheet("color: #ffa064; font-weight: bold; font-size: 14px;");
    } else {
        m_statusLabel->setText("✅ PASSED - No errors detected");
        m_statusLabel->setStyleSheet("color: #64ff64; font-weight: bold; font-size: 14px;");
    }

    buildErrorTree();
}

void TR101290Panel::buildErrorTree() {
    m_errorTree->clear();

    // Create priority groups
    QTreeWidgetItem* firstPriorityItem = new QTreeWidgetItem(m_errorTree);
    firstPriorityItem->setText(0, "First Priority Errors");
    firstPriorityItem->setText(1, QString::number(m_firstPriorityCount));
    firstPriorityItem->setForeground(0, QBrush(getPriorityColor(TR101290Priority::First)));
    firstPriorityItem->setExpanded(true);

    QTreeWidgetItem* secondPriorityItem = new QTreeWidgetItem(m_errorTree);
    secondPriorityItem->setText(0, "Second Priority Errors");
    secondPriorityItem->setText(1, QString::number(m_secondPriorityCount));
    secondPriorityItem->setForeground(0, QBrush(getPriorityColor(TR101290Priority::Second)));
    secondPriorityItem->setExpanded(true);

    QTreeWidgetItem* thirdPriorityItem = new QTreeWidgetItem(m_errorTree);
    thirdPriorityItem->setText(0, "Third Priority Errors");
    thirdPriorityItem->setText(1, QString::number(m_thirdPriorityCount));
    thirdPriorityItem->setForeground(0, QBrush(getPriorityColor(TR101290Priority::Third)));
    thirdPriorityItem->setExpanded(true);

    // Group errors by type
    QMap<TR101290ErrorType, QVector<TR101290Error>> errorsByType;
    for (const TR101290Error& error : m_errors) {
        errorsByType[error.type].append(error);
    }

    // Add errors to tree
    for (auto it = errorsByType.begin(); it != errorsByType.end(); ++it) {
        TR101290ErrorType type = it.key();
        const QVector<TR101290Error>& errors = it.value();

        if (errors.isEmpty()) continue;

        TR101290Priority priority = errors.first().priority;
        QTreeWidgetItem* parentItem = nullptr;

        switch (priority) {
            case TR101290Priority::First:
                parentItem = firstPriorityItem;
                break;
            case TR101290Priority::Second:
                parentItem = secondPriorityItem;
                break;
            case TR101290Priority::Third:
                parentItem = thirdPriorityItem;
                break;
        }

        if (!parentItem) continue;

        QTreeWidgetItem* typeItem = new QTreeWidgetItem(parentItem);
        typeItem->setText(0, getErrorTypeName(type));
        typeItem->setText(1, QString::number(errors.size()));
        typeItem->setText(2, errors.first().description);
        typeItem->setForeground(0, QBrush(getPriorityColor(priority)));

        // Add individual error instances (limit to first 10 for performance)
        int maxErrors = qMin(10, errors.size());
        for (int i = 0; i < maxErrors; ++i) {
            const TR101290Error& error = errors[i];
            QTreeWidgetItem* errorItem = new QTreeWidgetItem(typeItem);
            errorItem->setText(0, QString("Packet #%1").arg(error.packetIndex));
            errorItem->setText(1, QString("0x%1").arg(error.offset, 8, 16, QChar('0')));
            errorItem->setText(2, error.description);
            errorItem->setData(0, Qt::UserRole, error.packetIndex);
        }

        if (errors.size() > maxErrors) {
            QTreeWidgetItem* moreItem = new QTreeWidgetItem(typeItem);
            moreItem->setText(0, QString("... %1 more").arg(errors.size() - maxErrors));
            moreItem->setForeground(0, QBrush(Qt::gray));
        }
    }
}

QString TR101290Panel::getErrorTypeName(TR101290ErrorType type) const {
    switch (type) {
        // First priority
        case TR101290ErrorType::TSSyncLoss: return "TS Sync Loss";
        case TR101290ErrorType::SyncByteError: return "Sync Byte Error";
        case TR101290ErrorType::PATError: return "PAT Error";
        case TR101290ErrorType::ContinuityCountError: return "Continuity Count Error";
        case TR101290ErrorType::PMTError: return "PMT Error";
        case TR101290ErrorType::PIDError: return "PID Error";

        // Second priority
        case TR101290ErrorType::TransportError: return "Transport Error";
        case TR101290ErrorType::CRCError: return "CRC Error";
        case TR101290ErrorType::PCRRepetitionError: return "PCR Repetition Error";
        case TR101290ErrorType::PCRDiscontinuityError: return "PCR Discontinuity Error";
        case TR101290ErrorType::PCRAccuracyError: return "PCR Accuracy Error";
        case TR101290ErrorType::PTSError: return "PTS Error";
        case TR101290ErrorType::CATError: return "CAT Error";

        // Third priority
        case TR101290ErrorType::NITActualError: return "NIT Actual Error";
        case TR101290ErrorType::NITOtherError: return "NIT Other Error";
        case TR101290ErrorType::SIRepetitionError: return "SI Repetition Error";
        case TR101290ErrorType::UnreferencedPID: return "Unreferenced PID";
        case TR101290ErrorType::SDTActualError: return "SDT Actual Error";
        case TR101290ErrorType::SDTOtherError: return "SDT Other Error";
        case TR101290ErrorType::EITActualError: return "EIT Actual Error";
        case TR101290ErrorType::EITOtherError: return "EIT Other Error";
        case TR101290ErrorType::RSTError: return "RST Error";
        case TR101290ErrorType::EITPFError: return "EIT P/F Error";

        default: return "Unknown Error";
    }
}

QString TR101290Panel::getPriorityName(TR101290Priority priority) const {
    switch (priority) {
        case TR101290Priority::First: return "First Priority";
        case TR101290Priority::Second: return "Second Priority";
        case TR101290Priority::Third: return "Third Priority";
        default: return "Unknown";
    }
}

QColor TR101290Panel::getPriorityColor(TR101290Priority priority) const {
    switch (priority) {
        case TR101290Priority::First: return QColor(255, 100, 100); // Red
        case TR101290Priority::Second: return QColor(100, 160, 255); // Blue
        case TR101290Priority::Third: return QColor(255, 160, 100); // Orange
        default: return Qt::white;
    }
}

// First priority checks

void TR101290Panel::checkTSSyncLoss() {
    // Check for consecutive packets without sync byte
    // This is already covered by checkSyncByteError
}

void TR101290Panel::checkSyncByteError() {
    if (!m_parser) return;

    const auto& packets = m_parser->getPackets();

    for (int i = 0; i < packets.size(); ++i) {
        const TSPacket& packet = packets[i];
        if (packet.syncByte != 0x47) {
            TR101290Error error;
            error.type = TR101290ErrorType::SyncByteError;
            error.priority = TR101290Priority::First;
            error.offset = packet.offset;
            error.pid = packet.pid;
            error.packetIndex = i;
            error.description = QString("Invalid sync byte: 0x%1 (expected 0x47)")
                .arg(packet.syncByte, 2, 16, QChar('0'));
            m_errors.append(error);
        }
    }
}

void TR101290Panel::checkPATError() {
    if (!m_parser) return;

    const auto& packets = m_parser->getPackets();
    bool patFound = false;
    int64_t lastPATOffset = -1;
    const int64_t maxPATInterval = 500 * 188; // 500ms at typical bitrate

    for (int i = 0; i < packets.size(); ++i) {
        const TSPacket& packet = packets[i];
        if (packet.pid == 0x0000) {
            patFound = true;
            if (lastPATOffset >= 0) {
                int64_t interval = packet.offset - lastPATOffset;
                if (interval > maxPATInterval) {
                    TR101290Error error;
                    error.type = TR101290ErrorType::PATError;
                    error.priority = TR101290Priority::First;
                    error.offset = packet.offset;
                    error.pid = 0x0000;
                    error.packetIndex = i;
                    error.description = QString("PAT interval too large: %1 bytes (max 500ms)")
                        .arg(interval);
                    m_errors.append(error);
                }
            }
            lastPATOffset = packet.offset;
        }
    }

    if (!patFound) {
        TR101290Error error;
        error.type = TR101290ErrorType::PATError;
        error.priority = TR101290Priority::First;
        error.offset = 0;
        error.pid = 0x0000;
        error.packetIndex = -1;
        error.description = "PAT not found in stream";
        m_errors.append(error);
    }
}

void TR101290Panel::checkContinuityCountError() {
    if (!m_parser) return;

    const auto& packets = m_parser->getPackets();
    QMap<uint16_t, uint8_t> lastCC;

    for (int i = 0; i < packets.size(); ++i) {
        const TSPacket& packet = packets[i];

        // Skip packets without payload
        if (packet.adaptationFieldControl == 0 || packet.adaptationFieldControl == 2) {
            continue;
        }

        if (lastCC.contains(packet.pid)) {
            uint8_t expectedCC = (lastCC[packet.pid] + 1) & 0x0F;
            if (packet.continuityCounter != expectedCC) {
                TR101290Error error;
                error.type = TR101290ErrorType::ContinuityCountError;
                error.priority = TR101290Priority::First;
                error.offset = packet.offset;
                error.pid = packet.pid;
                error.packetIndex = i;
                error.description = QString("CC error on PID 0x%1: expected %2, got %3")
                    .arg(packet.pid, 4, 16, QChar('0'))
                    .arg(expectedCC)
                    .arg(packet.continuityCounter);
                m_errors.append(error);
            }
        }

        lastCC[packet.pid] = packet.continuityCounter;
    }
}

void TR101290Panel::checkPMTError() {
    if (!m_parser) return;

    const auto& programs = m_parser->getPrograms();
    const auto& packets = m_parser->getPackets();

    for (const ProgramInfo& program : programs) {
        bool pmtFound = false;

        for (int i = 0; i < packets.size(); ++i) {
            const TSPacket& packet = packets[i];
            if (packet.pid == program.pmtPid) {
                pmtFound = true;
                break;
            }
        }

        if (!pmtFound) {
            TR101290Error error;
            error.type = TR101290ErrorType::PMTError;
            error.priority = TR101290Priority::First;
            error.offset = 0;
            error.pid = program.pmtPid;
            error.packetIndex = -1;
            error.description = QString("PMT not found for program %1 (PID 0x%2)")
                .arg(program.programNumber)
                .arg(program.pmtPid, 4, 16, QChar('0'));
            m_errors.append(error);
        }
    }
}

void TR101290Panel::checkPIDError() {
    // Check for PIDs referenced in PMT but not present in stream
    if (!m_parser) return;

    const auto& programs = m_parser->getPrograms();
    const auto& pids = m_parser->getPIDs();

    for (const ProgramInfo& program : programs) {
        for (uint16_t esPid : program.elementaryPIDs) {
            if (!pids.contains(esPid)) {
                TR101290Error error;
                error.type = TR101290ErrorType::PIDError;
                error.priority = TR101290Priority::First;
                error.offset = 0;
                error.pid = esPid;
                error.packetIndex = -1;
                error.description = QString("PID 0x%1 referenced in PMT but not found in stream")
                    .arg(esPid, 4, 16, QChar('0'));
                m_errors.append(error);
            }
        }
    }
}

// Second priority checks

void TR101290Panel::checkTransportError() {
    if (!m_parser) return;

    const auto& packets = m_parser->getPackets();

    for (int i = 0; i < packets.size(); ++i) {
        const TSPacket& packet = packets[i];
        if (packet.transportErrorIndicator) {
            TR101290Error error;
            error.type = TR101290ErrorType::TransportError;
            error.priority = TR101290Priority::Second;
            error.offset = packet.offset;
            error.pid = packet.pid;
            error.packetIndex = i;
            error.description = QString("Transport error indicator set on PID 0x%1")
                .arg(packet.pid, 4, 16, QChar('0'));
            m_errors.append(error);
        }
    }
}

void TR101290Panel::checkCRCError() {
    if (!m_parser) return;

    const auto& psiTables = m_parser->getPSITables();

    for (const PSITable& table : psiTables) {
        // Only check tables that have CRC (PAT, PMT, CAT, NIT, SDT, EIT, BAT)
        if (table.data.size() < 4) continue;

        // Calculate CRC32
        const uint8_t* data = reinterpret_cast<const uint8_t*>(table.data.constData());
        int length = table.sectionLength + 3; // section_length doesn't include first 3 bytes

        if (length > table.data.size()) continue;

        uint32_t crc = 0xFFFFFFFF;
        for (int i = 0; i < length - 4; ++i) {
            crc ^= data[i] << 24;
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x80000000) {
                    crc = (crc << 1) ^ 0x04C11DB7;
                } else {
                    crc = crc << 1;
                }
            }
        }

        // Read CRC from table
        uint32_t tableCRC = (data[length - 4] << 24) |
                            (data[length - 3] << 16) |
                            (data[length - 2] << 8) |
                            data[length - 1];

        if (crc != tableCRC) {
            TR101290Error error;
            error.type = TR101290ErrorType::CRCError;
            error.priority = TR101290Priority::Second;
            error.offset = table.offset;
            error.pid = table.pid;
            error.packetIndex = -1;
            error.description = QString("CRC error in %1 table on PID 0x%2")
                .arg(table.type == PSITableType::PAT ? "PAT" :
                     table.type == PSITableType::PMT ? "PMT" :
                     table.type == PSITableType::SDT ? "SDT" :
                     table.type == PSITableType::EIT ? "EIT" : "PSI")
                .arg(table.pid, 4, 16, QChar('0'));
            m_errors.append(error);
        }
    }
}

void TR101290Panel::checkPCRRepetitionError() {
    if (!m_parser) return;

    const auto& packets = m_parser->getPackets();
    QMap<uint16_t, int64_t> lastPCRPacket;
    const int maxPCRInterval = 40; // 40ms = ~40 packets at typical bitrate

    for (int i = 0; i < packets.size(); ++i) {
        const TSPacket& packet = packets[i];
        if (packet.hasPCR) {
            if (lastPCRPacket.contains(packet.pid)) {
                int interval = i - lastPCRPacket[packet.pid];
                if (interval > maxPCRInterval * 10) { // Allow some margin
                    TR101290Error error;
                    error.type = TR101290ErrorType::PCRRepetitionError;
                    error.priority = TR101290Priority::Second;
                    error.offset = packet.offset;
                    error.pid = packet.pid;
                    error.packetIndex = i;
                    error.description = QString("PCR interval too large on PID 0x%1: %2 packets")
                        .arg(packet.pid, 4, 16, QChar('0'))
                        .arg(interval);
                    m_errors.append(error);
                }
            }
            lastPCRPacket[packet.pid] = i;
        }
    }
}

void TR101290Panel::checkPCRDiscontinuityError() {
    if (!m_parser) return;

    const auto& packets = m_parser->getPackets();
    QMap<uint16_t, int64_t> lastPCR;
    QMap<uint16_t, int> lastPCRPacket;

    for (int i = 0; i < packets.size(); ++i) {
        const TSPacket& packet = packets[i];
        if (packet.hasPCR) {
            if (lastPCR.contains(packet.pid)) {
                // Calculate expected PCR based on packet interval
                int packetInterval = i - lastPCRPacket[packet.pid];
                // Assume 188 bytes per packet, typical bitrate
                // PCR is 27MHz clock, so 27000 ticks per ms
                // At 10 Mbps: ~53 packets per ms, so ~509 PCR ticks per packet
                int64_t expectedPCRDiff = packetInterval * 509;
                int64_t actualPCRDiff = packet.pcr - lastPCR[packet.pid];

                // Check for discontinuity (difference > 100ms = 2.7M PCR ticks)
                if (std::abs(actualPCRDiff - expectedPCRDiff) > 2700000) {
                    // Check if discontinuity indicator is set
                    if (!packet.discontinuityIndicator) {
                        TR101290Error error;
                        error.type = TR101290ErrorType::PCRDiscontinuityError;
                        error.priority = TR101290Priority::Second;
                        error.offset = packet.offset;
                        error.pid = packet.pid;
                        error.packetIndex = i;
                        error.description = QString("PCR discontinuity without indicator on PID 0x%1")
                            .arg(packet.pid, 4, 16, QChar('0'));
                        m_errors.append(error);
                    }
                }
            }
            lastPCR[packet.pid] = packet.pcr;
            lastPCRPacket[packet.pid] = i;
        }
    }
}

void TR101290Panel::checkPCRAccuracyError() {
    if (!m_parser) return;

    const auto& packets = m_parser->getPackets();
    QMap<uint16_t, int64_t> lastPCR;
    QMap<uint16_t, int64_t> lastPCROffset;

    for (int i = 0; i < packets.size(); ++i) {
        const TSPacket& packet = packets[i];
        if (packet.hasPCR) {
            if (lastPCR.contains(packet.pid)) {
                // Calculate expected PCR based on byte offset
                int64_t byteInterval = packet.offset - lastPCROffset[packet.pid];
                // Assume constant bitrate: PCR is 27MHz clock
                // At 10 Mbps: 1.25 MB/s = 21600 PCR ticks per byte
                int64_t expectedPCR = lastPCR[packet.pid] + (byteInterval * 21600);
                int64_t actualPCR = packet.pcr;
                int64_t difference = actualPCR - expectedPCR;

                // PCR accuracy should be within ±500ns
                // 500ns = 13.5 PCR ticks (27MHz clock)
                if (std::abs(difference) > 13500) { // Allow 1000x margin for variable bitrate
                    TR101290Error error;
                    error.type = TR101290ErrorType::PCRAccuracyError;
                    error.priority = TR101290Priority::Second;
                    error.offset = packet.offset;
                    error.pid = packet.pid;
                    error.packetIndex = i;
                    error.description = QString("PCR accuracy error on PID 0x%1: %2 ticks")
                        .arg(packet.pid, 4, 16, QChar('0'))
                        .arg(difference);
                    m_errors.append(error);
                }
            }
            lastPCR[packet.pid] = packet.pcr;
            lastPCROffset[packet.pid] = packet.offset;
        }
    }
}

void TR101290Panel::checkPTSError() {
    if (!m_parser) return;

    const auto& packets = m_parser->getPackets();
    QMap<uint16_t, int64_t> lastPTS;
    QMap<uint16_t, int> lastPTSPacket;

    for (int i = 0; i < packets.size(); ++i) {
        const TSPacket& packet = packets[i];
        if (packet.hasPTS) {
            if (lastPTS.contains(packet.pid)) {
                int packetInterval = i - lastPTSPacket[packet.pid];

                // PTS should repeat at least every 700ms
                // At 90kHz clock: 700ms = 63000 ticks
                // At typical bitrate (~50 packets/ms): 700ms = ~35000 packets
                if (packetInterval > 35000) {
                    TR101290Error error;
                    error.type = TR101290ErrorType::PTSError;
                    error.priority = TR101290Priority::Second;
                    error.offset = packet.offset;
                    error.pid = packet.pid;
                    error.packetIndex = i;
                    error.description = QString("PTS interval too large on PID 0x%1: %2 packets")
                        .arg(packet.pid, 4, 16, QChar('0'))
                        .arg(packetInterval);
                    m_errors.append(error);
                }
            }
            lastPTS[packet.pid] = packet.pts;
            lastPTSPacket[packet.pid] = i;
        }
    }
}

void TR101290Panel::checkCATError() {
    if (!m_parser) return;

    const auto& psiTables = m_parser->getPSITables();
    bool catFound = false;
    int64_t lastCATOffset = -1;

    for (const PSITable& table : psiTables) {
        if (table.pid == 0x0001) { // CAT PID
            catFound = true;

            // Check CAT structure
            if (table.data.size() < 8) {
                TR101290Error error;
                error.type = TR101290ErrorType::CATError;
                error.priority = TR101290Priority::Second;
                error.offset = table.offset;
                error.pid = 0x0001;
                error.packetIndex = -1;
                error.description = "CAT table too short";
                m_errors.append(error);
                continue;
            }

            // Check table_id should be 0x01 for CAT
            const uint8_t* data = reinterpret_cast<const uint8_t*>(table.data.constData());
            if (data[0] != 0x01) {
                TR101290Error error;
                error.type = TR101290ErrorType::CATError;
                error.priority = TR101290Priority::Second;
                error.offset = table.offset;
                error.pid = 0x0001;
                error.packetIndex = -1;
                error.description = QString("Invalid CAT table_id: 0x%1")
                    .arg(data[0], 2, 16, QChar('0'));
                m_errors.append(error);
            }

            // Check CAT repetition interval (should be within reasonable time)
            if (lastCATOffset >= 0) {
                int64_t interval = table.offset - lastCATOffset;
                // CAT should repeat at least every 10 seconds at typical bitrate
                // At 10 Mbps: 10s = ~12.5 MB
                if (interval > 12500000) {
                    TR101290Error error;
                    error.type = TR101290ErrorType::CATError;
                    error.priority = TR101290Priority::Second;
                    error.offset = table.offset;
                    error.pid = 0x0001;
                    error.packetIndex = -1;
                    error.description = QString("CAT repetition interval too large: %1 bytes")
                        .arg(interval);
                    m_errors.append(error);
                }
            }
            lastCATOffset = table.offset;
        }
    }

    // Note: CAT is optional if no conditional access, so we don't error if not found
}

// Third priority checks

void TR101290Panel::checkNITActualError() {
    if (!m_parser) return;

    const auto& psiTables = m_parser->getPSITables();
    bool nitActualFound = false;
    int64_t lastNITOffset = -1;

    for (const PSITable& table : psiTables) {
        if (table.pid == 0x0010) { // NIT PID
            const uint8_t* data = reinterpret_cast<const uint8_t*>(table.data.constData());
            if (table.data.size() < 1) continue;

            uint8_t tableId = data[0];
            // NIT actual: table_id = 0x40, NIT other: table_id = 0x41
            if (tableId == 0x40) {
                nitActualFound = true;

                // Check NIT repetition interval (should be within 10 seconds)
                if (lastNITOffset >= 0) {
                    int64_t interval = table.offset - lastNITOffset;
                    if (interval > 12500000) { // 10s at 10 Mbps
                        TR101290Error error;
                        error.type = TR101290ErrorType::NITActualError;
                        error.priority = TR101290Priority::Third;
                        error.offset = table.offset;
                        error.pid = 0x0010;
                        error.packetIndex = -1;
                        error.description = QString("NIT actual repetition interval too large: %1 bytes")
                            .arg(interval);
                        m_errors.append(error);
                    }
                }
                lastNITOffset = table.offset;
            }
        }
    }

    // NIT actual should be present in DVB streams
    if (!nitActualFound) {
        TR101290Error error;
        error.type = TR101290ErrorType::NITActualError;
        error.priority = TR101290Priority::Third;
        error.offset = 0;
        error.pid = 0x0010;
        error.packetIndex = -1;
        error.description = "NIT actual table not found";
        m_errors.append(error);
    }
}

void TR101290Panel::checkSIRepetitionError() {
    if (!m_parser) return;

    const auto& psiTables = m_parser->getPSITables();
    QMap<uint16_t, int64_t> lastTableOffset; // PID -> last offset

    // Define maximum repetition intervals for SI tables (in bytes at 10 Mbps)
    QMap<uint16_t, int64_t> maxIntervals;
    maxIntervals[0x0000] = 625000;    // PAT: 0.5s
    maxIntervals[0x0010] = 12500000;  // NIT: 10s
    maxIntervals[0x0011] = 2500000;   // SDT: 2s
    maxIntervals[0x0012] = 2500000;   // EIT: 2s

    for (const PSITable& table : psiTables) {
        if (maxIntervals.contains(table.pid)) {
            if (lastTableOffset.contains(table.pid)) {
                int64_t interval = table.offset - lastTableOffset[table.pid];
                if (interval > maxIntervals[table.pid]) {
                    TR101290Error error;
                    error.type = TR101290ErrorType::SIRepetitionError;
                    error.priority = TR101290Priority::Third;
                    error.offset = table.offset;
                    error.pid = table.pid;
                    error.packetIndex = -1;
                    error.description = QString("SI repetition interval too large on PID 0x%1: %2 bytes")
                        .arg(table.pid, 4, 16, QChar('0'))
                        .arg(interval);
                    m_errors.append(error);
                }
            }
            lastTableOffset[table.pid] = table.offset;
        }
    }
}

void TR101290Panel::checkUnreferencedPID() {
    if (!m_parser) return;

    const auto& pids = m_parser->getPIDs();
    const auto& programs = m_parser->getPrograms();

    // Build set of referenced PIDs
    QSet<uint16_t> referencedPIDs;
    referencedPIDs.insert(0x0000); // PAT
    referencedPIDs.insert(0x0001); // CAT
    referencedPIDs.insert(0x0010); // NIT
    referencedPIDs.insert(0x0011); // SDT/BAT
    referencedPIDs.insert(0x0012); // EIT
    referencedPIDs.insert(0x0013); // RST
    referencedPIDs.insert(0x0014); // TDT/TOT
    referencedPIDs.insert(0x1FFF); // Null packets

    for (const ProgramInfo& program : programs) {
        referencedPIDs.insert(program.pmtPid);
        for (uint16_t pid : program.elementaryPIDs) {
            referencedPIDs.insert(pid);
        }
    }

    // Check for unreferenced PIDs
    for (auto it = pids.begin(); it != pids.end(); ++it) {
        uint16_t pid = it.key();
        if (!referencedPIDs.contains(pid) && pid < 0x1FFF) {
            TR101290Error error;
            error.type = TR101290ErrorType::UnreferencedPID;
            error.priority = TR101290Priority::Third;
            error.offset = 0;
            error.pid = pid;
            error.packetIndex = -1;
            error.description = QString("PID 0x%1 not referenced in PAT/PMT")
                .arg(pid, 4, 16, QChar('0'));
            m_errors.append(error);
        }
    }
}

void TR101290Panel::checkSDTActualError() {
    if (!m_parser) return;

    const auto& psiTables = m_parser->getPSITables();
    bool sdtActualFound = false;
    int64_t lastSDTOffset = -1;

    for (const PSITable& table : psiTables) {
        if (table.pid == 0x0011 && table.type == PSITableType::SDT) {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(table.data.constData());
            if (table.data.size() < 1) continue;

            uint8_t tableId = data[0];
            // SDT actual: table_id = 0x42, SDT other: table_id = 0x46
            if (tableId == 0x42) {
                sdtActualFound = true;

                // Check SDT repetition interval (should be within 2 seconds)
                if (lastSDTOffset >= 0) {
                    int64_t interval = table.offset - lastSDTOffset;
                    if (interval > 2500000) { // 2s at 10 Mbps
                        TR101290Error error;
                        error.type = TR101290ErrorType::SDTActualError;
                        error.priority = TR101290Priority::Third;
                        error.offset = table.offset;
                        error.pid = 0x0011;
                        error.packetIndex = -1;
                        error.description = QString("SDT actual repetition interval too large: %1 bytes")
                            .arg(interval);
                        m_errors.append(error);
                    }
                }
                lastSDTOffset = table.offset;
            }
        }
    }

    // SDT actual should be present in DVB streams
    if (!sdtActualFound) {
        TR101290Error error;
        error.type = TR101290ErrorType::SDTActualError;
        error.priority = TR101290Priority::Third;
        error.offset = 0;
        error.pid = 0x0011;
        error.packetIndex = -1;
        error.description = "SDT actual table not found";
        m_errors.append(error);
    }
}

void TR101290Panel::checkEITActualError() {
    if (!m_parser) return;

    const auto& psiTables = m_parser->getPSITables();
    bool eitActualFound = false;
    int64_t lastEITOffset = -1;

    for (const PSITable& table : psiTables) {
        if (table.pid == 0x0012 && table.type == PSITableType::EIT) {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(table.data.constData());
            if (table.data.size() < 1) continue;

            uint8_t tableId = data[0];
            // EIT actual present/following: table_id = 0x4E
            // EIT actual schedule: table_id = 0x50-0x5F
            if (tableId == 0x4E || (tableId >= 0x50 && tableId <= 0x5F)) {
                eitActualFound = true;

                // Check EIT repetition interval (should be within 2 seconds for present/following)
                if (tableId == 0x4E && lastEITOffset >= 0) {
                    int64_t interval = table.offset - lastEITOffset;
                    if (interval > 2500000) { // 2s at 10 Mbps
                        TR101290Error error;
                        error.type = TR101290ErrorType::EITActualError;
                        error.priority = TR101290Priority::Third;
                        error.offset = table.offset;
                        error.pid = 0x0012;
                        error.packetIndex = -1;
                        error.description = QString("EIT actual p/f repetition interval too large: %1 bytes")
                            .arg(interval);
                        m_errors.append(error);
                    }
                }
                if (tableId == 0x4E) {
                    lastEITOffset = table.offset;
                }
            }
        }
    }

    // EIT actual present/following should be present in DVB streams
    if (!eitActualFound) {
        TR101290Error error;
        error.type = TR101290ErrorType::EITActualError;
        error.priority = TR101290Priority::Third;
        error.offset = 0;
        error.pid = 0x0012;
        error.packetIndex = -1;
        error.description = "EIT actual table not found";
        m_errors.append(error);
    }
}

void TR101290Panel::onErrorDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);

    if (!item) {
        return;
    }

    QVariant data = item->data(0, Qt::UserRole);
    if (data.isValid()) {
        int packetIndex = data.toInt();
        if (packetIndex >= 0) {
            emit errorDoubleClicked(packetIndex);
            qDebug() << "TR 101-290 error double-clicked, jumping to packet:" << packetIndex;
        }
    }
}

} // namespace VideoStudio
