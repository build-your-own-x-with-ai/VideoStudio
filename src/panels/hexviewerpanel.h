#ifndef HEXVIEWERPANEL_H
#define HEXVIEWERPANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include "core/tsparser.h"
#include "core/mp4parser.h"
#include "core/mkvparser.h"
#include "core/aviparser.h"
#include "core/flvparser.h"

namespace VideoStudio {

class HexViewerPanel : public QWidget {
    Q_OBJECT

public:
    explicit HexViewerPanel(QWidget* parent = nullptr);
    ~HexViewerPanel();

    // Set parser data
    void setTSParser(TSParser* parser);
    void setMP4Parser(MP4Parser* parser);
    void setMKVParser(MKVParser* parser);
    void setAVIParser(AVIParser* parser);
    void setFLVParser(FLVParser* parser);

    // Display packet/atom/element/chunk/tag at offset
    void displayPacket(int packetIndex);
    void displayAtom(int64_t offset, int64_t size);
    void displayElement(int64_t offset, int64_t size);
    void displayChunk(int64_t offset, int64_t size);
    void displayTag(int64_t offset, int64_t size);

    // Jump to offset
    void jumpToOffset(int64_t offset);

public slots:
    void onPIDSelected(uint16_t pid);

private slots:
    void onBytesPerRowChanged(int value);
    void onOffsetSearchClicked();
    void onSearchTextChanged();
    void onDisplayModeChanged(int index);

private:
    void createToolbar();
    void updateDisplay();
    QString formatHexLine(int64_t offset, const QByteArray& data, int bytesPerRow);
    QString formatAscii(const QByteArray& data);
    void highlightCurrentPacket();
    void highlightSearchPattern(const QByteArray& pattern);

    QTextEdit* m_textEdit;
    QToolBar* m_toolbar;
    QSpinBox* m_bytesPerRowSpinBox;
    QLineEdit* m_offsetSearchEdit;
    QLineEdit* m_textSearchEdit;
    QComboBox* m_displayModeCombo;

    TSParser* m_tsParser;
    MP4Parser* m_mp4Parser;
    MKVParser* m_mkvParser;
    AVIParser* m_aviParser;
    FLVParser* m_flvParser;
    QString m_filePath;
    int m_currentPacketIndex;
    int64_t m_currentOffset;
    int64_t m_currentSize;
    int m_bytesPerRow;
    bool m_binaryMode;
};

} // namespace VideoStudio

#endif // HEXVIEWERPANEL_H
