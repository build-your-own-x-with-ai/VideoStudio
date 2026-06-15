#include "audioinfopanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>

namespace VideoStudio {

AudioInfoPanel::AudioInfoPanel(QWidget* parent)
    : QWidget(parent)
    , m_analyzer(std::make_unique<AudioAnalyzer>())
{
    setupUI();
}

AudioInfoPanel::~AudioInfoPanel() = default;

void AudioInfoPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Stream selection
    QGroupBox* streamGroup = new QGroupBox(tr("Audio Streams"), this);
    QVBoxLayout* streamLayout = new QVBoxLayout(streamGroup);

    m_streamCombo = new QComboBox(this);
    connect(m_streamCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AudioInfoPanel::onStreamSelected);
    streamLayout->addWidget(m_streamCombo);

    mainLayout->addWidget(streamGroup);

    // Audio stream information table
    QGroupBox* infoGroup = new QGroupBox(tr("Stream Information"), this);
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);

    m_infoTable = new QTableWidget(this);
    m_infoTable->setColumnCount(2);
    m_infoTable->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    m_infoTable->horizontalHeader()->setStretchLastSection(true);
    m_infoTable->verticalHeader()->setVisible(false);
    m_infoTable->setAlternatingRowColors(true);
    m_infoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_infoTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    infoLayout->addWidget(m_infoTable);
    mainLayout->addWidget(infoGroup);

    // Status label
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("QLabel { padding: 5px; }");
    mainLayout->addWidget(m_statusLabel);

    mainLayout->addStretch();
}

void AudioInfoPanel::setAudioFile(const QString& filename) {
    clear();
    m_currentFile = filename;

    if (filename.isEmpty()) {
        return;
    }

    // Open file and populate stream list
    if (m_analyzer->openFile(filename)) {
        populateStreamList();
        m_statusLabel->setText(tr("Audio file loaded successfully"));
    } else {
        m_statusLabel->setText(tr("Failed to load audio file"));
    }
}

void AudioInfoPanel::clear() {
    m_streamCombo->clear();
    m_infoTable->setRowCount(0);
    m_statusLabel->clear();
    m_currentFile.clear();
}

void AudioInfoPanel::populateStreamList() {
    m_streamCombo->clear();

    QVector<AudioStreamInfo> streams = m_analyzer->getAllAudioStreams();

    if (streams.isEmpty()) {
        m_statusLabel->setText(tr("No audio streams found"));
        return;
    }

    for (const AudioStreamInfo& info : streams) {
        QString label = QString("Stream #%1 - %2 (%3)")
            .arg(info.streamIndex)
            .arg(info.codecName)
            .arg(info.channelLayout);
        m_streamCombo->addItem(label, info.streamIndex);
    }

    // Select first stream
    if (m_streamCombo->count() > 0) {
        m_streamCombo->setCurrentIndex(0);
    }
}

void AudioInfoPanel::onStreamSelected(int index) {
    if (index < 0 || m_currentFile.isEmpty()) {
        return;
    }

    int streamIndex = m_streamCombo->itemData(index).toInt();

    // Reopen with selected stream
    if (m_analyzer->openFile(m_currentFile, streamIndex)) {
        AudioStreamInfo info = m_analyzer->getStreamInfo();
        displayStreamInfo(info);
    }
}

void AudioInfoPanel::displayStreamInfo(const AudioStreamInfo& info) {
    m_infoTable->setRowCount(0);

    auto addRow = [this](const QString& property, const QString& value) {
        int row = m_infoTable->rowCount();
        m_infoTable->insertRow(row);
        m_infoTable->setItem(row, 0, new QTableWidgetItem(property));
        m_infoTable->setItem(row, 1, new QTableWidgetItem(value));
    };

    // Stream index
    addRow(tr("Stream Index"), QString::number(info.streamIndex));

    // Codec
    addRow(tr("Codec"), QString("%1 (%2)")
        .arg(info.codecName)
        .arg(info.codecLongName));

    // Profile
    if (!info.profile.isEmpty()) {
        addRow(tr("Profile"), info.profile);
    }

    // Sample rate
    addRow(tr("Sample Rate"), QString("%1 Hz").arg(info.sampleRate));

    // Channels
    addRow(tr("Channels"), QString::number(info.channels));

    // Channel layout
    addRow(tr("Channel Layout"), info.channelLayout);

    // Sample format
    addRow(tr("Sample Format"), info.sampleFormat);

    // Bits per sample
    if (info.bitsPerSample > 0) {
        addRow(tr("Bits per Sample"), QString::number(info.bitsPerSample));
    }

    // Bitrate
    if (info.bitrate > 0) {
        double kbps = info.bitrate / 1000.0;
        addRow(tr("Bitrate"), QString("%1 kbps").arg(kbps, 0, 'f', 2));
    }

    // Duration
    if (info.duration > 0) {
        double seconds = info.duration / 1000000.0;
        int hours = (int)(seconds / 3600);
        int minutes = (int)((seconds - hours * 3600) / 60);
        double secs = seconds - hours * 3600 - minutes * 60;

        QString durationStr;
        if (hours > 0) {
            durationStr = QString("%1:%2:%3")
                .arg(hours, 2, 10, QChar('0'))
                .arg(minutes, 2, 10, QChar('0'))
                .arg(secs, 6, 'f', 3, QChar('0'));
        } else {
            durationStr = QString("%1:%2")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(secs, 6, 'f', 3, QChar('0'));
        }

        addRow(tr("Duration"), durationStr);
    }

    m_infoTable->resizeColumnsToContents();
}

} // namespace VideoStudio
