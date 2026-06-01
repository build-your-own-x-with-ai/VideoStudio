#include "widgets/packetview.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QDebug>

namespace VideoStudio {

PacketView::PacketView(QWidget* parent)
    : QWidget(parent)
    , m_parser(nullptr)
    , m_currentPacketIndex(-1)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels(QStringList() << "#" << "Offset" << "PID" << "Type" << "Properties");
    m_treeWidget->setColumnWidth(0, 60);   // #
    m_treeWidget->setColumnWidth(1, 100);  // Offset
    m_treeWidget->setColumnWidth(2, 80);   // PID
    m_treeWidget->setColumnWidth(3, 150);  // Type
    m_treeWidget->setColumnWidth(4, 300);  // Properties
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setRootIsDecorated(false);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(m_treeWidget, &QTreeWidget::itemClicked,
            this, &PacketView::onItemClicked);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &PacketView::onItemDoubleClicked);

    layout->addWidget(m_treeWidget);
}

PacketView::~PacketView() {
}

void PacketView::setTSParser(TSParser* parser) {
    m_parser = parser;
    buildPacketList();
}

void PacketView::clear() {
    m_treeWidget->clear();
    m_parser = nullptr;
    m_currentPacketIndex = -1;
}

void PacketView::setCurrentPacket(int packetIndex) {
    if (packetIndex < 0) {
        return;
    }

    m_currentPacketIndex = packetIndex;

    // Find and highlight the packet by its stored index
    bool found = false;
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
        QVariant data = item->data(0, Qt::UserRole);

        if (data.isValid() && data.toInt() == packetIndex) {
            // This is the target packet - highlight it
            item->setBackground(0, QBrush(QColor(100, 150, 255, 100)));
            item->setBackground(1, QBrush(QColor(100, 150, 255, 100)));
            item->setBackground(2, QBrush(QColor(100, 150, 255, 100)));
            item->setBackground(3, QBrush(QColor(100, 150, 255, 100)));
            item->setBackground(4, QBrush(QColor(100, 150, 255, 100)));
            m_treeWidget->setCurrentItem(item);
            m_treeWidget->scrollToItem(item, QAbstractItemView::PositionAtCenter);
            found = true;
            qDebug() << "Scrolled to packet:" << packetIndex;
        } else {
            // Clear background for other packets
            item->setBackground(0, QBrush());
            item->setBackground(1, QBrush());
            item->setBackground(2, QBrush());
            item->setBackground(3, QBrush());
            item->setBackground(4, QBrush());
        }
    }

    // Emit signal to update other panels (Property Panel, Hex Viewer)
    if (found) {
        emit packetSelected(packetIndex);
    }
}

void PacketView::buildPacketList() {
    m_treeWidget->clear();

    if (!m_parser) {
        return;
    }

    const auto& packets = m_parser->getPackets();
    const auto& pids = m_parser->getPIDs();

    qDebug() << "Building packet list with" << packets.size() << "packets";

    // Limit display to first 1000 packets for performance
    int maxPackets = qMin(1000, packets.size());

    for (int i = 0; i < maxPackets; ++i) {
        const TSPacket& packet = packets[i];

        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);

        // Packet number
        item->setText(0, QString::number(i));

        // Offset
        item->setText(1, QString("0x%1").arg(packet.offset, 8, 16, QChar('0')));

        // PID
        QString pidText = QString("0x%1").arg(packet.pid, 4, 16, QChar('0'));
        item->setText(2, pidText);

        // Type
        QString type = getPacketType(packet);
        item->setText(3, type);

        // Properties
        QString properties = formatProperties(packet);
        item->setText(4, properties);

        // Color coding
        QColor color = getPacketColor(packet);
        item->setForeground(2, QBrush(color));
        item->setForeground(3, QBrush(color));

        // Error indicators
        if (packet.transportErrorIndicator) {
            item->setIcon(0, style()->standardIcon(QStyle::SP_MessageBoxCritical));
        }

        // Store packet index in item data
        item->setData(0, Qt::UserRole, i);
    }

    if (packets.size() > maxPackets) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);
        item->setText(0, "...");
        item->setText(1, QString("(%1 more packets)").arg(packets.size() - maxPackets));
        item->setForeground(0, QBrush(Qt::gray));
        item->setForeground(1, QBrush(Qt::gray));
    }

    qDebug() << "Packet list built with" << m_treeWidget->topLevelItemCount() << "items";
}

QString PacketView::getPacketType(const TSPacket& packet) {
    if (!m_parser) {
        return "Unknown";
    }

    const auto& pids = m_parser->getPIDs();

    if (packet.pid == 0x0000) {
        return "PAT";
    } else if (packet.pid == 0x0001) {
        return "CAT";
    } else if (packet.pid == 0x0011) {
        return "SDT";
    } else if (packet.pid == 0x0012) {
        return "EIT";
    } else if (pids.contains(packet.pid)) {
        const PIDInfo& pidInfo = pids[packet.pid];
        if (pidInfo.type == "PMT") {
            return "PMT";
        } else if (pidInfo.type == "Video") {
            return QString("Video (%1)").arg(pidInfo.codec);
        } else if (pidInfo.type == "Audio") {
            return QString("Audio (%1)").arg(pidInfo.codec);
        } else {
            return pidInfo.type;
        }
    }

    return "Unknown";
}

QColor PacketView::getPacketColor(const TSPacket& packet) {
    if (packet.pid == 0x0000 || packet.pid == 0x0001) {
        return QColor(200, 100, 0); // Dark orange for PAT/CAT
    } else if (packet.pid == 0x0011 || packet.pid == 0x0012) {
        return QColor(50, 50, 200); // Dark blue for SDT/EIT
    }

    if (!m_parser) {
        return Qt::black;
    }

    const auto& pids = m_parser->getPIDs();
    if (pids.contains(packet.pid)) {
        const PIDInfo& pidInfo = pids[packet.pid];
        if (pidInfo.type == "PMT") {
            return QColor(200, 100, 0); // Dark orange for PMT
        } else if (pidInfo.type == "Video") {
            return QColor(0, 150, 0); // Dark green for video
        } else if (pidInfo.type == "Audio") {
            return QColor(0, 100, 200); // Dark blue for audio
        }
    }

    return Qt::black;
}

QString PacketView::formatProperties(const TSPacket& packet) {
    QStringList props;

    if (packet.transportErrorIndicator) {
        props << "ERROR";
    }

    if (packet.payloadUnitStartIndicator) {
        props << "PUSI";
    }

    if (packet.transportPriority) {
        props << "Priority";
    }

    if (packet.scramblingControl != 0) {
        props << QString("Scrambled(%1)").arg(packet.scramblingControl);
    }

    props << QString("CC=%1").arg(packet.continuityCounter);

    if (packet.hasAdaptationField) {
        props << "AF";
        if (packet.hasPCR) {
            props << QString("PCR=%1").arg(packet.pcr);
        }
    }

    if (packet.hasPTS) {
        props << QString("PTS=%1").arg(packet.pts);
    }

    if (packet.hasDTS) {
        props << QString("DTS=%1").arg(packet.dts);
    }

    props << QString("Payload=%1B").arg(packet.payload.size());

    return props.join(", ");
}

void PacketView::onItemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);

    if (!item) {
        return;
    }

    QVariant data = item->data(0, Qt::UserRole);
    if (data.isValid()) {
        int packetIndex = data.toInt();
        setCurrentPacket(packetIndex);
        emit packetSelected(packetIndex);
        qDebug() << "Packet selected:" << packetIndex;
    }
}

void PacketView::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);

    if (!item) {
        return;
    }

    QVariant data = item->data(0, Qt::UserRole);
    if (data.isValid()) {
        int packetIndex = data.toInt();
        emit packetDoubleClicked(packetIndex);
        qDebug() << "Packet double-clicked:" << packetIndex;
    }
}

} // namespace VideoStudio
