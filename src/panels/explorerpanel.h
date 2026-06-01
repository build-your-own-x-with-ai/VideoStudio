#ifndef EXPLORERPANEL_H
#define EXPLORERPANEL_H

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QMenu>
#include "core/tsparser.h"
#include "core/mp4parser.h"
#include "core/mkvparser.h"
#include "core/aviparser.h"
#include "core/flvparser.h"

namespace VideoStudio {

class ExplorerPanel : public QWidget {
    Q_OBJECT

public:
    explicit ExplorerPanel(QWidget* parent = nullptr);
    ~ExplorerPanel();

    // Set parser data
    void setTSParser(TSParser* parser);
    void setMP4Parser(MP4Parser* parser);
    void setMKVParser(MKVParser* parser);
    void setAVIParser(AVIParser* parser);
    void setFLVParser(FLVParser* parser);
    void clear();

signals:
    void pidSelected(uint16_t pid);
    void packetSelected(int64_t offset);
    void dumpElementaryStream(uint16_t pid);
    void setCompareMode(uint16_t pid);
    void setSyncMode(uint16_t pid);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onItemChanged(QTreeWidgetItem* item, int column);
    void onContextMenu(const QPoint& pos);
    void onExpandAll();
    void onCollapseAll();
    void onDumpES();
    void onCompareMode();
    void onSyncMode();

private:
    void buildTree();
    void addTransportStreamNode();
    void addMP4StreamNode();
    void addMP4AtomNodes(QTreeWidgetItem* parent, const QVector<MP4Atom>& atoms);
    void addMKVStreamNode();
    void addMKVElementNodes(QTreeWidgetItem* parent, const QVector<EBMLElement>& elements);
    void addAVIStreamNode();
    void addAVIChunkNodes(QTreeWidgetItem* parent, const QVector<AVIChunk>& chunks);
    void addFLVStreamNode();
    void addFLVTagNodes(QTreeWidgetItem* parent, const QVector<FLVTag>& tags);
    void addProgramNodes();
    void addPIDNodes();

    QTreeWidget* m_treeWidget;
    TSParser* m_tsParser;
    MP4Parser* m_mp4Parser;
    MKVParser* m_mkvParser;
    AVIParser* m_aviParser;
    FLVParser* m_flvParser;
    QTreeWidgetItem* m_contextMenuItem;
};

} // namespace VideoStudio

#endif // EXPLORERPANEL_H
