#ifndef PACKETVIEW_H
#define PACKETVIEW_H

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include "core/tsparser.h"

namespace VideoStudio {

class PacketView : public QWidget {
    Q_OBJECT

public:
    explicit PacketView(QWidget* parent = nullptr);
    ~PacketView();

    // Set TS parser data
    void setTSParser(TSParser* parser);

    // Clear view
    void clear();

    // Set current packet
    void setCurrentPacket(int packetIndex);

signals:
    void packetSelected(int packetIndex);
    void packetDoubleClicked(int packetIndex);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);

private:
    void buildPacketList();
    QString getPacketType(const TSPacket& packet);
    QColor getPacketColor(const TSPacket& packet);
    QString formatProperties(const TSPacket& packet);

    QTreeWidget* m_treeWidget;
    TSParser* m_parser;
    int m_currentPacketIndex;
};

} // namespace VideoStudio

#endif // PACKETVIEW_H
