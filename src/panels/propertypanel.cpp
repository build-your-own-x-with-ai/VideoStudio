#include "panels/propertypanel.h"
#include <QHeaderView>
#include <QDebug>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>

namespace VideoStudio {

PropertyPanel::PropertyPanel(QWidget* parent)
    : QWidget(parent)
    , m_tsParser(nullptr)
    , m_mp4Parser(nullptr)
    , m_mkvParser(nullptr)
    , m_aviParser(nullptr)
    , m_flvParser(nullptr)
    , m_mode(PropertyMode::Sync)
    , m_lastPacketIndex(-1)
    , m_lastAtomOffset(-1)
    , m_lastElementOffset(-1)
    , m_lastChunkOffset(-1)
    , m_lastTagOffset(-1)
    , m_selectedPID(0xFFFF)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    createToolbar();
    layout->addWidget(m_toolbar);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels(QStringList() << "Property" << "Value");
    m_treeWidget->setColumnWidth(0, 200);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested,
            this, &PropertyPanel::onContextMenu);

    layout->addWidget(m_treeWidget);
}

PropertyPanel::~PropertyPanel() {
}

void PropertyPanel::createToolbar() {
    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(16, 16));

    // Mode actions
    m_syncAction = m_toolbar->addAction("Sync");
    m_syncAction->setCheckable(true);
    m_syncAction->setChecked(true);
    m_syncAction->setToolTip("Auto-update when clicking packets");
    connect(m_syncAction, &QAction::triggered, this, [this]() {
        setMode(PropertyMode::Sync);
    });

    m_compareAction = m_toolbar->addAction("Compare");
    m_compareAction->setCheckable(true);
    m_compareAction->setToolTip("Compare with previous packet");
    connect(m_compareAction, &QAction::triggered, this, [this]() {
        setMode(PropertyMode::Compare);
    });

    m_dumpAction = m_toolbar->addAction("Dump");
    m_dumpAction->setCheckable(true);
    m_dumpAction->setToolTip("Dump elementary stream");
    connect(m_dumpAction, &QAction::triggered, this, [this]() {
        setMode(PropertyMode::Dump);
    });
}

void PropertyPanel::setTSParser(TSParser* parser) {
    m_tsParser = parser;
    m_mp4Parser = nullptr;
    m_mkvParser = nullptr;
    m_aviParser = nullptr;
    m_flvParser = nullptr;
    m_treeWidget->clear();
    m_lastPacketIndex = -1;
    m_lastAtomOffset = -1;
    m_lastElementOffset = -1;
    m_lastChunkOffset = -1;
    m_lastTagOffset = -1;
}

void PropertyPanel::setMP4Parser(MP4Parser* parser) {
    m_mp4Parser = parser;
    m_tsParser = nullptr;
    m_mkvParser = nullptr;
    m_aviParser = nullptr;
    m_flvParser = nullptr;
    m_treeWidget->clear();
    m_lastPacketIndex = -1;
    m_lastAtomOffset = -1;
    m_lastElementOffset = -1;
    m_lastChunkOffset = -1;
    m_lastTagOffset = -1;
}

void PropertyPanel::setMKVParser(MKVParser* parser) {
    m_mkvParser = parser;
    m_tsParser = nullptr;
    m_mp4Parser = nullptr;
    m_aviParser = nullptr;
    m_flvParser = nullptr;
    m_treeWidget->clear();
    m_lastPacketIndex = -1;
    m_lastAtomOffset = -1;
    m_lastElementOffset = -1;
    m_lastChunkOffset = -1;
    m_lastTagOffset = -1;
}

void PropertyPanel::setAVIParser(AVIParser* parser) {
    m_aviParser = parser;
    m_tsParser = nullptr;
    m_mp4Parser = nullptr;
    m_mkvParser = nullptr;
    m_flvParser = nullptr;
    m_treeWidget->clear();
    m_lastPacketIndex = -1;
    m_lastAtomOffset = -1;
    m_lastElementOffset = -1;
    m_lastChunkOffset = -1;
    m_lastTagOffset = -1;
}

void PropertyPanel::setFLVParser(FLVParser* parser) {
    m_flvParser = parser;
    m_tsParser = nullptr;
    m_mp4Parser = nullptr;
    m_mkvParser = nullptr;
    m_aviParser = nullptr;
    m_treeWidget->clear();
    m_lastPacketIndex = -1;
    m_lastAtomOffset = -1;
    m_lastElementOffset = -1;
    m_lastChunkOffset = -1;
    m_lastTagOffset = -1;
}

void PropertyPanel::setMode(PropertyMode mode) {
    m_mode = mode;

    // Update action states
    m_syncAction->setChecked(mode == PropertyMode::Sync);
    m_compareAction->setChecked(mode == PropertyMode::Compare);
    m_dumpAction->setChecked(mode == PropertyMode::Dump);

    // Refresh display if we have a packet
    if (m_lastPacketIndex >= 0) {
        displayPacket(m_lastPacketIndex);
    }
}

void PropertyPanel::displayPacket(int packetIndex) {
    if (!m_tsParser || packetIndex < 0) {
        return;
    }

    const auto& packets = m_tsParser->getPackets();
    if (packetIndex >= packets.size()) {
        return;
    }

    m_lastPacketIndex = packetIndex;

    switch (m_mode) {
    case PropertyMode::Sync:
        displayPacketSync(packetIndex);
        break;
    case PropertyMode::Compare:
        displayPacketCompare(packetIndex);
        break;
    case PropertyMode::Dump:
        displayPacketSync(packetIndex);
        break;
    }
}

void PropertyPanel::displayAtom(int64_t offset) {
    if (!m_mp4Parser || offset < 0) {
        return;
    }

    const MP4Atom* atom = findAtomByOffset(m_mp4Parser->getAtoms(), offset);
    if (!atom) {
        return;
    }

    switch (m_mode) {
    case PropertyMode::Sync:
        displayAtomSync(atom);
        m_lastAtomOffset = offset;
        break;
    case PropertyMode::Compare:
        displayAtomCompare(atom);
        // Update last offset AFTER comparison so next click compares against this one
        m_lastAtomOffset = offset;
        break;
    case PropertyMode::Dump:
        displayAtomSync(atom);
        m_lastAtomOffset = offset;
        break;
    }
}

void PropertyPanel::displayPacketSync(int packetIndex) {
    m_treeWidget->clear();

    const auto& packets = m_tsParser->getPackets();
    const TSPacket& packet = packets[packetIndex];

    // Root item: Packet info
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
    rootItem->setText(0, QString("Packet #%1").arg(packetIndex));
    rootItem->setText(1, QString("Offset: 0x%1").arg(packet.offset, 0, 16));
    rootItem->setExpanded(true);

    addPacketFields(rootItem, packet);

    // If this is a PSI table packet, parse and display table
    if (packet.pid == 0x0000 || packet.pid == 0x0001 ||
        (packet.pid >= 0x0010 && packet.pid <= 0x1FFE)) {
        addPSITableFields(rootItem, packet);
    }
}

void PropertyPanel::displayPacketCompare(int packetIndex) {
    m_treeWidget->clear();

    const auto& packets = m_tsParser->getPackets();
    const TSPacket& packet = packets[packetIndex];

    // Find previous packet with same PID
    int prevIndex = -1;
    for (int i = packetIndex - 1; i >= 0; --i) {
        if (packets[i].pid == packet.pid) {
            prevIndex = i;
            break;
        }
    }

    // Root item: Packet info
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
    rootItem->setText(0, QString("Packet #%1").arg(packetIndex));
    if (prevIndex >= 0) {
        rootItem->setText(1, QString("Compare with #%1").arg(prevIndex));
    } else {
        rootItem->setText(1, "No previous packet");
    }
    rootItem->setExpanded(true);

    addPacketFields(rootItem, packet);

    // Highlight differences if we have a previous packet
    if (prevIndex >= 0) {
        const TSPacket& prevPacket = packets[prevIndex];

        // Compare continuity counter
        if (packet.continuityCounter != prevPacket.continuityCounter) {
            for (int i = 0; i < rootItem->childCount(); ++i) {
                QTreeWidgetItem* child = rootItem->child(i);
                if (child->text(0) == "Continuity Counter") {
                    child->setForeground(1, QBrush(QColor(100, 150, 255))); // Blue
                }
            }
        }
    }
}

void PropertyPanel::addPacketFields(QTreeWidgetItem* parent, const TSPacket& packet) {
    // Sync byte
    QTreeWidgetItem* syncItem = new QTreeWidgetItem(parent);
    syncItem->setText(0, "Sync Byte");
    syncItem->setText(1, QString("0x%1").arg(packet.syncByte, 2, 16, QChar('0')));

    // Transport Error Indicator
    QTreeWidgetItem* teiItem = new QTreeWidgetItem(parent);
    teiItem->setText(0, "Transport Error Indicator");
    teiItem->setText(1, packet.transportErrorIndicator ? "1 (Error)" : "0");
    if (packet.transportErrorIndicator) {
        teiItem->setForeground(1, QBrush(Qt::red));
    }

    // Payload Unit Start Indicator
    QTreeWidgetItem* pusiItem = new QTreeWidgetItem(parent);
    pusiItem->setText(0, "Payload Unit Start Indicator");
    pusiItem->setText(1, packet.payloadUnitStartIndicator ? "1" : "0");

    // Transport Priority
    QTreeWidgetItem* tpItem = new QTreeWidgetItem(parent);
    tpItem->setText(0, "Transport Priority");
    tpItem->setText(1, packet.transportPriority ? "1" : "0");

    // PID
    QTreeWidgetItem* pidItem = new QTreeWidgetItem(parent);
    pidItem->setText(0, "PID");
    pidItem->setText(1, QString("0x%1 (%2)").arg(packet.pid, 4, 16, QChar('0')).arg(packet.pid));

    // Scrambling Control
    QTreeWidgetItem* scItem = new QTreeWidgetItem(parent);
    scItem->setText(0, "Scrambling Control");
    QString scText;
    switch (packet.scramblingControl) {
    case 0: scText = "Not scrambled"; break;
    case 1: scText = "Reserved"; break;
    case 2: scText = "Even key"; break;
    case 3: scText = "Odd key"; break;
    }
    scItem->setText(1, QString("%1 (%2)").arg(packet.scramblingControl).arg(scText));

    // Adaptation Field Control
    QTreeWidgetItem* afcItem = new QTreeWidgetItem(parent);
    afcItem->setText(0, "Adaptation Field Control");
    QString afcText;
    switch (packet.adaptationFieldControl) {
    case 0: afcText = "Reserved"; break;
    case 1: afcText = "Payload only"; break;
    case 2: afcText = "Adaptation field only"; break;
    case 3: afcText = "Adaptation field + payload"; break;
    }
    afcItem->setText(1, QString("%1 (%2)").arg(packet.adaptationFieldControl).arg(afcText));

    // Continuity Counter
    QTreeWidgetItem* ccItem = new QTreeWidgetItem(parent);
    ccItem->setText(0, "Continuity Counter");
    ccItem->setText(1, QString::number(packet.continuityCounter));

    // Adaptation field details
    if (packet.hasAdaptationField) {
        QTreeWidgetItem* afItem = new QTreeWidgetItem(parent);
        afItem->setText(0, "Adaptation Field");
        afItem->setText(1, "Present");
        afItem->setExpanded(true);

        if (packet.hasPCR) {
            QTreeWidgetItem* pcrItem = new QTreeWidgetItem(afItem);
            pcrItem->setText(0, "PCR");
            pcrItem->setText(1, formatTimestamp(packet.pcr));
        }
    }

    // PTS/DTS
    if (packet.hasPTS) {
        QTreeWidgetItem* ptsItem = new QTreeWidgetItem(parent);
        ptsItem->setText(0, "PTS");
        ptsItem->setText(1, formatTimestamp(packet.pts));
    }

    if (packet.hasDTS) {
        QTreeWidgetItem* dtsItem = new QTreeWidgetItem(parent);
        dtsItem->setText(0, "DTS");
        dtsItem->setText(1, formatTimestamp(packet.dts));
    }

    // Payload size
    QTreeWidgetItem* payloadItem = new QTreeWidgetItem(parent);
    payloadItem->setText(0, "Payload Size");
    payloadItem->setText(1, QString("%1 bytes").arg(packet.payload.size()));
}

void PropertyPanel::addPSITableFields(QTreeWidgetItem* parent, const TSPacket& packet) {
    // TODO: Parse and display PSI/SI table fields
    // This would parse PAT, PMT, SDT, etc. based on PID and table_id

    QTreeWidgetItem* tableItem = new QTreeWidgetItem(parent);
    tableItem->setText(0, "PSI/SI Table");

    if (packet.pid == 0x0000) {
        tableItem->setText(1, "PAT (Program Association Table)");
    } else if (packet.pid == 0x0001) {
        tableItem->setText(1, "CAT (Conditional Access Table)");
    } else if (packet.pid == 0x0011) {
        tableItem->setText(1, "SDT (Service Description Table)");
    } else {
        tableItem->setText(1, "PMT or other");
    }
}

QString PropertyPanel::formatTimestamp(int64_t timestamp) {
    // Format as: timestamp (HH:MM:SS.mmm)
    double seconds = timestamp / 90000.0;
    int hours = static_cast<int>(seconds / 3600);
    int minutes = static_cast<int>((seconds - hours * 3600) / 60);
    double secs = seconds - hours * 3600 - minutes * 60;

    return QString("%1 (0x%2) = %3:%4:%5")
        .arg(timestamp)
        .arg(timestamp, 0, 16)
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs, 6, 'f', 3, QChar('0'));
}

void PropertyPanel::onPIDSelected(uint16_t pid) {
    m_selectedPID = pid;

    // Find first packet with this PID
    if (!m_tsParser) {
        return;
    }

    const auto& packets = m_tsParser->getPackets();
    for (int i = 0; i < packets.size(); ++i) {
        if (packets[i].pid == pid) {
            displayPacket(i);
            break;
        }
    }
}

void PropertyPanel::onContextMenu(const QPoint& pos) {
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    if (!item) {
        return;
    }

    // Only show menu for leaf items (items with values)
    if (item->childCount() > 0) {
        return;
    }

    QMenu menu(this);
    QAction* addToGraphicsAction = menu.addAction("Add to Graphics");
    connect(addToGraphicsAction, &QAction::triggered, this, &PropertyPanel::onAddToGraphics);

    menu.exec(m_treeWidget->viewport()->mapToGlobal(pos));
}

void PropertyPanel::onAddToGraphics() {
    QTreeWidgetItem* item = m_treeWidget->currentItem();
    if (!item || !m_tsParser) {
        return;
    }

    QString parameterName = item->text(0);
    QString valueText = item->text(1);

    qDebug() << "Adding parameter to Graphics:" << parameterName << "=" << valueText;

    // Extract parameter values from all packets
    QVector<double> values;
    QVector<int64_t> offsets;

    const auto& packets = m_tsParser->getPackets();

    // Determine which parameter to extract based on name
    if (parameterName == "PID") {
        for (const TSPacket& packet : packets) {
            values.append(packet.pid);
            offsets.append(packet.offset);
        }
    } else if (parameterName == "Continuity Counter") {
        for (const TSPacket& packet : packets) {
            values.append(packet.continuityCounter);
            offsets.append(packet.offset);
        }
    } else if (parameterName == "Adaptation Field Control") {
        for (const TSPacket& packet : packets) {
            values.append(packet.adaptationFieldControl);
            offsets.append(packet.offset);
        }
    } else if (parameterName == "Scrambling Control") {
        for (const TSPacket& packet : packets) {
            values.append(packet.scramblingControl);
            offsets.append(packet.offset);
        }
    } else if (parameterName == "Transport Error Indicator") {
        for (const TSPacket& packet : packets) {
            values.append(packet.transportErrorIndicator ? 1.0 : 0.0);
            offsets.append(packet.offset);
        }
    } else if (parameterName == "Payload Unit Start Indicator") {
        for (const TSPacket& packet : packets) {
            values.append(packet.payloadUnitStartIndicator ? 1.0 : 0.0);
            offsets.append(packet.offset);
        }
    } else if (parameterName == "Transport Priority") {
        for (const TSPacket& packet : packets) {
            values.append(packet.transportPriority ? 1.0 : 0.0);
            offsets.append(packet.offset);
        }
    } else if (parameterName == "PCR" && m_selectedPID != 0xFFFF) {
        // Extract PCR values for selected PID
        for (const TSPacket& packet : packets) {
            if (packet.pid == m_selectedPID && packet.hasPCR) {
                values.append(packet.pcr / 90000.0); // Convert to seconds
                offsets.append(packet.offset);
            }
        }
    } else if (parameterName == "PTS" && m_selectedPID != 0xFFFF) {
        // Extract PTS values for selected PID
        for (const TSPacket& packet : packets) {
            if (packet.pid == m_selectedPID && packet.hasPTS) {
                values.append(packet.pts / 90000.0); // Convert to seconds
                offsets.append(packet.offset);
            }
        }
    } else if (parameterName == "DTS" && m_selectedPID != 0xFFFF) {
        // Extract DTS values for selected PID
        for (const TSPacket& packet : packets) {
            if (packet.pid == m_selectedPID && packet.hasDTS) {
                values.append(packet.dts / 90000.0); // Convert to seconds
                offsets.append(packet.offset);
            }
        }
    } else {
        qDebug() << "Parameter" << parameterName << "not supported for Graphics";
        return;
    }

    if (values.isEmpty()) {
        qDebug() << "No values found for parameter" << parameterName;
        return;
    }

    // Emit signal to add to Graphics Panel
    emit addToGraphics(parameterName, values, offsets);
    qDebug() << "Emitted addToGraphics signal with" << values.size() << "values";
}

void PropertyPanel::onAtomSelected(int64_t offset) {
    displayAtom(offset);
}

void PropertyPanel::onElementSelected(int64_t offset) {
    displayElement(offset);
}

void PropertyPanel::onDumpData() {
    if (m_mode != PropertyMode::Dump) {
        return;
    }

    if (m_tsParser && m_selectedPID != 0xFFFF) {
        // Dump elementary stream for selected PID
        QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save Elementary Stream"),
            QString("pid_0x%1.es").arg(m_selectedPID, 4, 16, QChar('0')),
            tr("Elementary Stream (*.es);;All Files (*)"));

        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to open file for writing"));
            return;
        }

        const auto& packets = m_tsParser->getPackets();
        int count = 0;
        for (const TSPacket& packet : packets) {
            if (packet.pid == m_selectedPID && !packet.payload.isEmpty()) {
                file.write(packet.payload);
                count++;
            }
        }

        file.close();
        QMessageBox::information(this, tr("Dump Complete"),
            tr("Dumped %1 packets to %2").arg(count).arg(fileName));
    } else if (m_mp4Parser && m_lastAtomOffset >= 0) {
        // Dump atom data
        const MP4Atom* atom = findAtomByOffset(m_mp4Parser->getAtoms(), m_lastAtomOffset);
        if (!atom) {
            return;
        }

        QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save Atom Data"),
            QString("%1_0x%2.bin").arg(atom->type).arg(atom->offset, 0, 16),
            tr("Binary Files (*.bin);;All Files (*)"));

        if (fileName.isEmpty()) {
            return;
        }

        QFile sourceFile(m_mp4Parser->getFilePath());
        if (!sourceFile.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to open source file"));
            return;
        }

        QFile destFile(fileName);
        if (!destFile.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to open destination file"));
            return;
        }

        sourceFile.seek(atom->offset);
        QByteArray data = sourceFile.read(atom->size);
        destFile.write(data);

        sourceFile.close();
        destFile.close();

        QMessageBox::information(this, tr("Dump Complete"),
            tr("Dumped %1 bytes to %2").arg(atom->size).arg(fileName));
    } else if (m_mkvParser && m_lastElementOffset >= 0) {
        // Dump element data
        const EBMLElement* element = findElementByOffset(m_mkvParser->getElements(), m_lastElementOffset);
        if (!element) {
            return;
        }

        QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save Element Data"),
            QString("%1_0x%2.bin").arg(element->name).arg(element->offset, 0, 16),
            tr("Binary Files (*.bin);;All Files (*)"));

        if (fileName.isEmpty()) {
            return;
        }

        QFile sourceFile(m_mkvParser->getFilePath());
        if (!sourceFile.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to open source file"));
            return;
        }

        QFile destFile(fileName);
        if (!destFile.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to open destination file"));
            return;
        }

        sourceFile.seek(element->offset);
        QByteArray data = sourceFile.read(element->totalSize);
        destFile.write(data);

        sourceFile.close();
        destFile.close();

        QMessageBox::information(this, tr("Dump Complete"),
            tr("Dumped %1 bytes to %2").arg(element->totalSize).arg(fileName));
    } else if (m_aviParser && m_lastChunkOffset >= 0) {
        // Dump chunk data
        const AVIChunk* chunk = findChunkByOffset(m_aviParser->getChunks(), m_lastChunkOffset);
        if (!chunk) {
            return;
        }

        QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save Chunk Data"),
            QString("%1_0x%2.bin").arg(chunk->fourCC).arg(chunk->offset, 0, 16),
            tr("Binary Files (*.bin);;All Files (*)"));

        if (fileName.isEmpty()) {
            return;
        }

        QFile sourceFile(m_aviParser->getFilePath());
        if (!sourceFile.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to open source file"));
            return;
        }

        QFile destFile(fileName);
        if (!destFile.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to open destination file"));
            return;
        }

        sourceFile.seek(chunk->offset);
        QByteArray data = sourceFile.read(chunk->totalSize);
        destFile.write(data);

        sourceFile.close();
        destFile.close();

        QMessageBox::information(this, tr("Dump Complete"),
            tr("Dumped %1 bytes to %2").arg(chunk->totalSize).arg(fileName));
    }
}

void PropertyPanel::displayAtomSync(const MP4Atom* atom) {
    m_treeWidget->clear();

    if (!atom) {
        return;
    }

    // Set 2 columns for sync mode
    m_treeWidget->setColumnCount(2);
    m_treeWidget->setHeaderLabels(QStringList() << "Property" << "Value");
    m_treeWidget->setColumnWidth(0, 200);

    // Root item: Atom info
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
    rootItem->setText(0, QString("Atom: %1").arg(atom->type));
    rootItem->setText(1, QString("Offset: 0x%1").arg(atom->offset, 0, 16));
    rootItem->setExpanded(true);

    addAtomFields(rootItem, *atom);
}

void PropertyPanel::displayAtomCompare(const MP4Atom* atom) {
    m_treeWidget->clear();

    if (!atom || !m_mp4Parser) {
        return;
    }

    // Find previous atom by offset
    const MP4Atom* prevAtom = nullptr;
    if (m_lastAtomOffset >= 0) {
        std::function<const MP4Atom*(const QVector<MP4Atom>&, int64_t)> findAtom;
        findAtom = [&](const QVector<MP4Atom>& atoms, int64_t offset) -> const MP4Atom* {
            for (const auto& a : atoms) {
                if (a.offset == offset) return &a;
                if (!a.children.isEmpty()) {
                    const MP4Atom* found = findAtom(a.children, offset);
                    if (found) return found;
                }
            }
            return nullptr;
        };
        prevAtom = findAtom(m_mp4Parser->getAtoms(), m_lastAtomOffset);
    }

    if (!prevAtom) {
        // No previous atom to compare, just display current
        displayAtomSync(atom);
        return;
    }

    // Set 3 columns for compare mode
    m_treeWidget->setColumnCount(3);
    m_treeWidget->setHeaderLabels(QStringList() << "Property" << "Previous" << "Current");
    m_treeWidget->setColumnWidth(0, 150);
    m_treeWidget->setColumnWidth(1, 200);
    m_treeWidget->setColumnWidth(2, 200);

    // Create side-by-side comparison
    QTreeWidgetItem* headerItem = new QTreeWidgetItem(m_treeWidget);
    headerItem->setText(0, "Comparison");
    headerItem->setText(1, QString("Previous (%1)").arg(prevAtom->type));
    headerItem->setText(2, QString("Current (%1)").arg(atom->type));
    headerItem->setExpanded(true);
    // Use darker background and white text for better visibility in dark theme
    headerItem->setBackground(0, QBrush(QColor(60, 60, 60)));
    headerItem->setBackground(1, QBrush(QColor(60, 60, 60)));
    headerItem->setBackground(2, QBrush(QColor(60, 60, 60)));
    headerItem->setForeground(0, QBrush(QColor(255, 255, 255)));
    headerItem->setForeground(1, QBrush(QColor(255, 255, 255)));
    headerItem->setForeground(2, QBrush(QColor(255, 255, 255)));

    // Compare fields
    addCompareRow(headerItem, "Type", prevAtom->type, atom->type);
    addCompareRow(headerItem, "Offset",
                  QString("0x%1 (%2)").arg(prevAtom->offset, 0, 16).arg(prevAtom->offset),
                  QString("0x%1 (%2)").arg(atom->offset, 0, 16).arg(atom->offset));
    addCompareRow(headerItem, "Size",
                  QString("%1 bytes").arg(prevAtom->size),
                  QString("%1 bytes").arg(atom->size));
    addCompareRow(headerItem, "Header Size",
                  QString("%1 bytes").arg(prevAtom->headerSize),
                  QString("%1 bytes").arg(atom->headerSize));
    addCompareRow(headerItem, "Data Size",
                  QString("%1 bytes").arg(prevAtom->dataSize),
                  QString("%1 bytes").arg(atom->dataSize));
    addCompareRow(headerItem, "Percentage",
                  QString("%1%").arg(prevAtom->percentage, 0, 'f', 2),
                  QString("%1%").arg(atom->percentage, 0, 'f', 2));
    addCompareRow(headerItem, "Level",
                  QString::number(prevAtom->level),
                  QString::number(atom->level));

    // Type-specific fields
    if (atom->type == "ftyp" && prevAtom->type == "ftyp") {
        addCompareRow(headerItem, "Major Brand", prevAtom->majorBrand, atom->majorBrand);
        addCompareRow(headerItem, "Compatible Brands",
                      prevAtom->compatibleBrands.join(", "),
                      atom->compatibleBrands.join(", "));
    } else if (atom->type == "tkhd" && prevAtom->type == "tkhd") {
        addCompareRow(headerItem, "Track ID",
                      QString::number(prevAtom->trackId),
                      QString::number(atom->trackId));
        if (atom->width > 0 && atom->height > 0) {
            addCompareRow(headerItem, "Width",
                          QString::number(prevAtom->width),
                          QString::number(atom->width));
            addCompareRow(headerItem, "Height",
                          QString::number(prevAtom->height),
                          QString::number(atom->height));
        }
    } else if (atom->type == "hdlr" && prevAtom->type == "hdlr") {
        addCompareRow(headerItem, "Handler Type", prevAtom->handlerType, atom->handlerType);
    } else if (atom->type == "stsd" && prevAtom->type == "stsd") {
        addCompareRow(headerItem, "Codec", prevAtom->codecType, atom->codecType);
        if (atom->width > 0 && atom->height > 0) {
            addCompareRow(headerItem, "Width",
                          QString::number(prevAtom->width),
                          QString::number(atom->width));
            addCompareRow(headerItem, "Height",
                          QString::number(prevAtom->height),
                          QString::number(atom->height));
        }
    }
}

void PropertyPanel::addCompareRow(QTreeWidgetItem* parent, const QString& property, const QString& prevValue, const QString& currValue) {
    QTreeWidgetItem* item = new QTreeWidgetItem(parent);
    item->setText(0, property);
    item->setText(1, prevValue);
    item->setText(2, currValue);

    // Highlight differences in blue
    if (prevValue != currValue) {
        item->setForeground(1, QBrush(QColor(0, 0, 255)));
        item->setForeground(2, QBrush(QColor(0, 0, 255)));
    }
}

void PropertyPanel::addAtomFields(QTreeWidgetItem* parent, const MP4Atom& atom) {
    // Type
    QTreeWidgetItem* typeItem = new QTreeWidgetItem(parent);
    typeItem->setText(0, "Type");
    typeItem->setText(1, atom.type);

    // Offset
    QTreeWidgetItem* offsetItem = new QTreeWidgetItem(parent);
    offsetItem->setText(0, "Offset");
    offsetItem->setText(1, QString("0x%1 (%2)").arg(atom.offset, 0, 16).arg(atom.offset));

    // Size
    QTreeWidgetItem* sizeItem = new QTreeWidgetItem(parent);
    sizeItem->setText(0, "Size");
    sizeItem->setText(1, QString("%1 bytes").arg(atom.size));

    // Header Size
    QTreeWidgetItem* headerSizeItem = new QTreeWidgetItem(parent);
    headerSizeItem->setText(0, "Header Size");
    headerSizeItem->setText(1, QString("%1 bytes").arg(atom.headerSize));

    // Data Size
    QTreeWidgetItem* dataSizeItem = new QTreeWidgetItem(parent);
    dataSizeItem->setText(0, "Data Size");
    dataSizeItem->setText(1, QString("%1 bytes").arg(atom.dataSize));

    // Percentage
    QTreeWidgetItem* percentageItem = new QTreeWidgetItem(parent);
    percentageItem->setText(0, "Percentage");
    percentageItem->setText(1, QString("%1%").arg(atom.percentage, 0, 'f', 2));

    // Level
    QTreeWidgetItem* levelItem = new QTreeWidgetItem(parent);
    levelItem->setText(0, "Level");
    levelItem->setText(1, QString::number(atom.level));

    // Type-specific fields
    if (atom.type == "ftyp" && !atom.majorBrand.isEmpty()) {
        QTreeWidgetItem* brandItem = new QTreeWidgetItem(parent);
        brandItem->setText(0, "Major Brand");
        brandItem->setText(1, atom.majorBrand);

        if (!atom.compatibleBrands.isEmpty()) {
            QTreeWidgetItem* compatItem = new QTreeWidgetItem(parent);
            compatItem->setText(0, "Compatible Brands");
            compatItem->setText(1, atom.compatibleBrands.join(", "));
        }
    } else if (atom.type == "tkhd" && atom.trackId > 0) {
        QTreeWidgetItem* trackIdItem = new QTreeWidgetItem(parent);
        trackIdItem->setText(0, "Track ID");
        trackIdItem->setText(1, QString::number(atom.trackId));

        if (atom.width > 0 && atom.height > 0) {
            QTreeWidgetItem* widthItem = new QTreeWidgetItem(parent);
            widthItem->setText(0, "Width");
            widthItem->setText(1, QString::number(atom.width));

            QTreeWidgetItem* heightItem = new QTreeWidgetItem(parent);
            heightItem->setText(0, "Height");
            heightItem->setText(1, QString::number(atom.height));
        }
    } else if (atom.type == "hdlr" && !atom.handlerType.isEmpty()) {
        QTreeWidgetItem* handlerItem = new QTreeWidgetItem(parent);
        handlerItem->setText(0, "Handler Type");
        handlerItem->setText(1, atom.handlerType);
    } else if (atom.type == "stsd" && !atom.codecType.isEmpty()) {
        QTreeWidgetItem* codecItem = new QTreeWidgetItem(parent);
        codecItem->setText(0, "Codec");
        codecItem->setText(1, atom.codecType);

        if (atom.width > 0 && atom.height > 0) {
            QTreeWidgetItem* widthItem = new QTreeWidgetItem(parent);
            widthItem->setText(0, "Width");
            widthItem->setText(1, QString::number(atom.width));

            QTreeWidgetItem* heightItem = new QTreeWidgetItem(parent);
            heightItem->setText(0, "Height");
            heightItem->setText(1, QString::number(atom.height));
        }

        if (atom.sampleRate > 0) {
            QTreeWidgetItem* sampleRateItem = new QTreeWidgetItem(parent);
            sampleRateItem->setText(0, "Sample Rate");
            sampleRateItem->setText(1, QString("%1 Hz").arg(atom.sampleRate));
        }

        if (atom.channelCount > 0) {
            QTreeWidgetItem* channelsItem = new QTreeWidgetItem(parent);
            channelsItem->setText(0, "Channels");
            channelsItem->setText(1, QString::number(atom.channelCount));
        }
    }

    // Children count
    if (!atom.children.isEmpty()) {
        QTreeWidgetItem* childrenItem = new QTreeWidgetItem(parent);
        childrenItem->setText(0, "Children");
        childrenItem->setText(1, QString::number(atom.children.size()));
    }
}

const MP4Atom* PropertyPanel::findAtomByOffset(const QVector<MP4Atom>& atoms, int64_t offset) {
    for (const MP4Atom& atom : atoms) {
        if (atom.offset == offset) {
            return &atom;
        }
        if (!atom.children.isEmpty()) {
            const MP4Atom* found = findAtomByOffset(atom.children, offset);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

void PropertyPanel::displayElement(int64_t offset) {
    if (!m_mkvParser || offset < 0) {
        return;
    }

    switch (m_mode) {
    case PropertyMode::Sync:
        {
            const EBMLElement* element = findElementByOffset(m_mkvParser->getElements(), offset);
            displayElementSync(element);
            m_lastElementOffset = offset;
        }
        break;
    case PropertyMode::Compare:
        {
            const EBMLElement* element = findElementByOffset(m_mkvParser->getElements(), offset);
            displayElementCompare(element);
            // Update last offset AFTER comparison so next click compares against this one
            m_lastElementOffset = offset;
        }
        break;
    case PropertyMode::Dump:
        // Dump mode is handled by onDumpData()
        m_lastElementOffset = offset;
        break;
    }
}

void PropertyPanel::displayElementSync(const EBMLElement* element) {
    m_treeWidget->clear();

    if (!element) {
        return;
    }

    // Set 2 columns for sync mode
    m_treeWidget->setColumnCount(2);
    m_treeWidget->setHeaderLabels(QStringList() << "Property" << "Value");
    m_treeWidget->setColumnWidth(0, 200);

    // Root item: Element info
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
    rootItem->setText(0, QString("Element: %1").arg(element->name));
    rootItem->setText(1, QString("Offset: 0x%1").arg(element->offset, 0, 16));
    rootItem->setExpanded(true);

    addElementFields(rootItem, *element);
}

void PropertyPanel::displayElementCompare(const EBMLElement* element) {
    m_treeWidget->clear();

    if (!element || !m_mkvParser) {
        return;
    }

    // Find previous element by offset
    const EBMLElement* prevElement = nullptr;
    if (m_lastElementOffset >= 0) {
        std::function<const EBMLElement*(const QVector<EBMLElement>&, int64_t)> findElement;
        findElement = [&](const QVector<EBMLElement>& elements, int64_t offset) -> const EBMLElement* {
            for (const auto& e : elements) {
                if (e.offset == offset) return &e;
                if (!e.children.isEmpty()) {
                    const EBMLElement* found = findElement(e.children, offset);
                    if (found) return found;
                }
            }
            return nullptr;
        };
        prevElement = findElement(m_mkvParser->getElements(), m_lastElementOffset);
    }

    if (!prevElement) {
        // No previous element to compare, just display current
        displayElementSync(element);
        return;
    }

    // Set 3 columns for compare mode
    m_treeWidget->setColumnCount(3);
    m_treeWidget->setHeaderLabels(QStringList() << "Property" << "Previous" << "Current");
    m_treeWidget->setColumnWidth(0, 150);
    m_treeWidget->setColumnWidth(1, 200);
    m_treeWidget->setColumnWidth(2, 200);

    // Create side-by-side comparison
    QTreeWidgetItem* headerItem = new QTreeWidgetItem(m_treeWidget);
    headerItem->setText(0, "Comparison");
    headerItem->setText(1, QString("Previous (%1)").arg(prevElement->name));
    headerItem->setText(2, QString("Current (%1)").arg(element->name));
    headerItem->setExpanded(true);
    // Use darker background and white text for better visibility in dark theme
    headerItem->setBackground(0, QBrush(QColor(60, 60, 60)));
    headerItem->setBackground(1, QBrush(QColor(60, 60, 60)));
    headerItem->setBackground(2, QBrush(QColor(60, 60, 60)));
    headerItem->setForeground(0, QBrush(QColor(255, 255, 255)));
    headerItem->setForeground(1, QBrush(QColor(255, 255, 255)));
    headerItem->setForeground(2, QBrush(QColor(255, 255, 255)));

    // Compare fields
    addCompareRow(headerItem, "Element ID",
                  QString("0x%1").arg(static_cast<uint32_t>(prevElement->id), 0, 16),
                  QString("0x%1").arg(static_cast<uint32_t>(element->id), 0, 16));
    addCompareRow(headerItem, "Offset",
                  QString("0x%1 (%2)").arg(prevElement->offset, 0, 16).arg(prevElement->offset),
                  QString("0x%1 (%2)").arg(element->offset, 0, 16).arg(element->offset));
    addCompareRow(headerItem, "Header Size",
                  QString("%1 bytes").arg(prevElement->headerSize),
                  QString("%1 bytes").arg(element->headerSize));
    addCompareRow(headerItem, "Data Size",
                  QString("%1 bytes").arg(prevElement->dataSize),
                  QString("%1 bytes").arg(element->dataSize));
    addCompareRow(headerItem, "Total Size",
                  QString("%1 bytes").arg(prevElement->totalSize),
                  QString("%1 bytes").arg(element->totalSize));
    addCompareRow(headerItem, "Percentage",
                  QString("%1%").arg(prevElement->percentage, 0, 'f', 2),
                  QString("%1%").arg(element->percentage, 0, 'f', 2));
    addCompareRow(headerItem, "Level",
                  QString::number(prevElement->level),
                  QString::number(element->level));

    // Element-specific data
    if (!element->stringValue.isEmpty() || !prevElement->stringValue.isEmpty()) {
        addCompareRow(headerItem, "String Value", prevElement->stringValue, element->stringValue);
    }

    if (element->uintValue > 0 || prevElement->uintValue > 0) {
        addCompareRow(headerItem, "UInt Value",
                      QString::number(prevElement->uintValue),
                      QString::number(element->uintValue));
    }

    if (element->floatValue > 0 || prevElement->floatValue > 0) {
        addCompareRow(headerItem, "Float Value",
                      QString::number(prevElement->floatValue, 'f', 6),
                      QString::number(element->floatValue, 'f', 6));
    }

    if (!element->binaryValue.isEmpty() || !prevElement->binaryValue.isEmpty()) {
        addCompareRow(headerItem, "Binary Value",
                      QString("%1 bytes").arg(prevElement->binaryValue.size()),
                      QString("%1 bytes").arg(element->binaryValue.size()));
    }

    // Track-specific fields
    if (element->trackNumber > 0 || prevElement->trackNumber > 0) {
        addCompareRow(headerItem, "Track Number",
                      QString::number(prevElement->trackNumber),
                      QString::number(element->trackNumber));
    }

    if (element->trackUID > 0 || prevElement->trackUID > 0) {
        addCompareRow(headerItem, "Track UID",
                      QString::number(prevElement->trackUID),
                      QString::number(element->trackUID));
    }
}

void PropertyPanel::addElementFields(QTreeWidgetItem* parent, const EBMLElement& element) {
    // Element ID
    QTreeWidgetItem* idItem = new QTreeWidgetItem(parent);
    idItem->setText(0, "Element ID");
    idItem->setText(1, QString("0x%1").arg(static_cast<uint32_t>(element.id), 0, 16));

    // Offset
    QTreeWidgetItem* offsetItem = new QTreeWidgetItem(parent);
    offsetItem->setText(0, "Offset");
    offsetItem->setText(1, QString("0x%1 (%2)").arg(element.offset, 0, 16).arg(element.offset));

    // Header Size
    QTreeWidgetItem* headerSizeItem = new QTreeWidgetItem(parent);
    headerSizeItem->setText(0, "Header Size");
    headerSizeItem->setText(1, QString("%1 bytes").arg(element.headerSize));

    // Data Size
    QTreeWidgetItem* dataSizeItem = new QTreeWidgetItem(parent);
    dataSizeItem->setText(0, "Data Size");
    dataSizeItem->setText(1, QString("%1 bytes").arg(element.dataSize));

    // Total Size
    QTreeWidgetItem* totalSizeItem = new QTreeWidgetItem(parent);
    totalSizeItem->setText(0, "Total Size");
    totalSizeItem->setText(1, QString("%1 bytes").arg(element.totalSize));

    // Percentage
    QTreeWidgetItem* percentageItem = new QTreeWidgetItem(parent);
    percentageItem->setText(0, "Percentage");
    percentageItem->setText(1, QString("%1%").arg(element.percentage, 0, 'f', 2));

    // Level
    QTreeWidgetItem* levelItem = new QTreeWidgetItem(parent);
    levelItem->setText(0, "Level");
    levelItem->setText(1, QString::number(element.level));

    // Element-specific data
    if (!element.stringValue.isEmpty()) {
        QTreeWidgetItem* stringItem = new QTreeWidgetItem(parent);
        stringItem->setText(0, "String Value");
        stringItem->setText(1, element.stringValue);
    }

    if (element.uintValue > 0) {
        QTreeWidgetItem* uintItem = new QTreeWidgetItem(parent);
        uintItem->setText(0, "UInt Value");
        uintItem->setText(1, QString::number(element.uintValue));
    }

    if (element.floatValue > 0) {
        QTreeWidgetItem* floatItem = new QTreeWidgetItem(parent);
        floatItem->setText(0, "Float Value");
        floatItem->setText(1, QString::number(element.floatValue, 'f', 6));
    }

    if (!element.binaryValue.isEmpty()) {
        QTreeWidgetItem* binaryItem = new QTreeWidgetItem(parent);
        binaryItem->setText(0, "Binary Value");
        binaryItem->setText(1, QString("%1 bytes").arg(element.binaryValue.size()));
    }

    // Track-specific fields
    if (element.trackNumber > 0) {
        QTreeWidgetItem* trackNumItem = new QTreeWidgetItem(parent);
        trackNumItem->setText(0, "Track Number");
        trackNumItem->setText(1, QString::number(element.trackNumber));
    }

    if (element.trackUID > 0) {
        QTreeWidgetItem* trackUIDItem = new QTreeWidgetItem(parent);
        trackUIDItem->setText(0, "Track UID");
        trackUIDItem->setText(1, QString::number(element.trackUID));
    }

    if (element.trackType > 0) {
        QTreeWidgetItem* trackTypeItem = new QTreeWidgetItem(parent);
        trackTypeItem->setText(0, "Track Type");
        QString trackTypeStr;
        switch (element.trackType) {
        case 1: trackTypeStr = "Video"; break;
        case 2: trackTypeStr = "Audio"; break;
        case 3: trackTypeStr = "Complex"; break;
        case 0x10: trackTypeStr = "Logo"; break;
        case 0x11: trackTypeStr = "Subtitle"; break;
        case 0x12: trackTypeStr = "Buttons"; break;
        case 0x20: trackTypeStr = "Control"; break;
        default: trackTypeStr = QString("Unknown (%1)").arg(element.trackType); break;
        }
        trackTypeItem->setText(1, trackTypeStr);
    }

    if (!element.codecID.isEmpty()) {
        QTreeWidgetItem* codecIDItem = new QTreeWidgetItem(parent);
        codecIDItem->setText(0, "Codec ID");
        codecIDItem->setText(1, element.codecID);
    }

    if (!element.codecName.isEmpty()) {
        QTreeWidgetItem* codecNameItem = new QTreeWidgetItem(parent);
        codecNameItem->setText(0, "Codec Name");
        codecNameItem->setText(1, element.codecName);
    }

    if (element.pixelWidth > 0) {
        QTreeWidgetItem* widthItem = new QTreeWidgetItem(parent);
        widthItem->setText(0, "Pixel Width");
        widthItem->setText(1, QString::number(element.pixelWidth));
    }

    if (element.pixelHeight > 0) {
        QTreeWidgetItem* heightItem = new QTreeWidgetItem(parent);
        heightItem->setText(0, "Pixel Height");
        heightItem->setText(1, QString::number(element.pixelHeight));
    }

    if (element.samplingFrequency > 0) {
        QTreeWidgetItem* sampleRateItem = new QTreeWidgetItem(parent);
        sampleRateItem->setText(0, "Sampling Frequency");
        sampleRateItem->setText(1, QString("%1 Hz").arg(element.samplingFrequency, 0, 'f', 0));
    }

    if (element.channels > 0) {
        QTreeWidgetItem* channelsItem = new QTreeWidgetItem(parent);
        channelsItem->setText(0, "Channels");
        channelsItem->setText(1, QString::number(element.channels));
    }

    // Children count
    if (!element.children.isEmpty()) {
        QTreeWidgetItem* childrenItem = new QTreeWidgetItem(parent);
        childrenItem->setText(0, "Children");
        childrenItem->setText(1, QString::number(element.children.size()));
    }
}

const EBMLElement* PropertyPanel::findElementByOffset(const QVector<EBMLElement>& elements, int64_t offset) {
    for (const EBMLElement& element : elements) {
        if (element.offset == offset) {
            return &element;
        }
        if (!element.children.isEmpty()) {
            const EBMLElement* found = findElementByOffset(element.children, offset);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

} // namespace VideoStudio
// AVI-specific methods for PropertyPanel
// This file contains the implementation of AVI chunk display methods

#include "panels/propertypanel.h"
#include <QDebug>

namespace VideoStudio {

void PropertyPanel::displayChunk(int64_t offset) {
    if (!m_aviParser) {
        return;
    }

    const AVIChunk* chunk = findChunkByOffset(m_aviParser->getChunks(), offset);
    if (!chunk) {
        qDebug() << "Chunk not found at offset:" << QString("0x%1").arg(offset, 0, 16);
        return;
    }

    switch (m_mode) {
    case PropertyMode::Sync:
        displayChunkSync(chunk);
        m_lastChunkOffset = offset;
        break;
    case PropertyMode::Compare:
        displayChunkCompare(chunk);
        // Update last offset AFTER comparison so next click compares against this one
        m_lastChunkOffset = offset;
        break;
    case PropertyMode::Dump:
        // Dump mode is handled by onDumpData()
        displayChunkSync(chunk);
        m_lastChunkOffset = offset;
        break;
    }
}

void PropertyPanel::onChunkSelected(int64_t offset) {
    displayChunk(offset);
}

void PropertyPanel::displayChunkSync(const AVIChunk* chunk) {
    m_treeWidget->clear();

    if (!chunk) {
        return;
    }

    // Set 2 columns for sync mode
    m_treeWidget->setColumnCount(2);
    m_treeWidget->setHeaderLabels(QStringList() << "Property" << "Value");
    m_treeWidget->setColumnWidth(0, 200);

    // Root item: Chunk info
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
    rootItem->setText(0, QString("Chunk: %1").arg(chunk->fourCC));
    rootItem->setText(1, QString("Offset: 0x%1").arg(chunk->offset, 0, 16));
    rootItem->setExpanded(true);

    addChunkFields(rootItem, *chunk);
}

void PropertyPanel::displayChunkCompare(const AVIChunk* chunk) {
    m_treeWidget->clear();

    if (!chunk || !m_aviParser) {
        return;
    }

    // Find previous chunk by offset
    const AVIChunk* prevChunk = nullptr;
    if (m_lastChunkOffset >= 0) {
        std::function<const AVIChunk*(const QVector<AVIChunk>&, int64_t)> findChunk;
        findChunk = [&](const QVector<AVIChunk>& chunks, int64_t offset) -> const AVIChunk* {
            for (const auto& c : chunks) {
                if (c.offset == offset) return &c;
                if (!c.children.isEmpty()) {
                    const AVIChunk* found = findChunk(c.children, offset);
                    if (found) return found;
                }
            }
            return nullptr;
        };
        prevChunk = findChunk(m_aviParser->getChunks(), m_lastChunkOffset);
    }

    if (!prevChunk) {
        // No previous chunk to compare, just display current
        displayChunkSync(chunk);
        return;
    }

    // Set 3 columns for compare mode
    m_treeWidget->setColumnCount(3);
    m_treeWidget->setHeaderLabels(QStringList() << "Property" << "Previous" << "Current");
    m_treeWidget->setColumnWidth(0, 150);
    m_treeWidget->setColumnWidth(1, 200);
    m_treeWidget->setColumnWidth(2, 200);

    // Create side-by-side comparison
    QTreeWidgetItem* headerItem = new QTreeWidgetItem(m_treeWidget);
    headerItem->setText(0, "Comparison");
    headerItem->setText(1, QString("Previous (%1)").arg(prevChunk->fourCC));
    headerItem->setText(2, QString("Current (%1)").arg(chunk->fourCC));
    headerItem->setExpanded(true);
    // Use darker background and white text for better visibility in dark theme
    headerItem->setBackground(0, QBrush(QColor(60, 60, 60)));
    headerItem->setBackground(1, QBrush(QColor(60, 60, 60)));
    headerItem->setBackground(2, QBrush(QColor(60, 60, 60)));
    headerItem->setForeground(0, QBrush(QColor(255, 255, 255)));
    headerItem->setForeground(1, QBrush(QColor(255, 255, 255)));
    headerItem->setForeground(2, QBrush(QColor(255, 255, 255)));

    // Compare fields
    addCompareRow(headerItem, "FourCC", prevChunk->fourCC, chunk->fourCC);
    addCompareRow(headerItem, "Offset",
                  QString("0x%1").arg(prevChunk->offset, 0, 16),
                  QString("0x%1").arg(chunk->offset, 0, 16));
    addCompareRow(headerItem, "Size",
                  QString("%1 bytes").arg(prevChunk->size),
                  QString("%1 bytes").arg(chunk->size));
    addCompareRow(headerItem, "Total Size",
                  QString("%1 bytes").arg(prevChunk->totalSize),
                  QString("%1 bytes").arg(chunk->totalSize));
    addCompareRow(headerItem, "Percentage",
                  QString("%1%").arg(prevChunk->percentage, 0, 'f', 2),
                  QString("%1%").arg(chunk->percentage, 0, 'f', 2));
    addCompareRow(headerItem, "Level",
                  QString::number(prevChunk->level),
                  QString::number(chunk->level));

    // List Type (for RIFF/LIST chunks)
    if (!chunk->listType.isEmpty() || !prevChunk->listType.isEmpty()) {
        addCompareRow(headerItem, "List Type", prevChunk->listType, chunk->listType);
    }

    // Main AVI Header (avih) fields
    if (chunk->fourCC == "avih" && prevChunk->fourCC == "avih") {
        if (chunk->microSecPerFrame > 0 || prevChunk->microSecPerFrame > 0) {
            addCompareRow(headerItem, "MicroSec Per Frame",
                          QString::number(prevChunk->microSecPerFrame),
                          QString::number(chunk->microSecPerFrame));

            // Calculate FPS
            double prevFps = prevChunk->microSecPerFrame > 0 ? 1000000.0 / prevChunk->microSecPerFrame : 0;
            double currFps = chunk->microSecPerFrame > 0 ? 1000000.0 / chunk->microSecPerFrame : 0;
            addCompareRow(headerItem, "Frame Rate",
                          QString("%1 fps").arg(prevFps, 0, 'f', 2),
                          QString("%1 fps").arg(currFps, 0, 'f', 2));
        }

        if (chunk->maxBytesPerSec > 0 || prevChunk->maxBytesPerSec > 0) {
            addCompareRow(headerItem, "Max Bytes Per Sec",
                          QString::number(prevChunk->maxBytesPerSec),
                          QString::number(chunk->maxBytesPerSec));
        }

        if (chunk->totalFrames > 0 || prevChunk->totalFrames > 0) {
            addCompareRow(headerItem, "Total Frames",
                          QString::number(prevChunk->totalFrames),
                          QString::number(chunk->totalFrames));
        }

        if (chunk->streams > 0 || prevChunk->streams > 0) {
            addCompareRow(headerItem, "Streams",
                          QString::number(prevChunk->streams),
                          QString::number(chunk->streams));
        }

        if (chunk->width > 0 || prevChunk->width > 0) {
            addCompareRow(headerItem, "Width",
                          QString::number(prevChunk->width),
                          QString::number(chunk->width));
        }

        if (chunk->height > 0 || prevChunk->height > 0) {
            addCompareRow(headerItem, "Height",
                          QString::number(prevChunk->height),
                          QString::number(chunk->height));
        }
    }
}

void PropertyPanel::addChunkFields(QTreeWidgetItem* parent, const AVIChunk& chunk) {
    // FourCC
    QTreeWidgetItem* fourCCItem = new QTreeWidgetItem(parent);
    fourCCItem->setText(0, "FourCC");
    fourCCItem->setText(1, chunk.fourCC);

    // Offset
    QTreeWidgetItem* offsetItem = new QTreeWidgetItem(parent);
    offsetItem->setText(0, "Offset");
    offsetItem->setText(1, QString("0x%1").arg(chunk.offset, 0, 16));

    // Size
    QTreeWidgetItem* sizeItem = new QTreeWidgetItem(parent);
    sizeItem->setText(0, "Size");
    sizeItem->setText(1, QString("%1 bytes").arg(chunk.size));

    // Total Size
    QTreeWidgetItem* totalSizeItem = new QTreeWidgetItem(parent);
    totalSizeItem->setText(0, "Total Size");
    totalSizeItem->setText(1, QString("%1 bytes").arg(chunk.totalSize));

    // Percentage
    QTreeWidgetItem* percentageItem = new QTreeWidgetItem(parent);
    percentageItem->setText(0, "Percentage");
    percentageItem->setText(1, QString("%1%").arg(chunk.percentage, 0, 'f', 2));

    // Level
    QTreeWidgetItem* levelItem = new QTreeWidgetItem(parent);
    levelItem->setText(0, "Level");
    levelItem->setText(1, QString::number(chunk.level));

    // List Type (for RIFF/LIST chunks)
    if (!chunk.listType.isEmpty()) {
        QTreeWidgetItem* listTypeItem = new QTreeWidgetItem(parent);
        listTypeItem->setText(0, "List Type");
        listTypeItem->setText(1, chunk.listType);
    }

    // Main AVI Header (avih) fields
    if (chunk.fourCC == "avih") {
        if (chunk.microSecPerFrame > 0) {
            QTreeWidgetItem* microSecItem = new QTreeWidgetItem(parent);
            microSecItem->setText(0, "MicroSec Per Frame");
            microSecItem->setText(1, QString::number(chunk.microSecPerFrame));

            // Calculate FPS
            double fps = 1000000.0 / chunk.microSecPerFrame;
            QTreeWidgetItem* fpsItem = new QTreeWidgetItem(parent);
            fpsItem->setText(0, "Frame Rate");
            fpsItem->setText(1, QString("%1 fps").arg(fps, 0, 'f', 2));
        }

        if (chunk.maxBytesPerSec > 0) {
            QTreeWidgetItem* maxBytesItem = new QTreeWidgetItem(parent);
            maxBytesItem->setText(0, "Max Bytes Per Sec");
            maxBytesItem->setText(1, QString::number(chunk.maxBytesPerSec));
        }

        if (chunk.totalFrames > 0) {
            QTreeWidgetItem* framesItem = new QTreeWidgetItem(parent);
            framesItem->setText(0, "Total Frames");
            framesItem->setText(1, QString::number(chunk.totalFrames));
        }

        if (chunk.streams > 0) {
            QTreeWidgetItem* streamsItem = new QTreeWidgetItem(parent);
            streamsItem->setText(0, "Streams");
            streamsItem->setText(1, QString::number(chunk.streams));
        }

        if (chunk.width > 0 && chunk.height > 0) {
            QTreeWidgetItem* resolutionItem = new QTreeWidgetItem(parent);
            resolutionItem->setText(0, "Resolution");
            resolutionItem->setText(1, QString("%1 x %2").arg(chunk.width).arg(chunk.height));
        }
    }

    // Stream Header (strh) fields
    if (chunk.fourCC == "strh") {
        if (!chunk.streamType.isEmpty()) {
            QTreeWidgetItem* streamTypeItem = new QTreeWidgetItem(parent);
            streamTypeItem->setText(0, "Stream Type");
            streamTypeItem->setText(1, chunk.streamType);
        }

        if (!chunk.codecFourCC.isEmpty()) {
            QTreeWidgetItem* codecItem = new QTreeWidgetItem(parent);
            codecItem->setText(0, "Codec");
            codecItem->setText(1, chunk.codecFourCC);
        }

        if (chunk.scale > 0 && chunk.rate > 0) {
            QTreeWidgetItem* rateItem = new QTreeWidgetItem(parent);
            rateItem->setText(0, "Rate");
            rateItem->setText(1, QString("%1 / %2").arg(chunk.rate).arg(chunk.scale));

            // Calculate frame rate for video streams
            if (chunk.streamType == "vids") {
                double fps = static_cast<double>(chunk.rate) / chunk.scale;
                QTreeWidgetItem* fpsItem = new QTreeWidgetItem(parent);
                fpsItem->setText(0, "Frame Rate");
                fpsItem->setText(1, QString("%1 fps").arg(fps, 0, 'f', 2));
            }
        }

        if (chunk.length > 0) {
            QTreeWidgetItem* lengthItem = new QTreeWidgetItem(parent);
            lengthItem->setText(0, "Length");
            lengthItem->setText(1, QString::number(chunk.length));
        }

        if (chunk.sampleSize > 0) {
            QTreeWidgetItem* sampleSizeItem = new QTreeWidgetItem(parent);
            sampleSizeItem->setText(0, "Sample Size");
            sampleSizeItem->setText(1, QString::number(chunk.sampleSize));
        }
    }

    // Children count
    if (!chunk.children.isEmpty()) {
        QTreeWidgetItem* childrenItem = new QTreeWidgetItem(parent);
        childrenItem->setText(0, "Children");
        childrenItem->setText(1, QString::number(chunk.children.size()));
    }
}

const AVIChunk* PropertyPanel::findChunkByOffset(const QVector<AVIChunk>& chunks, int64_t offset) {
    for (const AVIChunk& chunk : chunks) {
        if (chunk.offset == offset) {
            return &chunk;
        }
        if (!chunk.children.isEmpty()) {
            const AVIChunk* found = findChunkByOffset(chunk.children, offset);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

void PropertyPanel::displayTag(int64_t offset) {
    if (!m_flvParser) {
        return;
    }

    const FLVTag* tag = findTagByOffset(m_flvParser->getTags(), offset);
    if (!tag) {
        return;
    }

    if (m_mode == PropertyMode::Sync) {
        displayTagSync(tag);
        m_lastTagOffset = offset;
    } else if (m_mode == PropertyMode::Compare) {
        displayTagCompare(tag);
        // Update last offset AFTER comparison so next click compares against this one
        m_lastTagOffset = offset;
    }
}

void PropertyPanel::onTagSelected(int64_t offset) {
    displayTag(offset);
}

void PropertyPanel::displayTagSync(const FLVTag* tag) {
    m_treeWidget->clear();

    if (!tag) {
        return;
    }

    // Set 2 columns for sync mode
    m_treeWidget->setColumnCount(2);
    m_treeWidget->setHeaderLabels(QStringList() << "Property" << "Value");
    m_treeWidget->setColumnWidth(0, 200);

    // Root item: Tag info
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
    rootItem->setText(0, QString("FLV Tag: %1").arg(FLVParser::tagTypeToString(tag->type)));
    rootItem->setText(1, QString("Offset: 0x%1").arg(tag->offset, 0, 16));
    rootItem->setExpanded(true);

    addTagFields(rootItem, *tag);
}

void PropertyPanel::displayTagCompare(const FLVTag* tag) {
    m_treeWidget->clear();

    if (!tag || !m_flvParser) {
        return;
    }

    // Find previous tag by offset
    const FLVTag* prevTag = nullptr;
    if (m_lastTagOffset >= 0) {
        const auto& tags = m_flvParser->getTags();
        for (const auto& t : tags) {
            if (t.offset == m_lastTagOffset) {
                prevTag = &t;
                break;
            }
        }
    }

    if (!prevTag) {
        // No previous tag to compare, just display current
        displayTagSync(tag);
        return;
    }

    // Set 3 columns for compare mode
    m_treeWidget->setColumnCount(3);
    m_treeWidget->setHeaderLabels(QStringList() << "Property" << "Previous" << "Current");
    m_treeWidget->setColumnWidth(0, 150);
    m_treeWidget->setColumnWidth(1, 200);
    m_treeWidget->setColumnWidth(2, 200);

    // Create side-by-side comparison
    QTreeWidgetItem* headerItem = new QTreeWidgetItem(m_treeWidget);
    headerItem->setText(0, "Comparison");
    headerItem->setText(1, QString("Previous (%1)").arg(FLVParser::tagTypeToString(prevTag->type)));
    headerItem->setText(2, QString("Current (%1)").arg(FLVParser::tagTypeToString(tag->type)));
    headerItem->setExpanded(true);
    // Use darker background and white text for better visibility in dark theme
    headerItem->setBackground(0, QBrush(QColor(60, 60, 60)));
    headerItem->setBackground(1, QBrush(QColor(60, 60, 60)));
    headerItem->setBackground(2, QBrush(QColor(60, 60, 60)));
    headerItem->setForeground(0, QBrush(QColor(255, 255, 255)));
    headerItem->setForeground(1, QBrush(QColor(255, 255, 255)));
    headerItem->setForeground(2, QBrush(QColor(255, 255, 255)));

    // Compare fields
    addCompareRow(headerItem, "Tag Type",
                  FLVParser::tagTypeToString(prevTag->type),
                  FLVParser::tagTypeToString(tag->type));
    addCompareRow(headerItem, "Offset",
                  QString("0x%1").arg(prevTag->offset, 0, 16),
                  QString("0x%1").arg(tag->offset, 0, 16));
    addCompareRow(headerItem, "Data Size",
                  QString("%1 bytes").arg(prevTag->dataSize),
                  QString("%1 bytes").arg(tag->dataSize));
    addCompareRow(headerItem, "Total Size",
                  QString("%1 bytes").arg(prevTag->totalSize),
                  QString("%1 bytes").arg(tag->totalSize));
    addCompareRow(headerItem, "Percentage",
                  QString("%1%").arg(prevTag->percentage, 0, 'f', 2),
                  QString("%1%").arg(tag->percentage, 0, 'f', 2));
    addCompareRow(headerItem, "Timestamp",
                  QString("%1 ms").arg(prevTag->timestamp),
                  QString("%1 ms").arg(tag->timestamp));
    addCompareRow(headerItem, "Stream ID",
                  QString::number(prevTag->streamID),
                  QString::number(tag->streamID));

    // Video-specific fields
    if (tag->type == FLVTagType::Video && prevTag->type == FLVTagType::Video) {
        addCompareRow(headerItem, "Frame Type",
                      FLVParser::frameTypeToString(prevTag->frameType),
                      FLVParser::frameTypeToString(tag->frameType));
        addCompareRow(headerItem, "Video Codec",
                      FLVParser::videoCodecToString(prevTag->videoCodec),
                      FLVParser::videoCodecToString(tag->videoCodec));

        if ((tag->videoCodec == FLVVideoCodec::AVC || tag->videoCodec == FLVVideoCodec::HEVC) &&
            (prevTag->videoCodec == FLVVideoCodec::AVC || prevTag->videoCodec == FLVVideoCodec::HEVC)) {
            QString prevAvcType, currAvcType;
            switch (prevTag->avcPacketType) {
                case 0: prevAvcType = "0 (Sequence Header)"; break;
                case 1: prevAvcType = "1 (NALU)"; break;
                case 2: prevAvcType = "2 (End of Sequence)"; break;
                default: prevAvcType = QString::number(prevTag->avcPacketType); break;
            }
            switch (tag->avcPacketType) {
                case 0: currAvcType = "0 (Sequence Header)"; break;
                case 1: currAvcType = "1 (NALU)"; break;
                case 2: currAvcType = "2 (End of Sequence)"; break;
                default: currAvcType = QString::number(tag->avcPacketType); break;
            }
            addCompareRow(headerItem, "AVC Packet Type", prevAvcType, currAvcType);
            addCompareRow(headerItem, "Composition Time",
                          QString("%1 ms").arg(prevTag->compositionTime),
                          QString("%1 ms").arg(tag->compositionTime));
        }
    }

    // Audio-specific fields
    if (tag->type == FLVTagType::Audio && prevTag->type == FLVTagType::Audio) {
        addCompareRow(headerItem, "Audio Format",
                      FLVParser::audioFormatToString(prevTag->audioFormat),
                      FLVParser::audioFormatToString(tag->audioFormat));

        // Sound Rate
        QString prevRate, currRate;
        switch (prevTag->soundRate) {
            case 0: prevRate = "5.5 kHz"; break;
            case 1: prevRate = "11 kHz"; break;
            case 2: prevRate = "22 kHz"; break;
            case 3: prevRate = "44 kHz"; break;
            default: prevRate = QString::number(prevTag->soundRate); break;
        }
        switch (tag->soundRate) {
            case 0: currRate = "5.5 kHz"; break;
            case 1: currRate = "11 kHz"; break;
            case 2: currRate = "22 kHz"; break;
            case 3: currRate = "44 kHz"; break;
            default: currRate = QString::number(tag->soundRate); break;
        }
        addCompareRow(headerItem, "Sample Rate", prevRate, currRate);

        addCompareRow(headerItem, "Sample Size",
                      prevTag->soundSize == 1 ? "16-bit" : "8-bit",
                      tag->soundSize == 1 ? "16-bit" : "8-bit");
        addCompareRow(headerItem, "Channels",
                      prevTag->soundType == 1 ? "Stereo" : "Mono",
                      tag->soundType == 1 ? "Stereo" : "Mono");
    }
}

void PropertyPanel::addTagFields(QTreeWidgetItem* parent, const FLVTag& tag) {
    // Tag Type
    QTreeWidgetItem* typeItem = new QTreeWidgetItem(parent);
    typeItem->setText(0, "Tag Type");
    typeItem->setText(1, FLVParser::tagTypeToString(tag.type));

    // Offset
    QTreeWidgetItem* offsetItem = new QTreeWidgetItem(parent);
    offsetItem->setText(0, "Offset");
    offsetItem->setText(1, QString("0x%1").arg(tag.offset, 0, 16));

    // Data Size
    QTreeWidgetItem* dataSizeItem = new QTreeWidgetItem(parent);
    dataSizeItem->setText(0, "Data Size");
    dataSizeItem->setText(1, QString("%1 bytes").arg(tag.dataSize));

    // Total Size
    QTreeWidgetItem* totalSizeItem = new QTreeWidgetItem(parent);
    totalSizeItem->setText(0, "Total Size");
    totalSizeItem->setText(1, QString("%1 bytes").arg(tag.totalSize));

    // Percentage
    QTreeWidgetItem* percentageItem = new QTreeWidgetItem(parent);
    percentageItem->setText(0, "Percentage");
    percentageItem->setText(1, QString("%1%").arg(tag.percentage, 0, 'f', 2));

    // Timestamp
    QTreeWidgetItem* timestampItem = new QTreeWidgetItem(parent);
    timestampItem->setText(0, "Timestamp");
    timestampItem->setText(1, QString("%1 ms").arg(tag.timestamp));

    // Stream ID
    QTreeWidgetItem* streamIDItem = new QTreeWidgetItem(parent);
    streamIDItem->setText(0, "Stream ID");
    streamIDItem->setText(1, QString::number(tag.streamID));

    // Video-specific fields
    if (tag.type == FLVTagType::Video) {
        QTreeWidgetItem* videoItem = new QTreeWidgetItem(parent);
        videoItem->setText(0, "Video Properties");
        videoItem->setText(1, "");
        videoItem->setExpanded(true);

        QTreeWidgetItem* frameTypeItem = new QTreeWidgetItem(videoItem);
        frameTypeItem->setText(0, "Frame Type");
        frameTypeItem->setText(1, FLVParser::frameTypeToString(tag.frameType));

        QTreeWidgetItem* codecItem = new QTreeWidgetItem(videoItem);
        codecItem->setText(0, "Video Codec");
        codecItem->setText(1, FLVParser::videoCodecToString(tag.videoCodec));

        if (tag.videoCodec == FLVVideoCodec::AVC || tag.videoCodec == FLVVideoCodec::HEVC) {
            QTreeWidgetItem* avcPacketTypeItem = new QTreeWidgetItem(videoItem);
            avcPacketTypeItem->setText(0, "AVC Packet Type");
            QString avcTypeStr;
            switch (tag.avcPacketType) {
                case 0: avcTypeStr = "0 (Sequence Header)"; break;
                case 1: avcTypeStr = "1 (NALU)"; break;
                case 2: avcTypeStr = "2 (End of Sequence)"; break;
                default: avcTypeStr = QString::number(tag.avcPacketType); break;
            }
            avcPacketTypeItem->setText(1, avcTypeStr);

            QTreeWidgetItem* compositionTimeItem = new QTreeWidgetItem(videoItem);
            compositionTimeItem->setText(0, "Composition Time");
            compositionTimeItem->setText(1, QString("%1 ms").arg(tag.compositionTime));
        }
    }

    // Audio-specific fields
    if (tag.type == FLVTagType::Audio) {
        QTreeWidgetItem* audioItem = new QTreeWidgetItem(parent);
        audioItem->setText(0, "Audio Properties");
        audioItem->setText(1, "");
        audioItem->setExpanded(true);

        QTreeWidgetItem* formatItem = new QTreeWidgetItem(audioItem);
        formatItem->setText(0, "Audio Format");
        formatItem->setText(1, FLVParser::audioFormatToString(tag.audioFormat));

        QTreeWidgetItem* soundRateItem = new QTreeWidgetItem(audioItem);
        soundRateItem->setText(0, "Sample Rate");
        QString rateStr;
        switch (tag.soundRate) {
            case 0: rateStr = "5.5 kHz"; break;
            case 1: rateStr = "11 kHz"; break;
            case 2: rateStr = "22 kHz"; break;
            case 3: rateStr = "44 kHz"; break;
            default: rateStr = QString("Unknown (%1)").arg(tag.soundRate); break;
        }
        soundRateItem->setText(1, rateStr);

        QTreeWidgetItem* soundSizeItem = new QTreeWidgetItem(audioItem);
        soundSizeItem->setText(0, "Bit Depth");
        soundSizeItem->setText(1, tag.soundSize == 0 ? "8-bit" : "16-bit");

        QTreeWidgetItem* soundTypeItem = new QTreeWidgetItem(audioItem);
        soundTypeItem->setText(0, "Channels");
        soundTypeItem->setText(1, tag.soundType == 0 ? "Mono (1)" : "Stereo (2)");

        if (tag.audioFormat == FLVAudioFormat::AAC) {
            QTreeWidgetItem* aacPacketTypeItem = new QTreeWidgetItem(audioItem);
            aacPacketTypeItem->setText(0, "AAC Packet Type");
            aacPacketTypeItem->setText(1, tag.aacPacketType == 0 ? "0 (Sequence Header)" : "1 (Raw AAC Data)");
        }
    }

    // Script data fields
    if (tag.type == FLVTagType::ScriptData) {
        if (!tag.scriptName.isEmpty()) {
            QTreeWidgetItem* scriptNameItem = new QTreeWidgetItem(parent);
            scriptNameItem->setText(0, "Script Name");
            scriptNameItem->setText(1, tag.scriptName);
        }

        if (!tag.scriptData.isEmpty()) {
            QTreeWidgetItem* scriptDataItem = new QTreeWidgetItem(parent);
            scriptDataItem->setText(0, "Script Data");
            scriptDataItem->setText(1, QString("%1 properties").arg(tag.scriptData.size()));
            scriptDataItem->setExpanded(true);

            // Display common metadata properties first
            QStringList commonKeys = {"duration", "width", "height", "videodatarate", "audiodatarate",
                                      "framerate", "videocodecid", "audiocodecid", "filesize"};

            for (const QString& key : commonKeys) {
                if (tag.scriptData.contains(key)) {
                    QTreeWidgetItem* propItem = new QTreeWidgetItem(scriptDataItem);
                    propItem->setText(0, key);
                    QVariant value = tag.scriptData[key];

                    // Format specific values
                    if (key == "duration") {
                        propItem->setText(1, QString("%1 seconds").arg(value.toDouble(), 0, 'f', 2));
                    } else if (key == "width" || key == "height") {
                        propItem->setText(1, QString("%1 pixels").arg(value.toInt()));
                    } else if (key == "videodatarate" || key == "audiodatarate") {
                        propItem->setText(1, QString("%1 kbps").arg(value.toDouble(), 0, 'f', 2));
                    } else if (key == "framerate") {
                        propItem->setText(1, QString("%1 fps").arg(value.toDouble(), 0, 'f', 2));
                    } else if (key == "filesize") {
                        propItem->setText(1, QString("%1 bytes").arg(value.toLongLong()));
                    } else {
                        propItem->setText(1, value.toString());
                    }
                }
            }

            // Display remaining properties
            for (auto it = tag.scriptData.begin(); it != tag.scriptData.end(); ++it) {
                if (!commonKeys.contains(it.key())) {
                    QTreeWidgetItem* propItem = new QTreeWidgetItem(scriptDataItem);
                    propItem->setText(0, it.key());
                    propItem->setText(1, it.value().toString());
                }
            }
        }
    }
}

const FLVTag* PropertyPanel::findTagByOffset(const QVector<FLVTag>& tags, int64_t offset) {
    for (const FLVTag& tag : tags) {
        if (tag.offset == offset) {
            return &tag;
        }
    }
    return nullptr;
}

} // namespace VideoStudio
