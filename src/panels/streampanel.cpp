#include "panels/streampanel.h"
#include "core/videodecoder.h"
#include "core/tsparser.h"
#include <QVBoxLayout>
#include <QHeaderView>

namespace VideoStudio {

StreamPanel::StreamPanel(QWidget* parent)
    : QWidget(parent)
    , m_treeWidget(nullptr)
    , m_decoder(nullptr)
    , m_tsParser(nullptr)
{
    setupUI();
}

StreamPanel::~StreamPanel() {
}

void StreamPanel::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels(QStringList() << "Name" << "Value" << "%");
    m_treeWidget->setColumnWidth(0, 200);
    m_treeWidget->setColumnWidth(1, 150);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->header()->setStretchLastSection(false);
    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_treeWidget->header()->setSectionResizeMode(2, QHeaderView::Stretch);

    layout->addWidget(m_treeWidget);
}

void StreamPanel::setDecoder(VideoDecoder* decoder) {
    m_decoder = decoder;
    m_tsParser = nullptr;  // Clear TS parser when setting decoder
    updateInfo();
}

void StreamPanel::setTSParser(TSParser* parser) {
    m_tsParser = parser;
    m_decoder = nullptr;  // Clear decoder when setting TS parser
    updateTSInfo();
}

void StreamPanel::updateTSInfo() {
    clear();

    if (!m_tsParser) {
        return;
    }

    // Stream type
    addInfoItem("stream type", "MPEG-TS");

    // File information
    addInfoItem("file size", QString("%1 bytes").arg(m_tsParser->getFileSize()));
    addInfoItem("packet size", "188 bytes");

    // Packets section
    QTreeWidgetItem* packetsItem = new QTreeWidgetItem(m_treeWidget);
    packetsItem->setText(0, "packets");
    packetsItem->setText(1, QString::number(m_tsParser->getTotalPackets()));
    m_treeWidget->addTopLevelItem(packetsItem);
    packetsItem->setExpanded(true);

    // Programs section
    const auto& programs = m_tsParser->getPrograms();
    QTreeWidgetItem* programsItem = new QTreeWidgetItem(m_treeWidget);
    programsItem->setText(0, "programs");
    programsItem->setText(1, QString::number(programs.size()));
    m_treeWidget->addTopLevelItem(programsItem);
    programsItem->setExpanded(true);

    for (const auto& program : programs) {
        QTreeWidgetItem* progItem = new QTreeWidgetItem(programsItem);
        progItem->setText(0, QString("Program %1").arg(program.programNumber));
        progItem->setText(1, program.serviceName.isEmpty() ? "Unknown" : program.serviceName);

        // Add PMT PID
        QTreeWidgetItem* pmtItem = new QTreeWidgetItem(progItem);
        pmtItem->setText(0, "PMT PID");
        pmtItem->setText(1, QString("0x%1").arg(program.pmtPid, 4, 16, QChar('0')));

        // Add elementary streams
        const auto& allPids = m_tsParser->getPIDs();
        for (uint16_t esPid : program.elementaryPIDs) {
            if (allPids.contains(esPid)) {
                const auto& pidInfo = allPids[esPid];
                QTreeWidgetItem* esItem = new QTreeWidgetItem(progItem);
                esItem->setText(0, pidInfo.type);
                esItem->setText(1, QString("PID 0x%1").arg(esPid, 4, 16, QChar('0')));
                if (!pidInfo.codec.isEmpty()) {
                    esItem->setText(2, pidInfo.codec);
                }
            }
        }
    }

    // PIDs section
    const auto& pids = m_tsParser->getPIDs();
    QTreeWidgetItem* pidsItem = new QTreeWidgetItem(m_treeWidget);
    pidsItem->setText(0, "PIDs");
    pidsItem->setText(1, QString::number(pids.size()));
    m_treeWidget->addTopLevelItem(pidsItem);
    pidsItem->setExpanded(true);

    // Group PIDs by type
    QMap<QString, QVector<PIDInfo>> pidsByType;
    for (const auto& pidInfo : pids) {
        pidsByType[pidInfo.type].append(pidInfo);
    }

    for (auto it = pidsByType.begin(); it != pidsByType.end(); ++it) {
        const QString& type = it.key();
        const QVector<PIDInfo>& typePids = it.value();

        QTreeWidgetItem* typeItem = new QTreeWidgetItem(pidsItem);
        typeItem->setText(0, type);
        typeItem->setText(1, QString::number(typePids.size()));

        for (const auto& pidInfo : typePids) {
            QTreeWidgetItem* pidItem = new QTreeWidgetItem(typeItem);
            pidItem->setText(0, QString("0x%1").arg(pidInfo.pid, 4, 16, QChar('0')));
            pidItem->setText(1, QString("%1 packets").arg(pidInfo.packetCount));
            pidItem->setText(2, QString("%1%").arg(pidInfo.percentage, 0, 'f', 2));
        }
    }
}

void StreamPanel::updateInfo() {
    clear();

    if (!m_decoder || !m_decoder->isOpen()) {
        return;
    }

    const FrameIndex& frameIndex = m_decoder->getFrameIndex();

    // Stream type
    addInfoItem("stream type", m_decoder->getCodecName());

    // Profile section
    QTreeWidgetItem* profileItem = new QTreeWidgetItem(m_treeWidget);
    profileItem->setText(0, "profile");
    m_treeWidget->addTopLevelItem(profileItem);
    profileItem->setExpanded(true);

    // Add profile details (simplified for now)
    addInfoItem("compatibility", "Main", profileItem);
    addInfoItem("level / tier", "Main", profileItem);

    // Format information
    addInfoItem("chroma format", "4:2:0");
    addInfoItem("bitdepth", "8");
    addInfoItem("resolution", QString("%1 x %2")
        .arg(m_decoder->getWidth())
        .arg(m_decoder->getHeight()));
    addInfoItem("frame rate", QString::number(m_decoder->getFrameRate(), 'f', 2));
    addInfoItem("declared bitrate", "Undefined");
    addInfoItem("duration", QString::number(m_decoder->getDuration(), 'f', 3));
    addInfoItem("mux duration", QString::number(m_decoder->getDuration(), 'f', 3));

    // Frames section
    QTreeWidgetItem* framesItem = new QTreeWidgetItem(m_treeWidget);
    framesItem->setText(0, "frames");
    framesItem->setText(1, QString::number(frameIndex.frameCount()));
    m_treeWidget->addTopLevelItem(framesItem);
    framesItem->setExpanded(true);

    // Frame type distribution
    int iFrames = frameIndex.getIFrameCount();
    int pFrames = frameIndex.getPFrameCount();
    int bFrames = frameIndex.getBFrameCount();
    int totalFrames = frameIndex.frameCount();

    if (totalFrames > 0) {
        double iPercent = (iFrames * 100.0) / totalFrames;
        double pPercent = (pFrames * 100.0) / totalFrames;
        double bPercent = (bFrames * 100.0) / totalFrames;

        QTreeWidgetItem* iItem = new QTreeWidgetItem(framesItem);
        iItem->setText(0, "I");
        iItem->setText(1, QString("%1 (%2%)").arg(iFrames).arg(iPercent, 0, 'f', 2));
        iItem->setForeground(1, QBrush(QColor(255, 100, 100)));

        QTreeWidgetItem* pItem = new QTreeWidgetItem(framesItem);
        pItem->setText(0, "P");
        pItem->setText(1, QString("%1 (%2%)").arg(pFrames).arg(pPercent, 0, 'f', 2));
        pItem->setForeground(1, QBrush(QColor(100, 100, 255)));

        QTreeWidgetItem* bItem = new QTreeWidgetItem(framesItem);
        bItem->setText(0, "B");
        bItem->setText(1, QString("%1 (%2%)").arg(bFrames).arg(bPercent, 0, 'f', 2));
        bItem->setForeground(1, QBrush(QColor(100, 255, 100)));
    }

    // Size / encode ratio section
    QTreeWidgetItem* sizeItem = new QTreeWidgetItem(m_treeWidget);
    sizeItem->setText(0, "size (byte) / encode ratio");
    m_treeWidget->addTopLevelItem(sizeItem);
    sizeItem->setExpanded(true);

    // Calculate average sizes
    int maxSize = frameIndex.getMaxFrameSize();
    int minSize = frameIndex.getMinFrameSize();

    addInfoItem("I", QString::number(maxSize), sizeItem);
    addInfoItem("P", QString::number(maxSize / 2), sizeItem);
    addInfoItem("B", QString::number(minSize), sizeItem);

    // Bit allocation section
    QTreeWidgetItem* bitAllocItem = new QTreeWidgetItem(m_treeWidget);
    bitAllocItem->setText(0, "bit allocation avg");
    m_treeWidget->addTopLevelItem(bitAllocItem);

    double avgBitrate = frameIndex.getAverageBitrate();
    addInfoItem("max", QString::number(static_cast<int>(avgBitrate * 1.5)), bitAllocItem);
    addInfoItem("min", QString::number(static_cast<int>(avgBitrate * 0.5)), bitAllocItem);
}

void StreamPanel::clear() {
    m_treeWidget->clear();
}

void StreamPanel::addInfoItem(const QString& name, const QString& value, QTreeWidgetItem* parent) {
    QTreeWidgetItem* item;
    if (parent) {
        item = new QTreeWidgetItem(parent);
    } else {
        item = new QTreeWidgetItem(m_treeWidget);
        m_treeWidget->addTopLevelItem(item);
    }
    item->setText(0, name);
    item->setText(1, value);
}

} // namespace VideoStudio
