#include "audioinfopanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QPushButton>

namespace VideoStudio {

AudioInfoPanel::AudioInfoPanel(QWidget* parent)
    : QWidget(parent)
    , m_analyzer(std::make_unique<AudioAnalyzer>())
    , m_monitor(std::make_unique<AudioMonitor>())
{
    setupUI();

    // Connect monitor to widgets
    m_monitor->setLevelMeterLeft(m_levelMeterLeft);
    m_monitor->setLevelMeterRight(m_levelMeterRight);
    m_monitor->setSpectrumWidget(m_spectrumWidget);
    m_monitor->setWaveformWidget(m_waveformWidget);

    // Connect monitor signals to spectrum widget
    connect(m_monitor.get(), &AudioMonitor::spectrumUpdated,
            m_spectrumWidget, &SpectrumWidget::updateSpectrumData);
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

    // Waveform visualization
    QGroupBox* waveformGroup = new QGroupBox(tr("Waveform"), this);
    QVBoxLayout* waveformLayout = new QVBoxLayout(waveformGroup);

    m_waveformWidget = new WaveformWidget(this);
    m_waveformWidget->setMinimumHeight(100);
    waveformLayout->addWidget(m_waveformWidget);

    mainLayout->addWidget(waveformGroup);
    waveformGroup->hide(); // Hidden by default

    // Audio level meters
    QGroupBox* levelGroup = new QGroupBox(tr("Audio Levels"), this);
    QHBoxLayout* levelLayout = new QHBoxLayout(levelGroup);

    // Left channel meter (or mono)
    QVBoxLayout* leftLayout = new QVBoxLayout();
    QLabel* leftLabel = new QLabel(tr("L / Mono"), this);
    leftLabel->setAlignment(Qt::AlignCenter);
    m_levelMeterLeft = new AudioLevelWidget(this);
    m_levelMeterLeft->setOrientation(Qt::Vertical);
    m_levelMeterLeft->setFixedWidth(60);
    leftLayout->addWidget(leftLabel);
    leftLayout->addWidget(m_levelMeterLeft, 1);

    // Right channel meter
    QVBoxLayout* rightLayout = new QVBoxLayout();
    QLabel* rightLabel = new QLabel(tr("R"), this);
    rightLabel->setAlignment(Qt::AlignCenter);
    m_levelMeterRight = new AudioLevelWidget(this);
    m_levelMeterRight->setOrientation(Qt::Vertical);
    m_levelMeterRight->setFixedWidth(60);
    rightLayout->addWidget(rightLabel);
    rightLayout->addWidget(m_levelMeterRight, 1);
    m_levelMeterRight->hide(); // Hide for mono audio

    levelLayout->addLayout(leftLayout);
    levelLayout->addLayout(rightLayout);
    levelLayout->addStretch();

    mainLayout->addWidget(levelGroup);

    // Show waveform button
    m_showWaveformButton = new QPushButton(tr("Show Waveform"), this);
    m_showWaveformButton->setCheckable(true);
    connect(m_showWaveformButton, &QPushButton::toggled, waveformGroup, &QWidget::setVisible);
    mainLayout->addWidget(m_showWaveformButton);

    // Spectrum analyzer
    QGroupBox* spectrumGroup = new QGroupBox(tr("Spectrum Analyzer"), this);
    QVBoxLayout* spectrumLayout = new QVBoxLayout(spectrumGroup);

    m_spectrumWidget = new SpectrumWidget(this);
    m_spectrumWidget->setMinimumHeight(200);
    m_spectrumWidget->setDisplayMode(SpectrumWidget::Bars);
    spectrumLayout->addWidget(m_spectrumWidget);

    // Spectrum controls
    QHBoxLayout* spectrumControls = new QHBoxLayout();

    QPushButton* startButton = new QPushButton(tr("Start Analysis"), this);
    connect(startButton, &QPushButton::clicked, this, [this]() {
        m_spectrumWidget->startAnalysis();
        m_monitor->start();
    });
    spectrumControls->addWidget(startButton);

    QPushButton* stopButton = new QPushButton(tr("Stop"), this);
    connect(stopButton, &QPushButton::clicked, this, [this]() {
        m_spectrumWidget->stopAnalysis();
        m_monitor->stop();
    });
    spectrumControls->addWidget(stopButton);

    QComboBox* modeCombo = new QComboBox(this);
    modeCombo->addItem(tr("Bars"), SpectrumWidget::Bars);
    modeCombo->addItem(tr("Line"), SpectrumWidget::Line);
    modeCombo->addItem(tr("Filled"), SpectrumWidget::Filled);
    modeCombo->addItem(tr("Waterfall"), SpectrumWidget::Waterfall);
    connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, modeCombo](int index) {
        m_spectrumWidget->setDisplayMode(static_cast<SpectrumWidget::DisplayMode>(modeCombo->itemData(index).toInt()));
    });
    spectrumControls->addWidget(new QLabel(tr("Display Mode:"), this));
    spectrumControls->addWidget(modeCombo);

    spectrumControls->addStretch();
    spectrumLayout->addLayout(spectrumControls);

    mainLayout->addWidget(spectrumGroup);
    spectrumGroup->hide(); // Hidden by default

    // Show spectrum button
    m_showSpectrumButton = new QPushButton(tr("Show Spectrum"), this);
    m_showSpectrumButton->setCheckable(true);
    connect(m_showSpectrumButton, &QPushButton::toggled, spectrumGroup, &QWidget::setVisible);
    mainLayout->addWidget(m_showSpectrumButton);

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

        // Load waveform
        m_waveformWidget->setAudioFile(filename);

        // Load spectrum analyzer
        m_spectrumWidget->setAudioFile(filename);

        // Setup audio monitor for real-time updates
        m_monitor->setAudioFile(filename);

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

    // Stop monitoring
    m_monitor->stop();

    // Clear visualization widgets
    m_waveformWidget->clear();
    m_spectrumWidget->clear();
    m_levelMeterLeft->setPeakLevel(0.0f);
    m_levelMeterLeft->setRMSLevel(0.0f);
    m_levelMeterRight->setPeakLevel(0.0f);
    m_levelMeterRight->setRMSLevel(0.0f);
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
