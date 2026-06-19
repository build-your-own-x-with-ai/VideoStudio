#include "panels/explorerpanel.h"
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDebug>

namespace VideoStudio {

ExplorerPanel::ExplorerPanel(QWidget* parent)
    : QWidget(parent)
    , m_tsParser(nullptr)
    , m_mp4Parser(nullptr)
    , m_mkvParser(nullptr)
    , m_aviParser(nullptr)
    , m_flvParser(nullptr)
    , m_contextMenuItem(nullptr)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels(QStringList() << "Element" << "Type/ID" << "%");
    m_treeWidget->setColumnWidth(0, 200);
    m_treeWidget->setColumnWidth(1, 100);
    m_treeWidget->setColumnWidth(2, 60);
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_treeWidget, &QTreeWidget::itemClicked,
            this, &ExplorerPanel::onItemClicked);
    connect(m_treeWidget, &QTreeWidget::itemChanged,
            this, &ExplorerPanel::onItemChanged);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested,
            this, &ExplorerPanel::onContextMenu);

    layout->addWidget(m_treeWidget);
}

ExplorerPanel::~ExplorerPanel() {
}

void ExplorerPanel::setTSParser(TSParser* parser) {
    m_tsParser = parser;
    m_mp4Parser = nullptr;
    buildTree();
}

void ExplorerPanel::setMP4Parser(MP4Parser* parser) {
    m_mp4Parser = parser;
    m_tsParser = nullptr;
    m_mkvParser = nullptr;
    buildTree();
}

void ExplorerPanel::setMKVParser(MKVParser* parser) {
    m_mkvParser = parser;
    m_tsParser = nullptr;
    m_mp4Parser = nullptr;
    m_aviParser = nullptr;
    buildTree();
}

void ExplorerPanel::setAVIParser(AVIParser* parser) {
    m_aviParser = parser;
    m_tsParser = nullptr;
    m_mp4Parser = nullptr;
    m_mkvParser = nullptr;
    m_flvParser = nullptr;
    buildTree();
}

void ExplorerPanel::setFLVParser(FLVParser* parser) {
    m_flvParser = parser;
    m_tsParser = nullptr;
    m_mp4Parser = nullptr;
    m_mkvParser = nullptr;
    m_aviParser = nullptr;
    buildTree();
}

void ExplorerPanel::clear() {
    m_treeWidget->clear();
    m_tsParser = nullptr;
    m_mp4Parser = nullptr;
    m_mkvParser = nullptr;
    m_aviParser = nullptr;
    m_flvParser = nullptr;
}

void ExplorerPanel::buildTree() {
    m_treeWidget->clear();

    if (m_tsParser) {
        addTransportStreamNode();
    } else if (m_mp4Parser) {
        addMP4StreamNode();
    } else if (m_mkvParser) {
        addMKVStreamNode();
    } else if (m_aviParser) {
        addAVIStreamNode();
    } else if (m_flvParser) {
        addFLVStreamNode();
    }
}

void ExplorerPanel::addTransportStreamNode() {
    // Root node: Transport stream
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
    rootItem->setText(0, "Transport stream");
    rootItem->setText(1, "PID/ID");
    rootItem->setText(2, "%");
    rootItem->setExpanded(true);

    // Add PSI Table node
    QTreeWidgetItem* psiTableItem = new QTreeWidgetItem(rootItem);
    psiTableItem->setText(0, "PSI Table");
    psiTableItem->setCheckState(0, Qt::Checked);
    psiTableItem->setExpanded(true);

    // Add PAT
    const auto& pids = m_tsParser->getPIDs();
    if (pids.contains(0x0000)) {
        QTreeWidgetItem* patItem = new QTreeWidgetItem(psiTableItem);
        patItem->setText(0, "PAT");
        patItem->setText(1, "0x0000");
        patItem->setText(2, QString::number(pids[0x0000].percentage, 'f', 1));
        patItem->setCheckState(0, Qt::Checked);
        patItem->setData(0, Qt::UserRole, 0x0000); // Store PID
    }

    // Add programs
    const auto& programs = m_tsParser->getPrograms();
    for (const auto& program : programs) {
        QTreeWidgetItem* programItem = new QTreeWidgetItem(rootItem);

        QString programName = QString("Program #%1").arg(program.programNumber);
        if (!program.serviceName.isEmpty()) {
            programName += QString(" (%1)").arg(program.serviceName);
        }

        programItem->setText(0, programName);
        programItem->setText(1, QString("0x%1").arg(program.programNumber, 4, 16, QChar('0')));
        programItem->setCheckState(0, Qt::Checked);
        programItem->setExpanded(true);

        // Add PMT
        if (pids.contains(program.pmtPid)) {
            QTreeWidgetItem* pmtItem = new QTreeWidgetItem(programItem);
            pmtItem->setText(0, "PMT");
            pmtItem->setText(1, QString("0x%1").arg(program.pmtPid, 4, 16, QChar('0')));
            pmtItem->setText(2, QString::number(pids[program.pmtPid].percentage, 'f', 1));
            pmtItem->setCheckState(0, Qt::Checked);
            pmtItem->setData(0, Qt::UserRole, program.pmtPid);
        }

        // Add elementary streams
        for (uint16_t esPid : program.elementaryPIDs) {
            if (pids.contains(esPid)) {
                const PIDInfo& pidInfo = pids[esPid];
                QTreeWidgetItem* esItem = new QTreeWidgetItem(programItem);

                QString esName = pidInfo.type;
                if (!pidInfo.codec.isEmpty()) {
                    esName += QString(" (%1)").arg(pidInfo.codec);
                }

                esItem->setText(0, esName);
                esItem->setText(1, QString("0x%1").arg(esPid, 4, 16, QChar('0')));
                esItem->setText(2, QString::number(pidInfo.percentage, 'f', 1));
                esItem->setCheckState(0, Qt::Checked);
                esItem->setData(0, Qt::UserRole, esPid);
            }
        }
    }

    // Add "Other PIDs" node for PIDs not in any program
    QTreeWidgetItem* otherPidsItem = new QTreeWidgetItem(rootItem);
    otherPidsItem->setText(0, "Other PIDs");
    otherPidsItem->setCheckState(0, Qt::Checked);
    otherPidsItem->setExpanded(false);

    QSet<uint16_t> programPids;
    programPids.insert(0x0000); // PAT
    for (const auto& program : programs) {
        programPids.insert(program.pmtPid);
        for (uint16_t esPid : program.elementaryPIDs) {
            programPids.insert(esPid);
        }
    }

    for (auto it = pids.begin(); it != pids.end(); ++it) {
        uint16_t pid = it.key();
        if (!programPids.contains(pid)) {
            const PIDInfo& pidInfo = it.value();
            QTreeWidgetItem* pidItem = new QTreeWidgetItem(otherPidsItem);

            QString pidName = pidInfo.type.isEmpty() ? "Unknown" : pidInfo.type;
            if (!pidInfo.codec.isEmpty()) {
                pidName += QString(" (%1)").arg(pidInfo.codec);
            }

            pidItem->setText(0, pidName);
            pidItem->setText(1, QString("0x%1").arg(pid, 4, 16, QChar('0')));
            pidItem->setText(2, QString::number(pidInfo.percentage, 'f', 1));
            pidItem->setCheckState(0, Qt::Checked);
            pidItem->setData(0, Qt::UserRole, pid);
        }
    }

    // Hide "Other PIDs" if empty
    if (otherPidsItem->childCount() == 0) {
        delete otherPidsItem;
    }
}

void ExplorerPanel::addMP4StreamNode() {
    // Root node: MP4 stream
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
    rootItem->setText(0, "MP4 stream");
    rootItem->setText(1, "Type");
    rootItem->setText(2, "%");
    rootItem->setExpanded(true);

    // Add all atoms
    const auto& atoms = m_mp4Parser->getAtoms();
    addMP4AtomNodes(rootItem, atoms);
}

void ExplorerPanel::addMP4AtomNodes(QTreeWidgetItem* parent, const QVector<MP4Atom>& atoms) {
    for (const MP4Atom& atom : atoms) {
        QTreeWidgetItem* atomItem = new QTreeWidgetItem(parent);

        // Atom name with additional info
        QString atomName = atom.type;
        if (atom.type == "ftyp" && !atom.majorBrand.isEmpty()) {
            atomName += QString(" (%1)").arg(atom.majorBrand);
        } else if (atom.type == "trak" && atom.trackId > 0) {
            atomName += QString(" #%1").arg(atom.trackId);
        } else if (atom.type == "hdlr" && !atom.handlerType.isEmpty()) {
            atomName += QString(" (%1)").arg(atom.handlerType);
        } else if (atom.type == "stsd" && !atom.codecType.isEmpty()) {
            atomName += QString(" (%1)").arg(atom.codecType);
            if (atom.width > 0 && atom.height > 0) {
                atomName += QString(" %1x%2").arg(atom.width).arg(atom.height);
            } else if (atom.sampleRate > 0) {
                atomName += QString(" %1Hz").arg(atom.sampleRate);
            }
        }

        atomItem->setText(0, atomName);
        atomItem->setText(1, QString("0x%1").arg(atom.offset, 0, 16));
        atomItem->setText(2, QString::number(atom.percentage, 'f', 2));
        atomItem->setCheckState(0, Qt::Checked);

        // Store atom offset as user data (cast to qlonglong for Qt 6.5.3)
        atomItem->setData(0, Qt::UserRole, static_cast<qlonglong>(atom.offset));

        // Recursively add children
        if (!atom.children.isEmpty()) {
            addMP4AtomNodes(atomItem, atom.children);
            atomItem->setExpanded(true);
        }
    }
}

void ExplorerPanel::addMKVStreamNode() {
    // Root node: MKV stream
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
    rootItem->setText(0, "MKV stream");
    rootItem->setText(1, "Type");
    rootItem->setText(2, "%");
    rootItem->setExpanded(true);

    // Add all elements
    const auto& elements = m_mkvParser->getElements();
    addMKVElementNodes(rootItem, elements);
}

void ExplorerPanel::addMKVElementNodes(QTreeWidgetItem* parent, const QVector<EBMLElement>& elements) {
    for (const EBMLElement& element : elements) {
        QTreeWidgetItem* elementItem = new QTreeWidgetItem(parent);

        // Element name with additional info
        QString elementName = element.name;
        if (element.name == "TrackEntry" && element.trackNumber > 0) {
            elementName += QString(" #%1").arg(element.trackNumber);
            if (!element.codecID.isEmpty()) {
                elementName += QString(" (%1)").arg(element.codecID);
            }
            if (element.pixelWidth > 0 && element.pixelHeight > 0) {
                elementName += QString(" %1x%2").arg(element.pixelWidth).arg(element.pixelHeight);
            } else if (element.samplingFrequency > 0) {
                elementName += QString(" %1Hz").arg(element.samplingFrequency, 0, 'f', 0);
            }
        } else if (!element.stringValue.isEmpty()) {
            elementName += QString(" (%1)").arg(element.stringValue);
        } else if (element.uintValue > 0 && element.name != "TrackNumber" && element.name != "TrackUID") {
            elementName += QString(" (%1)").arg(element.uintValue);
        } else if (element.floatValue > 0) {
            elementName += QString(" (%1)").arg(element.floatValue, 0, 'f', 2);
        }

        elementItem->setText(0, elementName);
        elementItem->setText(1, QString("0x%1").arg(element.offset, 0, 16));
        elementItem->setText(2, QString::number(element.percentage, 'f', 2));
        elementItem->setCheckState(0, Qt::Checked);

        // Store element offset as user data
        elementItem->setData(0, Qt::UserRole, element.offset);

        // Recursively add children
        if (!element.children.isEmpty()) {
            addMKVElementNodes(elementItem, element.children);
            elementItem->setExpanded(true);
        }
    }
}

void ExplorerPanel::addAVIStreamNode() {
    // Root node: AVI stream
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
    rootItem->setText(0, "AVI stream");
    rootItem->setText(1, "Type");
    rootItem->setText(2, "%");
    rootItem->setExpanded(true);

    // Add all chunks
    const auto& chunks = m_aviParser->getChunks();
    addAVIChunkNodes(rootItem, chunks);
}

void ExplorerPanel::addAVIChunkNodes(QTreeWidgetItem* parent, const QVector<AVIChunk>& chunks) {
    for (const AVIChunk& chunk : chunks) {
        QTreeWidgetItem* chunkItem = new QTreeWidgetItem(parent);

        // Chunk name with additional info
        QString chunkName = chunk.fourCC;
        if (chunk.fourCC == "RIFF" || chunk.fourCC == "LIST") {
            chunkName += QString(" (%1)").arg(chunk.listType);
        } else if (chunk.fourCC == "avih") {
            chunkName += QString(" (%1x%2, %3 frames)")
                .arg(chunk.width)
                .arg(chunk.height)
                .arg(chunk.totalFrames);
        } else if (chunk.fourCC == "strh") {
            chunkName += QString(" (%1, %2)")
                .arg(chunk.streamType)
                .arg(chunk.codecFourCC);
        }

        chunkItem->setText(0, chunkName);
        chunkItem->setText(1, QString("0x%1").arg(chunk.offset, 0, 16));
        chunkItem->setText(2, QString::number(chunk.percentage, 'f', 2));
        chunkItem->setCheckState(0, Qt::Checked);

        // Store chunk offset as user data
        chunkItem->setData(0, Qt::UserRole, chunk.offset);

        // Recursively add children
        if (!chunk.children.isEmpty()) {
            addAVIChunkNodes(chunkItem, chunk.children);
            chunkItem->setExpanded(true);
        }
    }
}

void ExplorerPanel::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);

    if (!item) {
        return;
    }

    // Get PID or offset from item data
    QVariant data = item->data(0, Qt::UserRole);
    if (data.isValid()) {
        if (m_tsParser) {
            // TS mode: emit PID
            uint16_t pid = data.toUInt();
            emit pidSelected(pid);
            qDebug() << "PID selected:" << QString("0x%1").arg(pid, 4, 16, QChar('0'));
        } else if (m_mp4Parser || m_mkvParser || m_aviParser || m_flvParser) {
            // MP4/MKV/AVI/FLV mode: emit offset
            int64_t offset = data.toLongLong();
            emit packetSelected(offset);
            qDebug() << "Element selected at offset:" << QString("0x%1").arg(offset, 0, 16);
        }
    }
}

void ExplorerPanel::onItemChanged(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);

    if (!item) {
        return;
    }

    // Handle checkbox state change
    Qt::CheckState state = item->checkState(0);
    // qDebug() << "Item" << item->text(0) << "check state changed to" << state;
}

void ExplorerPanel::onContextMenu(const QPoint& pos) {
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    if (!item) {
        return;
    }

    m_contextMenuItem = item;

    QMenu contextMenu(this);

    // Always available actions
    QAction* expandAllAction = contextMenu.addAction("Expand All");
    connect(expandAllAction, &QAction::triggered, this, &ExplorerPanel::onExpandAll);

    QAction* collapseAllAction = contextMenu.addAction("Collapse All");
    connect(collapseAllAction, &QAction::triggered, this, &ExplorerPanel::onCollapseAll);

    // Check if item has data (PID for TS, offset for others)
    QVariant itemData = item->data(0, Qt::UserRole);
    if (itemData.isValid()) {
        contextMenu.addSeparator();

        // For TS files: show all options
        if (m_tsParser) {
            QAction* dumpESAction = contextMenu.addAction("Dump Elementary Stream");
            connect(dumpESAction, &QAction::triggered, this, &ExplorerPanel::onDumpES);

            QAction* compareModeAction = contextMenu.addAction("Set Compare Mode");
            connect(compareModeAction, &QAction::triggered, this, &ExplorerPanel::onCompareMode);

            QAction* syncModeAction = contextMenu.addAction("Set Sync Mode");
            connect(syncModeAction, &QAction::triggered, this, &ExplorerPanel::onSyncMode);
        }
        // For container formats: show dump option
        else if (m_mp4Parser || m_mkvParser || m_aviParser || m_flvParser) {
            QAction* dumpDataAction = contextMenu.addAction("Dump Data to File");
            connect(dumpDataAction, &QAction::triggered, this, &ExplorerPanel::onDumpES);
        }
    }

    contextMenu.exec(m_treeWidget->mapToGlobal(pos));
}

void ExplorerPanel::onExpandAll() {
    m_treeWidget->expandAll();
}

void ExplorerPanel::onCollapseAll() {
    m_treeWidget->collapseAll();
}

void ExplorerPanel::onDumpES() {
    if (!m_contextMenuItem) {
        return;
    }

    QVariant itemData = m_contextMenuItem->data(0, Qt::UserRole);
    if (!itemData.isValid()) {
        return;
    }

    // TS file: Dump elementary stream for selected PID
    if (m_tsParser) {
        uint16_t pid = itemData.toUInt();

        QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save Elementary Stream"),
            QString("pid_0x%1.es").arg(pid, 4, 16, QChar('0')),
            tr("Elementary Stream (*.es);;H.264 (*.h264);;H.265 (*.h265);;All Files (*)"));

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
        int64_t totalBytes = 0;

        for (const TSPacket& packet : packets) {
            if (packet.pid == pid && !packet.payload.isEmpty()) {
                const QByteArray& payload = packet.payload;

                // Check if this is a PES packet (starts with 0x000001)
                if (payload.size() >= 6 &&
                    (uint8_t)payload[0] == 0x00 &&
                    (uint8_t)payload[1] == 0x00 &&
                    (uint8_t)payload[2] == 0x01) {

                    // This is a PES packet, extract elementary stream data
                    uint8_t streamId = (uint8_t)payload[3];

                    // Check if this is a video/audio stream (0xC0-0xDF = audio, 0xE0-0xEF = video)
                    if ((streamId >= 0xC0 && streamId <= 0xDF) ||
                        (streamId >= 0xE0 && streamId <= 0xEF)) {

                        // PES packet structure:
                        // 0-2: Start code (0x000001)
                        // 3: Stream ID
                        // 4-5: PES packet length
                        // 6: Flags
                        // 7: Flags
                        // 8: PES header data length
                        // 9+: Optional fields (PTS/DTS/etc)
                        // 9+headerLen: Elementary stream data

                        if (payload.size() >= 9) {
                            uint8_t pesHeaderDataLength = (uint8_t)payload[8];
                            int esDataOffset = 9 + pesHeaderDataLength;

                            if (esDataOffset < payload.size()) {
                                // Write only the elementary stream data (skip PES header)
                                QByteArray esData = payload.mid(esDataOffset);
                                file.write(esData);
                                totalBytes += esData.size();
                                count++;
                            }
                        }
                    } else {
                        // Not a standard video/audio PES, write as-is
                        file.write(payload);
                        totalBytes += payload.size();
                        count++;
                    }
                } else {
                    // Not a PES packet (continuation packet), write as-is
                    file.write(payload);
                    totalBytes += payload.size();
                    count++;
                }
            }
        }

        file.close();
        QMessageBox::information(this, tr("Dump Complete"),
            tr("Dumped %1 packets (%2 bytes) to:\n%3")
                .arg(count)
                .arg(totalBytes)
                .arg(fileName));
    }
    // MP4 file: Dump atom data
    else if (m_mp4Parser) {
        int64_t offset = itemData.toLongLong();

        // Find atom by offset
        std::function<const MP4Atom*(const QVector<MP4Atom>&, int64_t)> findAtom;
        findAtom = [&](const QVector<MP4Atom>& atoms, int64_t off) -> const MP4Atom* {
            for (const auto& atom : atoms) {
                if (atom.offset == off) return &atom;
                if (!atom.children.isEmpty()) {
                    const MP4Atom* found = findAtom(atom.children, off);
                    if (found) return found;
                }
            }
            return nullptr;
        };

        const MP4Atom* atom = findAtom(m_mp4Parser->getAtoms(), offset);
        if (!atom) {
            QMessageBox::warning(this, tr("Error"), tr("Atom not found"));
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
            sourceFile.close();
            return;
        }

        sourceFile.seek(atom->offset);
        QByteArray data = sourceFile.read(atom->size);
        destFile.write(data);

        sourceFile.close();
        destFile.close();

        QMessageBox::information(this, tr("Dump Complete"),
            tr("Dumped %1 bytes to:\n%2").arg(atom->size).arg(fileName));
    }
    // MKV file: Dump element data
    else if (m_mkvParser) {
        int64_t offset = itemData.toLongLong();

        // Find element by offset
        std::function<const EBMLElement*(const QVector<EBMLElement>&, int64_t)> findElement;
        findElement = [&](const QVector<EBMLElement>& elements, int64_t off) -> const EBMLElement* {
            for (const auto& elem : elements) {
                if (elem.offset == off) return &elem;
                if (!elem.children.isEmpty()) {
                    const EBMLElement* found = findElement(elem.children, off);
                    if (found) return found;
                }
            }
            return nullptr;
        };

        const EBMLElement* element = findElement(m_mkvParser->getElements(), offset);
        if (!element) {
            QMessageBox::warning(this, tr("Error"), tr("Element not found"));
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
            sourceFile.close();
            return;
        }

        sourceFile.seek(element->offset);
        QByteArray data = sourceFile.read(element->totalSize);
        destFile.write(data);

        sourceFile.close();
        destFile.close();

        QMessageBox::information(this, tr("Dump Complete"),
            tr("Dumped %1 bytes to:\n%2").arg(element->totalSize).arg(fileName));
    }
    // AVI file: Dump chunk data
    else if (m_aviParser) {
        int64_t offset = itemData.toLongLong();

        // Find chunk by offset
        std::function<const AVIChunk*(const QVector<AVIChunk>&, int64_t)> findChunk;
        findChunk = [&](const QVector<AVIChunk>& chunks, int64_t off) -> const AVIChunk* {
            for (const auto& chunk : chunks) {
                if (chunk.offset == off) return &chunk;
                if (!chunk.children.isEmpty()) {
                    const AVIChunk* found = findChunk(chunk.children, off);
                    if (found) return found;
                }
            }
            return nullptr;
        };

        const AVIChunk* chunk = findChunk(m_aviParser->getChunks(), offset);
        if (!chunk) {
            QMessageBox::warning(this, tr("Error"), tr("Chunk not found"));
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
            sourceFile.close();
            return;
        }

        sourceFile.seek(chunk->offset);
        QByteArray data = sourceFile.read(chunk->totalSize);
        destFile.write(data);

        sourceFile.close();
        destFile.close();

        QMessageBox::information(this, tr("Dump Complete"),
            tr("Dumped %1 bytes to:\n%2").arg(chunk->totalSize).arg(fileName));
    }
    // FLV file: Dump tag data
    else if (m_flvParser) {
        int64_t offset = itemData.toLongLong();

        // Find tag by offset
        const auto& tags = m_flvParser->getTags();
        const FLVTag* tag = nullptr;
        for (const auto& t : tags) {
            if (t.offset == offset) {
                tag = &t;
                break;
            }
        }

        if (!tag) {
            QMessageBox::warning(this, tr("Error"), tr("Tag not found"));
            return;
        }

        QString tagType;
        switch (tag->type) {
            case FLVTagType::Video: tagType = "video"; break;
            case FLVTagType::Audio: tagType = "audio"; break;
            case FLVTagType::ScriptData: tagType = "script"; break;
            default: tagType = "unknown"; break;
        }

        QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save Tag Data"),
            QString("flv_%1_0x%2.bin").arg(tagType).arg(tag->offset, 0, 16),
            tr("Binary Files (*.bin);;All Files (*)"));

        if (fileName.isEmpty()) {
            return;
        }

        QFile sourceFile(m_flvParser->getFilePath());
        if (!sourceFile.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to open source file"));
            return;
        }

        QFile destFile(fileName);
        if (!destFile.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to open destination file"));
            sourceFile.close();
            return;
        }

        sourceFile.seek(tag->offset);
        QByteArray data = sourceFile.read(tag->totalSize);
        destFile.write(data);

        sourceFile.close();
        destFile.close();

        QMessageBox::information(this, tr("Dump Complete"),
            tr("Dumped %1 bytes to:\n%2").arg(tag->totalSize).arg(fileName));
    }
}

void ExplorerPanel::onCompareMode() {
    if (!m_contextMenuItem) {
        return;
    }

    QVariant pidData = m_contextMenuItem->data(0, Qt::UserRole);
    if (!pidData.isValid()) {
        return;
    }

    uint16_t pid = pidData.toUInt();
    emit setCompareMode(pid);

    qDebug() << "Compare mode set for PID:" << QString("0x%1").arg(pid, 4, 16, QChar('0'));
}

void ExplorerPanel::onSyncMode() {
    if (!m_contextMenuItem) {
        return;
    }

    QVariant pidData = m_contextMenuItem->data(0, Qt::UserRole);
    if (!pidData.isValid()) {
        return;
    }

    uint16_t pid = pidData.toUInt();
    emit setSyncMode(pid);

    qDebug() << "Sync mode set for PID:" << QString("0x%1").arg(pid, 4, 16, QChar('0'));
}

void ExplorerPanel::addFLVStreamNode() {
    // Root node: FLV stream
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
    rootItem->setText(0, "FLV stream");
    rootItem->setText(1, "Type");
    rootItem->setText(2, "%");
    rootItem->setExpanded(true);

    // FLV Header
    QTreeWidgetItem* headerItem = new QTreeWidgetItem(rootItem);
    const FLVHeader& header = m_flvParser->getHeader();
    headerItem->setText(0, "FLV Header");
    headerItem->setText(1, QString("v%1").arg(header.version));
    headerItem->setText(2, QString::number((header.dataOffset * 100.0) / m_flvParser->getFileSize(), 'f', 2));
    headerItem->setData(0, Qt::UserRole, QVariant::fromValue<int64_t>(0));

    // Video/Audio flags
    QTreeWidgetItem* flagsItem = new QTreeWidgetItem(headerItem);
    flagsItem->setText(0, "Flags");
    flagsItem->setText(1, QString("Video:%1 Audio:%2")
        .arg(header.hasVideo() ? "Yes" : "No")
        .arg(header.hasAudio() ? "Yes" : "No"));

    // Tags
    const auto& tags = m_flvParser->getTags();
    addFLVTagNodes(rootItem, tags);

    // Statistics
    QTreeWidgetItem* statsItem = new QTreeWidgetItem(rootItem);
    statsItem->setText(0, "Statistics");
    statsItem->setText(1, "");
    statsItem->setText(2, "");

    QTreeWidgetItem* videoCountItem = new QTreeWidgetItem(statsItem);
    videoCountItem->setText(0, "Video Tags");
    videoCountItem->setText(1, QString::number(m_flvParser->getVideoTagCount()));

    QTreeWidgetItem* audioCountItem = new QTreeWidgetItem(statsItem);
    audioCountItem->setText(0, "Audio Tags");
    audioCountItem->setText(1, QString::number(m_flvParser->getAudioTagCount()));

    QTreeWidgetItem* scriptCountItem = new QTreeWidgetItem(statsItem);
    scriptCountItem->setText(0, "Script Data Tags");
    scriptCountItem->setText(1, QString::number(m_flvParser->getScriptDataTagCount()));

    QTreeWidgetItem* keyFrameCountItem = new QTreeWidgetItem(statsItem);
    keyFrameCountItem->setText(0, "Key Frames");
    keyFrameCountItem->setText(1, QString::number(m_flvParser->getKeyFrameCount()));
}

void ExplorerPanel::addFLVTagNodes(QTreeWidgetItem* parent, const QVector<FLVTag>& tags) {
    // Group tags by type
    QTreeWidgetItem* videoTagsItem = new QTreeWidgetItem(parent);
    videoTagsItem->setText(0, "Video Tags");
    videoTagsItem->setText(1, QString::number(m_flvParser->getVideoTagCount()));

    QTreeWidgetItem* audioTagsItem = new QTreeWidgetItem(parent);
    audioTagsItem->setText(0, "Audio Tags");
    audioTagsItem->setText(1, QString::number(m_flvParser->getAudioTagCount()));

    QTreeWidgetItem* scriptTagsItem = new QTreeWidgetItem(parent);
    scriptTagsItem->setText(0, "Script Data Tags");
    scriptTagsItem->setText(1, QString::number(m_flvParser->getScriptDataTagCount()));

    // Add individual tags (limit to first 100 of each type for performance)
    int videoCount = 0;
    int audioCount = 0;
    int scriptCount = 0;

    for (const FLVTag& tag : tags) {
        QTreeWidgetItem* parentItem = nullptr;
        QString tagName;

        if (tag.type == FLVTagType::Video && videoCount < 100) {
            parentItem = videoTagsItem;
            tagName = QString("Video Tag #%1").arg(videoCount);
            videoCount++;
        } else if (tag.type == FLVTagType::Audio && audioCount < 100) {
            parentItem = audioTagsItem;
            tagName = QString("Audio Tag #%1").arg(audioCount);
            audioCount++;
        } else if (tag.type == FLVTagType::ScriptData && scriptCount < 100) {
            parentItem = scriptTagsItem;
            tagName = tag.scriptName.isEmpty() ? QString("Script Tag #%1").arg(scriptCount) : tag.scriptName;
            scriptCount++;
        }

        if (parentItem) {
            QTreeWidgetItem* tagItem = new QTreeWidgetItem(parentItem);
            tagItem->setText(0, tagName);
            tagItem->setText(1, QString("@0x%1").arg(tag.offset, 0, 16));
            tagItem->setText(2, QString::number(tag.percentage, 'f', 2));
            tagItem->setData(0, Qt::UserRole, QVariant::fromValue<int64_t>(tag.offset));
        }
    }
}

} // namespace VideoStudio
