#include "panels/hexviewerpanel.h"
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QDebug>

namespace VideoStudio {

HexViewerPanel::HexViewerPanel(QWidget* parent)
    : QWidget(parent)
    , m_tsParser(nullptr)
    , m_mp4Parser(nullptr)
    , m_mkvParser(nullptr)
    , m_aviParser(nullptr)
    , m_flvParser(nullptr)
    , m_nalUnitParser(nullptr)
    , m_currentPacketIndex(-1)
    , m_currentOffset(-1)
    , m_currentSize(0)
    , m_bytesPerRow(16)
    , m_binaryMode(false)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    createToolbar();
    layout->addWidget(m_toolbar);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setFont(QFont("Courier", 10));
    m_textEdit->setLineWrapMode(QTextEdit::NoWrap);
    layout->addWidget(m_textEdit);
}

HexViewerPanel::~HexViewerPanel() {
}

void HexViewerPanel::createToolbar() {
    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(16, 16));

    // Display mode
    m_toolbar->addWidget(new QLabel("Mode:", m_toolbar));
    m_displayModeCombo = new QComboBox(m_toolbar);
    m_displayModeCombo->addItem("Hex");
    m_displayModeCombo->addItem("Binary");
    m_displayModeCombo->setCurrentIndex(0);
    connect(m_displayModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HexViewerPanel::onDisplayModeChanged);
    m_toolbar->addWidget(m_displayModeCombo);

    m_toolbar->addSeparator();

    // Bytes per row
    m_toolbar->addWidget(new QLabel("Bytes/Row:", m_toolbar));
    m_bytesPerRowSpinBox = new QSpinBox(m_toolbar);
    m_bytesPerRowSpinBox->setRange(8, 32);
    m_bytesPerRowSpinBox->setValue(16);
    m_bytesPerRowSpinBox->setSingleStep(8);
    connect(m_bytesPerRowSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &HexViewerPanel::onBytesPerRowChanged);
    m_toolbar->addWidget(m_bytesPerRowSpinBox);

    m_toolbar->addSeparator();

    // Offset search
    m_toolbar->addWidget(new QLabel("Offset:", m_toolbar));
    m_offsetSearchEdit = new QLineEdit(m_toolbar);
    m_offsetSearchEdit->setPlaceholderText("0x0000");
    m_offsetSearchEdit->setMaximumWidth(100);
    m_toolbar->addWidget(m_offsetSearchEdit);

    QPushButton* offsetSearchBtn = new QPushButton("Go", m_toolbar);
    connect(offsetSearchBtn, &QPushButton::clicked,
            this, &HexViewerPanel::onOffsetSearchClicked);
    m_toolbar->addWidget(offsetSearchBtn);

    m_toolbar->addSeparator();

    // Text search
    m_toolbar->addWidget(new QLabel("Search:", m_toolbar));
    m_textSearchEdit = new QLineEdit(m_toolbar);
    m_textSearchEdit->setPlaceholderText("Hex bytes (e.g., 47 40)");
    m_textSearchEdit->setMaximumWidth(150);
    connect(m_textSearchEdit, &QLineEdit::textChanged,
            this, &HexViewerPanel::onSearchTextChanged);
    m_toolbar->addWidget(m_textSearchEdit);
}

void HexViewerPanel::setTSParser(TSParser* parser) {
    m_tsParser = parser;
    m_mp4Parser = nullptr;
    m_mkvParser = nullptr;
    m_aviParser = nullptr;
    m_flvParser = nullptr;
    m_filePath = parser ? parser->getFilePath() : QString();
    m_textEdit->clear();
    m_currentPacketIndex = -1;
    m_currentOffset = -1;
    m_currentSize = 0;
}

void HexViewerPanel::setMP4Parser(MP4Parser* parser) {
    m_mp4Parser = parser;
    m_tsParser = nullptr;
    m_mkvParser = nullptr;
    m_aviParser = nullptr;
    m_flvParser = nullptr;
    m_filePath = parser ? parser->getFilePath() : QString();
    m_textEdit->clear();
    m_currentPacketIndex = -1;
    m_currentOffset = -1;
    m_currentSize = 0;
}

void HexViewerPanel::setMKVParser(MKVParser* parser) {
    m_mkvParser = parser;
    m_tsParser = nullptr;
    m_mp4Parser = nullptr;
    m_aviParser = nullptr;
    m_flvParser = nullptr;
    m_filePath = parser ? parser->getFilePath() : QString();
    m_textEdit->clear();
    m_currentPacketIndex = -1;
    m_currentOffset = -1;
    m_currentSize = 0;
}

void HexViewerPanel::setAVIParser(AVIParser* parser) {
    m_aviParser = parser;
    m_tsParser = nullptr;
    m_mp4Parser = nullptr;
    m_mkvParser = nullptr;
    m_flvParser = nullptr;
    m_filePath = parser ? parser->getFilePath() : QString();
    m_textEdit->clear();
    m_currentPacketIndex = -1;
    m_currentOffset = -1;
    m_currentSize = 0;
}

void HexViewerPanel::setFLVParser(FLVParser* parser) {
    m_flvParser = parser;
    m_tsParser = nullptr;
    m_mp4Parser = nullptr;
    m_mkvParser = nullptr;
    m_aviParser = nullptr;
    m_filePath = parser ? parser->getFilePath() : QString();
    m_textEdit->clear();
    m_currentPacketIndex = -1;
    m_currentOffset = -1;
    m_currentSize = 0;
}

void HexViewerPanel::setNALUnitParser(NALUnitParser* parser) {
    m_nalUnitParser = parser;
    if (parser) {
        m_filePath = parser->getFilePath();
    }
}

void HexViewerPanel::displayPacket(int packetIndex) {
    if (!m_tsParser || packetIndex < 0) {
        return;
    }

    const auto& packets = m_tsParser->getPackets();
    if (packetIndex >= packets.size()) {
        return;
    }

    m_currentPacketIndex = packetIndex;
    updateDisplay();
}

void HexViewerPanel::displayNALUnit(int nalIndex) {
    qDebug() << "HexViewerPanel::displayNALUnit called with nalIndex:" << nalIndex;

    if (!m_nalUnitParser || nalIndex < 0) {
        qDebug() << "HexViewerPanel::displayNALUnit: parser is null or invalid index";
        return;
    }

    const NALUnitInfo* nalInfo = m_nalUnitParser->getNALUnit(nalIndex);
    if (!nalInfo) {
        qDebug() << "HexViewerPanel::displayNALUnit: Failed to get NAL info for index" << nalIndex;
        return;
    }

    qDebug() << "HexViewerPanel::displayNALUnit: NAL info - type:" << nalInfo->typeName
             << "offset:" << nalInfo->fileOffset << "size:" << nalInfo->size;
    qDebug() << "HexViewerPanel::displayNALUnit: File path:" << m_filePath;

    // Read NAL unit data from file
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "HexViewerPanel::displayNALUnit: Failed to open file" << m_filePath;
        return;
    }

    if (!file.seek(nalInfo->fileOffset)) {
        qDebug() << "HexViewerPanel::displayNALUnit: Failed to seek to offset" << nalInfo->fileOffset;
        file.close();
        return;
    }

    QByteArray nalData = file.read(nalInfo->size);
    file.close();

    if (nalData.size() != nalInfo->size) {
        qDebug() << "HexViewerPanel::displayNALUnit: Read size mismatch, expected" << nalInfo->size << "got" << nalData.size();
        return;
    }

    // Display the NAL unit data
    m_currentOffset = nalInfo->fileOffset;
    m_currentSize = nalInfo->size;
    m_textEdit->clear();

    QString output;
    output += QString("=== NAL Unit #%1 ===\n").arg(nalIndex);
    output += QString("Type: %1\n").arg(nalInfo->typeName);
    output += QString("Offset: 0x%1\n").arg(nalInfo->fileOffset, 0, 16);
    output += QString("Size: %1 bytes\n").arg(nalInfo->size);
    output += QString("Frame: %1\n").arg(nalInfo->frameNumber);
    output += "\n";

    // Display hex dump
    for (int i = 0; i < nalData.size(); i += m_bytesPerRow) {
        int chunkSize = qMin(m_bytesPerRow, nalData.size() - i);
        QByteArray chunk = nalData.mid(i, chunkSize);
        output += formatHexLine(nalInfo->fileOffset + i, chunk, m_bytesPerRow);
    }

    m_textEdit->setPlainText(output);
}

void HexViewerPanel::displayAudioFrame(int audioIndex) {
    qDebug() << "HexViewerPanel::displayAudioFrame called with audioIndex:" << audioIndex;

    if (!m_nalUnitParser || audioIndex < 0) {
        qDebug() << "HexViewerPanel::displayAudioFrame: parser is null or invalid index";
        return;
    }

    const AudioFrameInfo* audioInfo = m_nalUnitParser->getAudioFrame(audioIndex);
    if (!audioInfo) {
        qDebug() << "HexViewerPanel::displayAudioFrame: Failed to get audio frame info for index" << audioIndex;
        return;
    }

    qDebug() << "HexViewerPanel::displayAudioFrame: Audio frame - type:" << audioInfo->frameName
             << "offset:" << audioInfo->fileOffset << "size:" << audioInfo->size;

    // Read audio frame data from file
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "HexViewerPanel::displayAudioFrame: Failed to open file" << m_filePath;
        return;
    }

    if (!file.seek(audioInfo->fileOffset)) {
        qDebug() << "HexViewerPanel::displayAudioFrame: Failed to seek to offset" << audioInfo->fileOffset;
        file.close();
        return;
    }

    QByteArray audioData = file.read(audioInfo->size);
    file.close();

    if (audioData.size() != audioInfo->size) {
        qDebug() << "HexViewerPanel::displayAudioFrame: Read size mismatch, expected" << audioInfo->size << "got" << audioData.size();
        return;
    }

    // Display the audio frame data
    m_currentOffset = audioInfo->fileOffset;
    m_currentSize = audioInfo->size;
    m_textEdit->clear();

    QString output;
    output += QString("=== Audio Frame #%1 ===\n").arg(audioIndex);
    output += QString("Type: %1\n").arg(audioInfo->frameName);
    output += QString("Codec: %1\n").arg(audioInfo->codecType);
    output += QString("Offset: 0x%1\n").arg(audioInfo->fileOffset, 0, 16);
    output += QString("Size: %1 bytes\n").arg(audioInfo->size);
    output += QString("Frame: %1\n").arg(audioInfo->frameNumber);
    output += "\n";

    // Display hex dump
    for (int i = 0; i < audioData.size(); i += m_bytesPerRow) {
        int chunkSize = qMin(m_bytesPerRow, audioData.size() - i);
        QByteArray chunk = audioData.mid(i, chunkSize);
        output += formatHexLine(audioInfo->fileOffset + i, chunk, m_bytesPerRow);
    }

    m_textEdit->setPlainText(output);
}

void HexViewerPanel::jumpToOffset(int64_t offset) {
    if (!m_tsParser) {
        return;
    }

    // Find packet at this offset
    const auto& packets = m_tsParser->getPackets();
    for (int i = 0; i < packets.size(); ++i) {
        if (packets[i].offset == offset) {
            displayPacket(i);
            return;
        }
    }
}

void HexViewerPanel::onPIDSelected(uint16_t pid) {
    if (!m_tsParser) {
        return;
    }

    // Find first packet with this PID
    const auto& packets = m_tsParser->getPackets();
    for (int i = 0; i < packets.size(); ++i) {
        if (packets[i].pid == pid) {
            displayPacket(i);
            return;
        }
    }
}

void HexViewerPanel::onBytesPerRowChanged(int value) {
    m_bytesPerRow = value;
    if (m_currentPacketIndex >= 0 || (m_currentOffset >= 0 && m_currentSize > 0)) {
        updateDisplay();
    }
}

void HexViewerPanel::onOffsetSearchClicked() {
    QString offsetText = m_offsetSearchEdit->text();
    if (offsetText.isEmpty()) {
        return;
    }

    bool ok;
    int64_t offset = offsetText.toLongLong(&ok, 0); // Auto-detect base (0x for hex)
    if (ok) {
        jumpToOffset(offset);
    }
}

void HexViewerPanel::onSearchTextChanged() {
    QString searchText = m_textSearchEdit->text().trimmed();

    // Always update display first
    if (m_currentPacketIndex >= 0 || (m_currentOffset >= 0 && m_currentSize > 0)) {
        updateDisplay();
    }

    if (searchText.isEmpty()) {
        return;
    }

    // Remove "0x" prefix if present
    searchText = searchText.replace("0x", "", Qt::CaseInsensitive);
    searchText = searchText.replace(" ", "");

    // Parse hex bytes from search text (e.g., "47" or "4740")
    if (searchText.length() % 2 != 0) {
        // Odd number of hex digits, invalid
        return;
    }

    QByteArray searchPattern;
    for (int i = 0; i < searchText.length(); i += 2) {
        QString hexByte = searchText.mid(i, 2);
        bool ok;
        uint8_t byte = hexByte.toUInt(&ok, 16);
        if (ok) {
            searchPattern.append(byte);
        } else {
            // Invalid hex input
            return;
        }
    }

    if (searchPattern.isEmpty()) {
        return;
    }

    // Highlight matches in current display
    highlightSearchPattern(searchPattern);
}

void HexViewerPanel::onDisplayModeChanged(int index) {
    m_binaryMode = (index == 1);
    if (m_currentPacketIndex >= 0 || (m_currentOffset >= 0 && m_currentSize > 0)) {
        updateDisplay();
    }
}

void HexViewerPanel::updateDisplay() {
    if (m_tsParser && m_currentPacketIndex >= 0) {
        const auto& packets = m_tsParser->getPackets();
        if (m_currentPacketIndex >= packets.size()) {
            return;
        }

        const TSPacket& packet = packets[m_currentPacketIndex];

        // Build full packet data (188 bytes)
        QByteArray packetData;
        packetData.append(packet.syncByte);

    // Transport packet header (4 bytes total including sync)
    uint8_t byte1 = (packet.transportErrorIndicator ? 0x80 : 0) |
                    (packet.payloadUnitStartIndicator ? 0x40 : 0) |
                    (packet.transportPriority ? 0x20 : 0) |
                    ((packet.pid >> 8) & 0x1F);
    uint8_t byte2 = packet.pid & 0xFF;
    uint8_t byte3 = (packet.scramblingControl << 6) |
                    (packet.adaptationFieldControl << 4) |
                    packet.continuityCounter;

    packetData.append(byte1);
    packetData.append(byte2);
    packetData.append(byte3);

    // Add payload (remaining bytes to make 188 total)
    packetData.append(packet.payload);

    // Pad to 188 bytes if needed
    while (packetData.size() < 188) {
        packetData.append((char)0xFF);
    }

    // Format display
    QString text;
    text += QString("=== Packet #%1 at offset 0x%2 ===\n\n")
        .arg(m_currentPacketIndex)
        .arg(packet.offset, 0, 16);

    int64_t currentOffset = packet.offset;
    for (int i = 0; i < packetData.size(); i += m_bytesPerRow) {
        int bytesToShow = qMin(m_bytesPerRow, packetData.size() - i);
        QByteArray lineData = packetData.mid(i, bytesToShow);
        text += formatHexLine(currentOffset, lineData, m_bytesPerRow);
        currentOffset += bytesToShow;
    }

    m_textEdit->setPlainText(text);
    } else if ((m_mp4Parser || m_mkvParser || m_aviParser || m_flvParser) && m_currentOffset >= 0 && m_currentSize > 0) {
        // Display MP4 atom, MKV element, AVI chunk, or FLV tag data
        QFile file(m_filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            m_textEdit->setPlainText("Error: Cannot open file");
            return;
        }

        file.seek(m_currentOffset);
        QByteArray data = file.read(m_currentSize);
        file.close();

        // Format display
        QString text;
        text += QString("=== Data at offset 0x%1 (size: %2 bytes) ===\n\n")
            .arg(m_currentOffset, 0, 16)
            .arg(m_currentSize);

        int64_t currentOffset = m_currentOffset;
        for (int i = 0; i < data.size(); i += m_bytesPerRow) {
            int bytesToShow = qMin(m_bytesPerRow, data.size() - i);
            QByteArray lineData = data.mid(i, bytesToShow);
            text += formatHexLine(currentOffset, lineData, m_bytesPerRow);
            currentOffset += bytesToShow;
        }

        m_textEdit->setPlainText(text);
    }
}

QString HexViewerPanel::formatHexLine(int64_t offset, const QByteArray& data, int bytesPerRow) {
    QString line;

    // Offset column
    line += QString("%1  ").arg(offset, 8, 16, QChar('0'));

    if (m_binaryMode) {
        // Binary mode
        for (int i = 0; i < bytesPerRow; ++i) {
            if (i < data.size()) {
                uint8_t byte = static_cast<uint8_t>(data[i]);
                line += QString("%1 ").arg(byte, 8, 2, QChar('0'));
            } else {
                line += "         ";
            }
            if ((i + 1) % 4 == 0) {
                line += " ";
            }
        }
    } else {
        // Hex mode
        for (int i = 0; i < bytesPerRow; ++i) {
            if (i < data.size()) {
                uint8_t byte = static_cast<uint8_t>(data[i]);
                line += QString("%1 ").arg(byte, 2, 16, QChar('0'));
            } else {
                line += "   ";
            }
            if ((i + 1) % 8 == 0) {
                line += " ";
            }
        }
    }

    // ASCII column
    line += " | ";
    line += formatAscii(data);
    line += "\n";

    return line;
}

QString HexViewerPanel::formatAscii(const QByteArray& data) {
    QString ascii;
    for (int i = 0; i < data.size(); ++i) {
        uint8_t byte = static_cast<uint8_t>(data[i]);
        if (byte >= 32 && byte <= 126) {
            ascii += QChar(byte);
        } else {
            ascii += '.';
        }
    }
    return ascii;
}

void HexViewerPanel::highlightCurrentPacket() {
    // This method is called to highlight search results
    // The actual highlighting is done in highlightSearchPattern
}

void HexViewerPanel::highlightSearchPattern(const QByteArray& pattern) {
    if (pattern.isEmpty()) {
        return;
    }

    // Highlight matches by searching for the hex string in the text display
    QString searchHex = pattern.toHex(' ').toUpper();
    qDebug() << "Searching for hex string:" << searchHex;

    QTextDocument* doc = m_textEdit->document();
    QTextCursor highlightCursor(doc);
    QTextCharFormat highlightFormat;
    highlightFormat.setBackground(QColor(255, 255, 0)); // Yellow

    // Search and highlight
    while (!highlightCursor.isNull() && !highlightCursor.atEnd()) {
        highlightCursor = doc->find(searchHex, highlightCursor);
        if (!highlightCursor.isNull()) {
            highlightCursor.mergeCharFormat(highlightFormat);
        }
    }
}

void HexViewerPanel::displayAtom(int64_t offset, int64_t size) {
    m_currentOffset = offset;
    m_currentSize = size;
    m_currentPacketIndex = -1;
    updateDisplay();
}

void HexViewerPanel::displayElement(int64_t offset, int64_t size) {
    m_currentOffset = offset;
    m_currentSize = size;
    m_currentPacketIndex = -1;
    updateDisplay();
}

void HexViewerPanel::displayChunk(int64_t offset, int64_t size) {
    m_currentOffset = offset;
    m_currentSize = size;
    m_currentPacketIndex = -1;
    updateDisplay();
}

void HexViewerPanel::displayTag(int64_t offset, int64_t size) {
    m_currentOffset = offset;
    m_currentSize = size;
    m_currentPacketIndex = -1;
    updateDisplay();
}

} // namespace VideoStudio
