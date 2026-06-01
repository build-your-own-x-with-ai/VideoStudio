#include "dialogs/savestreaminfodialog.h"
#include "core/tsparser.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QSet>

namespace VideoStudio {

SaveStreamInfoDialog::SaveStreamInfoDialog(TSParser* parser, QWidget* parent)
    : QDialog(parent)
    , m_parser(parser)
{
    setWindowTitle("Save Stream Information");
    resize(600, 500);

    createUI();
}

SaveStreamInfoDialog::~SaveStreamInfoDialog() {
}

void SaveStreamInfoDialog::createUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    createStreamInfoTab();
    createDumpTab();

    // Close button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton* closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);
}

void SaveStreamInfoDialog::createStreamInfoTab() {
    QWidget* streamInfoTab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(streamInfoTab);

    // Stream structure group
    QGroupBox* structureGroup = new QGroupBox("Stream Structure", streamInfoTab);
    QVBoxLayout* structureLayout = new QVBoxLayout(structureGroup);

    m_streamStructureCheck = new QCheckBox("Stream structure (Explorer panel)", structureGroup);
    m_streamStructureCheck->setChecked(true);
    structureLayout->addWidget(m_streamStructureCheck);

    m_fullStreamInfoCheck = new QCheckBox("Full stream information (checked elements only)", structureGroup);
    structureLayout->addWidget(m_fullStreamInfoCheck);

    layout->addWidget(structureGroup);

    // Range group
    QGroupBox* rangeGroup = new QGroupBox("Range", streamInfoTab);
    QVBoxLayout* rangeLayout = new QVBoxLayout(rangeGroup);

    m_rangeAllRadio = new QRadioButton("All", rangeGroup);
    m_rangeAllRadio->setChecked(true);
    rangeLayout->addWidget(m_rangeAllRadio);

    QHBoxLayout* offsetLayout = new QHBoxLayout();
    m_rangeOffsetRadio = new QRadioButton("Offset:", rangeGroup);
    offsetLayout->addWidget(m_rangeOffsetRadio);
    m_offsetStartEdit = new QLineEdit(rangeGroup);
    m_offsetStartEdit->setPlaceholderText("Start");
    m_offsetStartEdit->setEnabled(false);
    offsetLayout->addWidget(m_offsetStartEdit);
    offsetLayout->addWidget(new QLabel("-", rangeGroup));
    m_offsetEndEdit = new QLineEdit(rangeGroup);
    m_offsetEndEdit->setPlaceholderText("End");
    m_offsetEndEdit->setEnabled(false);
    offsetLayout->addWidget(m_offsetEndEdit);
    rangeLayout->addLayout(offsetLayout);

    connect(m_rangeOffsetRadio, &QRadioButton::toggled, this, [this](bool checked) {
        m_offsetStartEdit->setEnabled(checked);
        m_offsetEndEdit->setEnabled(checked);
    });

    layout->addWidget(rangeGroup);

    // Messages group
    QGroupBox* messagesGroup = new QGroupBox("Messages", streamInfoTab);
    QVBoxLayout* messagesLayout = new QVBoxLayout(messagesGroup);

    m_messagesCheck = new QCheckBox("Messages (general info)", messagesGroup);
    messagesLayout->addWidget(m_messagesCheck);

    m_messageDetailsCheck = new QCheckBox("Message details (errors with descriptions)", messagesGroup);
    messagesLayout->addWidget(m_messageDetailsCheck);

    layout->addWidget(messagesGroup);

    // TR 101-290 group
    QGroupBox* tr101290Group = new QGroupBox("TR 101-290", streamInfoTab);
    QVBoxLayout* tr101290Layout = new QVBoxLayout(tr101290Group);

    m_tr101290Check = new QCheckBox("TR 101-290 (error types)", tr101290Group);
    tr101290Layout->addWidget(m_tr101290Check);

    m_tr101290DetailsCheck = new QCheckBox("TR 101-290 details (errors with descriptions)", tr101290Group);
    tr101290Layout->addWidget(m_tr101290DetailsCheck);

    layout->addWidget(tr101290Group);

    // Headers group
    QGroupBox* headersGroup = new QGroupBox("Headers", streamInfoTab);
    QVBoxLayout* headersLayout = new QVBoxLayout(headersGroup);

    m_headersCheck = new QCheckBox("Headers", headersGroup);
    headersLayout->addWidget(m_headersCheck);

    m_visibleOnlyCheck = new QCheckBox("Visible only", headersGroup);
    headersLayout->addWidget(m_visibleOnlyCheck);

    m_headerDetailsCheck = new QCheckBox("Headers details", headersGroup);
    headersLayout->addWidget(m_headerDetailsCheck);

    layout->addWidget(headersGroup);

    // File path
    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(new QLabel("File:", streamInfoTab));
    m_streamInfoPathEdit = new QLineEdit(streamInfoTab);
    pathLayout->addWidget(m_streamInfoPathEdit);
    m_streamInfoBrowseButton = new QPushButton("Browse...", streamInfoTab);
    connect(m_streamInfoBrowseButton, &QPushButton::clicked, this, &SaveStreamInfoDialog::onBrowseStreamInfo);
    pathLayout->addWidget(m_streamInfoBrowseButton);
    layout->addLayout(pathLayout);

    // Save button
    QHBoxLayout* saveLayout = new QHBoxLayout();
    saveLayout->addStretch();
    m_streamInfoSaveButton = new QPushButton("Save", streamInfoTab);
    connect(m_streamInfoSaveButton, &QPushButton::clicked, this, &SaveStreamInfoDialog::onSaveStreamInfo);
    saveLayout->addWidget(m_streamInfoSaveButton);
    layout->addLayout(saveLayout);

    layout->addStretch();

    m_tabWidget->addTab(streamInfoTab, "Stream Info");
}

void SaveStreamInfoDialog::createDumpTab() {
    QWidget* dumpTab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(dumpTab);

    // Transport stream selection
    QGroupBox* streamGroup = new QGroupBox("Transport Stream", dumpTab);
    QVBoxLayout* streamLayout = new QVBoxLayout(streamGroup);

    streamLayout->addWidget(new QLabel("Select elementary stream to dump:", streamGroup));
    m_dumpStreamCombo = new QComboBox(streamGroup);

    // Populate with elementary streams from parser
    if (m_parser) {
        const auto& programs = m_parser->getPrograms();
        const auto& pids = m_parser->getPIDs();

        for (const auto& program : programs) {
            for (uint16_t esPid : program.elementaryPIDs) {
                if (pids.contains(esPid)) {
                    const auto& pidInfo = pids[esPid];
                    QString label = QString("PID 0x%1 - %2 (%3)")
                        .arg(esPid, 4, 16, QChar('0'))
                        .arg(pidInfo.type)
                        .arg(pidInfo.codec.isEmpty() ? "Unknown" : pidInfo.codec);
                    m_dumpStreamCombo->addItem(label, esPid);
                }
            }
        }
    }

    streamLayout->addWidget(m_dumpStreamCombo);
    layout->addWidget(streamGroup);

    // Range group
    QGroupBox* rangeGroup = new QGroupBox("Range", dumpTab);
    QVBoxLayout* rangeLayout = new QVBoxLayout(rangeGroup);

    m_dumpRangeAllRadio = new QRadioButton("All", rangeGroup);
    m_dumpRangeAllRadio->setChecked(true);
    rangeLayout->addWidget(m_dumpRangeAllRadio);

    QHBoxLayout* offsetLayout = new QHBoxLayout();
    m_dumpRangeOffsetRadio = new QRadioButton("Offset:", rangeGroup);
    offsetLayout->addWidget(m_dumpRangeOffsetRadio);
    m_dumpOffsetStartEdit = new QLineEdit(rangeGroup);
    m_dumpOffsetStartEdit->setPlaceholderText("Start");
    m_dumpOffsetStartEdit->setEnabled(false);
    offsetLayout->addWidget(m_dumpOffsetStartEdit);
    offsetLayout->addWidget(new QLabel("-", rangeGroup));
    m_dumpOffsetEndEdit = new QLineEdit(rangeGroup);
    m_dumpOffsetEndEdit->setPlaceholderText("End");
    m_dumpOffsetEndEdit->setEnabled(false);
    offsetLayout->addWidget(m_dumpOffsetEndEdit);
    rangeLayout->addLayout(offsetLayout);

    connect(m_dumpRangeOffsetRadio, &QRadioButton::toggled, this, [this](bool checked) {
        m_dumpOffsetStartEdit->setEnabled(checked);
        m_dumpOffsetEndEdit->setEnabled(checked);
    });

    layout->addWidget(rangeGroup);

    // File path
    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(new QLabel("File:", dumpTab));
    m_dumpPathEdit = new QLineEdit(dumpTab);
    pathLayout->addWidget(m_dumpPathEdit);
    m_dumpBrowseButton = new QPushButton("Browse...", dumpTab);
    connect(m_dumpBrowseButton, &QPushButton::clicked, this, &SaveStreamInfoDialog::onBrowseDump);
    pathLayout->addWidget(m_dumpBrowseButton);
    layout->addLayout(pathLayout);

    // Save button
    QHBoxLayout* saveLayout = new QHBoxLayout();
    saveLayout->addStretch();
    m_dumpSaveButton = new QPushButton("Save", dumpTab);
    connect(m_dumpSaveButton, &QPushButton::clicked, this, &SaveStreamInfoDialog::onSaveDump);
    saveLayout->addWidget(m_dumpSaveButton);
    layout->addLayout(saveLayout);

    layout->addStretch();

    m_tabWidget->addTab(dumpTab, "Dump");
}

void SaveStreamInfoDialog::onBrowseStreamInfo() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Stream Information"), QString(),
        tr("Text Files (*.txt);;CSV Files (*.csv);;All Files (*)"));

    if (!fileName.isEmpty()) {
        m_streamInfoPathEdit->setText(fileName);
    }
}

void SaveStreamInfoDialog::onBrowseDump() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Elementary Stream"), QString(),
        tr("Elementary Stream (*.es *.264 *.265 *.h264 *.h265);;All Files (*)"));

    if (!fileName.isEmpty()) {
        m_dumpPathEdit->setText(fileName);
    }
}

void SaveStreamInfoDialog::onSaveStreamInfo() {
    QString filePath = m_streamInfoPathEdit->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please specify output file path");
        return;
    }

    if (!m_parser) {
        QMessageBox::warning(this, "Error", "No TS parser available");
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Failed to open file for writing");
        return;
    }

    QTextStream out(&file);

    // Write stream structure
    if (m_streamStructureCheck->isChecked()) {
        out << "=== Stream Structure ===\n\n";
        out << "File: " << m_parser->getFilePath() << "\n";
        out << "File Size: " << m_parser->getFileSize() << " bytes\n";
        out << "Total Packets: " << m_parser->getTotalPackets() << "\n\n";

        const auto& programs = m_parser->getPrograms();
        out << "Programs: " << programs.size() << "\n";
        for (const auto& program : programs) {
            out << "  Program " << program.programNumber;
            if (!program.serviceName.isEmpty()) {
                out << " (" << program.serviceName << ")";
            }
            out << "\n";
            out << "    PMT PID: 0x" << QString::number(program.pmtPid, 16).toUpper() << "\n";

            const auto& pids = m_parser->getPIDs();
            QSet<uint16_t> uniquePids;  // Use set to avoid duplicates
            for (uint16_t esPid : program.elementaryPIDs) {
                uniquePids.insert(esPid);
            }

            for (uint16_t esPid : uniquePids) {
                if (pids.contains(esPid)) {
                    const auto& pidInfo = pids[esPid];
                    out << "    Elementary Stream: PID 0x" << QString::number(esPid, 16).toUpper()
                        << " - " << pidInfo.type;
                    if (!pidInfo.codec.isEmpty()) {
                        out << " (" << pidInfo.codec << ")";
                    }
                    out << "\n";
                }
            }
        }
        out << "\n";
    }

    // Write PID information
    if (m_fullStreamInfoCheck->isChecked()) {
        out << "=== PID Information ===\n\n";
        const auto& pids = m_parser->getPIDs();
        for (const auto& pidInfo : pids) {
            out << "PID 0x" << QString::number(pidInfo.pid, 16).toUpper()
                << " - " << pidInfo.type
                << " - " << pidInfo.packetCount << " packets"
                << " (" << QString::number(pidInfo.percentage, 'f', 2) << "%)\n";
        }
        out << "\n";
    }

    file.close();

    QMessageBox::information(this, "Success", "Stream information saved successfully");
}

void SaveStreamInfoDialog::onSaveDump() {
    QString filePath = m_dumpPathEdit->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please specify output file path");
        return;
    }

    if (!m_parser) {
        QMessageBox::warning(this, "Error", "No TS parser available");
        return;
    }

    if (m_dumpStreamCombo->count() == 0) {
        QMessageBox::warning(this, "Error", "No elementary streams available");
        return;
    }

    uint16_t selectedPid = m_dumpStreamCombo->currentData().toUInt();

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error", "Failed to open file for writing");
        return;
    }

    // Extract elementary stream data
    const auto& packets = m_parser->getPackets();
    int64_t startOffset = 0;
    int64_t endOffset = m_parser->getFileSize();

    if (m_dumpRangeOffsetRadio->isChecked()) {
        bool ok;
        startOffset = m_dumpOffsetStartEdit->text().toLongLong(&ok);
        if (!ok) startOffset = 0;
        endOffset = m_dumpOffsetEndEdit->text().toLongLong(&ok);
        if (!ok) endOffset = m_parser->getFileSize();
    }

    int packetCount = 0;
    for (const auto& packet : packets) {
        if (packet.pid == selectedPid &&
            packet.offset >= startOffset &&
            packet.offset <= endOffset) {

            // Write payload data
            if (!packet.payload.isEmpty()) {
                file.write(packet.payload);
                packetCount++;
            }
        }
    }

    file.close();

    QMessageBox::information(this, "Success",
        QString("Elementary stream saved successfully\n%1 packets written").arg(packetCount));
}

} // namespace VideoStudio
