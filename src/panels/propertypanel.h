#ifndef PROPERTYPANEL_H
#define PROPERTYPANEL_H

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>
#include "core/tsparser.h"
#include "core/mp4parser.h"
#include "core/mkvparser.h"
#include "core/aviparser.h"
#include "core/flvparser.h"

namespace VideoStudio {

enum class PropertyMode {
    Sync,       // Auto-update when clicking packets/atoms
    Compare,    // Compare with previous packet/atom
    Dump        // Dump elementary stream or atom data
};

class PropertyPanel : public QWidget {
    Q_OBJECT

public:
    explicit PropertyPanel(QWidget* parent = nullptr);
    ~PropertyPanel();

    // Set parser data
    void setTSParser(TSParser* parser);
    void setMP4Parser(MP4Parser* parser);
    void setMKVParser(MKVParser* parser);
    void setAVIParser(AVIParser* parser);
    void setFLVParser(FLVParser* parser);

    // Display packet/atom/element/chunk/tag properties
    void displayPacket(int packetIndex);
    void displayAtom(int64_t offset);
    void displayElement(int64_t offset);
    void displayChunk(int64_t offset);
    void displayTag(int64_t offset);

    // Set mode
    void setMode(PropertyMode mode);
    PropertyMode getMode() const { return m_mode; }

public slots:
    void onPIDSelected(uint16_t pid);
    void onAtomSelected(int64_t offset);
    void onElementSelected(int64_t offset);
    void onChunkSelected(int64_t offset);
    void onTagSelected(int64_t offset);

signals:
    void addToGraphics(const QString& parameterName, const QVector<double>& values, const QVector<int64_t>& offsets);

private slots:
    void onContextMenu(const QPoint& pos);
    void onAddToGraphics();
    void onDumpData();

private:
    void createToolbar();
    void displayPacketSync(int packetIndex);
    void displayPacketCompare(int packetIndex);
    void displayAtomSync(const MP4Atom* atom);
    void displayAtomCompare(const MP4Atom* atom);
    void displayElementSync(const EBMLElement* element);
    void displayElementCompare(const EBMLElement* element);
    void displayChunkSync(const AVIChunk* chunk);
    void displayChunkCompare(const AVIChunk* chunk);
    void displayTagSync(const FLVTag* tag);
    void displayTagCompare(const FLVTag* tag);
    void addPacketFields(QTreeWidgetItem* parent, const TSPacket& packet);
    void addPSITableFields(QTreeWidgetItem* parent, const TSPacket& packet);
    void addAtomFields(QTreeWidgetItem* parent, const MP4Atom& atom);
    void addElementFields(QTreeWidgetItem* parent, const EBMLElement& element);
    void addChunkFields(QTreeWidgetItem* parent, const AVIChunk& chunk);
    void addTagFields(QTreeWidgetItem* parent, const FLVTag& tag);
    const MP4Atom* findAtomByOffset(const QVector<MP4Atom>& atoms, int64_t offset);
    const EBMLElement* findElementByOffset(const QVector<EBMLElement>& elements, int64_t offset);
    const AVIChunk* findChunkByOffset(const QVector<AVIChunk>& chunks, int64_t offset);
    const FLVTag* findTagByOffset(const QVector<FLVTag>& tags, int64_t offset);
    QString formatTimestamp(int64_t timestamp);
    void addCompareRow(QTreeWidgetItem* parent, const QString& property, const QString& prevValue, const QString& currValue);

    QTreeWidget* m_treeWidget;
    QToolBar* m_toolbar;
    TSParser* m_tsParser;
    MP4Parser* m_mp4Parser;
    MKVParser* m_mkvParser;
    AVIParser* m_aviParser;
    FLVParser* m_flvParser;
    PropertyMode m_mode;
    int m_lastPacketIndex;
    int64_t m_lastAtomOffset;
    int64_t m_lastElementOffset;
    int64_t m_lastChunkOffset;
    int64_t m_lastTagOffset;
    uint16_t m_selectedPID;

    QAction* m_syncAction;
    QAction* m_compareAction;
    QAction* m_dumpAction;
};

} // namespace VideoStudio

#endif // PROPERTYPANEL_H
