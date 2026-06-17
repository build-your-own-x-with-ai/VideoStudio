#include "audioinfopanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QPushButton>
#include <QScrollArea>

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

    // Connect monitor signals to LUFS widget
    connect(m_monitor.get(), &AudioMonitor::audioSamplesReady,
            this, [this](const QVector<float>& left, const QVector<float>& right, int sampleRate) {
                std::vector<float> leftVec(left.begin(), left.end());
                std::vector<float> rightVec(right.begin(), right.end());
                m_lufsWidget->updateLoudness(leftVec, rightVec, sampleRate);
                m_phaseMeterWidget->updatePhase(leftVec, rightVec);
            });
}

AudioInfoPanel::~AudioInfoPanel() = default;

void AudioInfoPanel::setupUI() {
    // Create scroll area for all content
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Create container widget for all groups
    QWidget* container = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(3);  // Reduce spacing between widgets

    // Stream selection (collapsible)
    QGroupBox* streamGroup = new QGroupBox(tr("Audio Streams"), container);
    streamGroup->setCheckable(true);
    streamGroup->setChecked(true);
    QVBoxLayout* streamLayout = new QVBoxLayout(streamGroup);

    m_streamCombo = new QComboBox(container);
    connect(m_streamCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AudioInfoPanel::onStreamSelected);
    streamLayout->addWidget(m_streamCombo);

    // Connect toggle to hide/show content
    connect(streamGroup, &QGroupBox::toggled, [streamLayout](bool checked) {
        for (int i = 0; i < streamLayout->count(); ++i) {
            if (QWidget* widget = streamLayout->itemAt(i)->widget()) {
                widget->setVisible(checked);
            }
        }
    });

    mainLayout->addWidget(streamGroup);

    // Audio stream information table (collapsible)
    QGroupBox* infoGroup = new QGroupBox(tr("Stream Information"), this);
    infoGroup->setCheckable(true);
    infoGroup->setChecked(true);
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);

    m_infoTable = new QTableWidget(this);
    m_infoTable->setColumnCount(2);
    m_infoTable->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    m_infoTable->horizontalHeader()->setStretchLastSection(true);
    m_infoTable->verticalHeader()->setVisible(false);
    m_infoTable->setAlternatingRowColors(true);
    m_infoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_infoTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_infoTable->setMinimumHeight(150);  // Reduced from 200
    m_infoTable->setMaximumHeight(200);  // Add maximum height

    infoLayout->addWidget(m_infoTable);

    // Connect toggle to hide/show content
    connect(infoGroup, &QGroupBox::toggled, [infoLayout](bool checked) {
        for (int i = 0; i < infoLayout->count(); ++i) {
            if (QWidget* widget = infoLayout->itemAt(i)->widget()) {
                widget->setVisible(checked);
            }
        }
    });

    mainLayout->addWidget(infoGroup, 1);  // Reduced stretch factor

    // Waveform visualization (collapsible)
    QGroupBox* waveformGroup = new QGroupBox(tr("Waveform"), this);
    waveformGroup->setCheckable(true);
    waveformGroup->setChecked(true);  // Enabled by default
    QVBoxLayout* waveformLayout = new QVBoxLayout(waveformGroup);

    m_waveformWidget = new WaveformWidget(container);
    m_waveformWidget->setMinimumHeight(80);  // Reduced from 100
    waveformLayout->addWidget(m_waveformWidget);

    // Waveform controls
    QHBoxLayout* waveformControls = new QHBoxLayout();
    QPushButton* zoomInBtn = new QPushButton(tr("Zoom In"), waveformGroup);
    QPushButton* zoomOutBtn = new QPushButton(tr("Zoom Out"), waveformGroup);
    QPushButton* zoomFitBtn = new QPushButton(tr("Fit All"), waveformGroup);

    zoomInBtn->setMaximumWidth(80);
    zoomOutBtn->setMaximumWidth(80);
    zoomFitBtn->setMaximumWidth(80);

    // Connect GroupBox toggle to hide/show content
    connect(waveformGroup, &QGroupBox::toggled, [waveformLayout](bool checked) {
        // Hide/show all widgets in the layout
        for (int i = 0; i < waveformLayout->count(); ++i) {
            QWidget* widget = waveformLayout->itemAt(i)->widget();
            if (widget) {
                widget->setVisible(checked);
            }
        }
    });

    connect(zoomInBtn, &QPushButton::clicked, this, [this]() {
        qDebug() << "Zoom In button clicked!";
        if (!m_waveformWidget) {
            qWarning() << "m_waveformWidget is null!";
            return;
        }
        double start = m_waveformWidget->getStartTime();
        double end = m_waveformWidget->getEndTime();
        double duration = m_waveformWidget->getDuration();
        qDebug() << "Current range:" << start << "-" << end << "duration:" << duration;
        double timeRange = end - start;
        double newRange = qMax(0.1, timeRange * 0.7);  // Zoom in 30%
        double center = (start + end) / 2.0;

        double newStart = center - newRange / 2.0;
        double newEnd = center + newRange / 2.0;

        // Clamp to valid range
        if (newStart < 0.0) {
            newStart = 0.0;
            newEnd = newRange;
        }
        if (newEnd > duration) {
            newEnd = duration;
            newStart = duration - newRange;
            if (newStart < 0.0) newStart = 0.0;
        }

        qDebug() << "Zoom In:" << start << "-" << end << "=>" << newStart << "-" << newEnd;
        m_waveformWidget->setTimeRange(newStart, newEnd);
    });

    connect(zoomOutBtn, &QPushButton::clicked, this, [this]() {
        qDebug() << "Zoom Out button clicked!";
        if (!m_waveformWidget) {
            qWarning() << "m_waveformWidget is null!";
            return;
        }
        double start = m_waveformWidget->getStartTime();
        double end = m_waveformWidget->getEndTime();
        double duration = m_waveformWidget->getDuration();
        double timeRange = end - start;
        double newRange = qMin(duration, timeRange * 1.5);  // Zoom out 50%
        double center = (start + end) / 2.0;

        double newStart = center - newRange / 2.0;
        double newEnd = center + newRange / 2.0;

        // Clamp to valid range
        if (newStart < 0.0) {
            newStart = 0.0;
            newEnd = newRange;
        }
        if (newEnd > duration) {
            newEnd = duration;
            newStart = duration - newRange;
            if (newStart < 0.0) newStart = 0.0;
        }

        qDebug() << "Zoom Out:" << start << "-" << end << "=>" << newStart << "-" << newEnd;
        m_waveformWidget->setTimeRange(newStart, newEnd);
    });

    connect(zoomFitBtn, &QPushButton::clicked, this, [this]() {
        qDebug() << "Fit All button clicked!";
        if (!m_waveformWidget) {
            qWarning() << "m_waveformWidget is null!";
            return;
        }
        double duration = m_waveformWidget->getDuration();
        qDebug() << "Fit All: 0.0 -" << duration;
        m_waveformWidget->setTimeRange(0.0, duration);
    });

    waveformControls->addWidget(zoomInBtn);
    waveformControls->addWidget(zoomOutBtn);
    waveformControls->addWidget(zoomFitBtn);
    waveformControls->addStretch();
    QLabel* zoomHint = new QLabel(tr("Tip: Ctrl+Wheel to zoom"), waveformGroup);
    zoomHint->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    waveformControls->addWidget(zoomHint);

    waveformLayout->addLayout(waveformControls);

    // Connect toggle to hide/show content (must be after all widgets added)
    connect(waveformGroup, &QGroupBox::toggled, [waveformLayout](bool checked) {
        for (int i = 0; i < waveformLayout->count(); ++i) {
            QLayoutItem* item = waveformLayout->itemAt(i);
            if (QWidget* widget = item->widget()) {
                widget->setVisible(checked);
            } else if (QLayout* layout = item->layout()) {
                // Handle nested layouts
                for (int j = 0; j < layout->count(); ++j) {
                    if (QWidget* subWidget = layout->itemAt(j)->widget()) {
                        subWidget->setVisible(checked);
                    }
                }
            }
        }
    });

    mainLayout->addWidget(waveformGroup);

    // Audio level meters (collapsible)
    QGroupBox* levelGroup = new QGroupBox(tr("Audio Levels"), this);
    levelGroup->setCheckable(true);
    levelGroup->setChecked(true);
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

    // Connect toggle to hide/show content
    connect(levelGroup, &QGroupBox::toggled, [levelLayout](bool checked) {
        for (int i = 0; i < levelLayout->count(); ++i) {
            QLayoutItem* item = levelLayout->itemAt(i);
            if (QWidget* widget = item->widget()) {
                widget->setVisible(checked);
            } else if (QLayout* layout = item->layout()) {
                for (int j = 0; j < layout->count(); ++j) {
                    if (QWidget* subWidget = layout->itemAt(j)->widget()) {
                        subWidget->setVisible(checked);
                    }
                }
            }
        }
    });

    mainLayout->addWidget(levelGroup);

    // Spectrum analyzer (collapsible)
    QGroupBox* spectrumGroup = new QGroupBox(tr("Spectrum Analyzer"), this);
    spectrumGroup->setCheckable(true);
    spectrumGroup->setChecked(false);  // Hidden by default
    QVBoxLayout* spectrumLayout = new QVBoxLayout(spectrumGroup);

    m_spectrumWidget = new SpectrumWidget(this);
    m_spectrumWidget->setMinimumHeight(200);  // Increased from 150
    m_spectrumWidget->setDisplayMode(SpectrumWidget::Bars);
    spectrumLayout->addWidget(m_spectrumWidget);

    // Spectrum controls
    QHBoxLayout* spectrumControls = new QHBoxLayout();

    // Display mode selector
    QLabel* modeLabel = new QLabel(tr("Display Mode:"), this);
    QComboBox* modeCombo = new QComboBox(this);
    modeCombo->addItem(tr("Bars"), SpectrumWidget::Bars);
    modeCombo->addItem(tr("Line"), SpectrumWidget::Line);
    modeCombo->addItem(tr("Filled"), SpectrumWidget::Filled);
    modeCombo->addItem(tr("Waterfall"), SpectrumWidget::Waterfall);
    connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, modeCombo](int index) {
        m_spectrumWidget->setDisplayMode(static_cast<SpectrumWidget::DisplayMode>(modeCombo->itemData(index).toInt()));
    });
    spectrumControls->addWidget(modeLabel);
    spectrumControls->addWidget(modeCombo);

    spectrumControls->addStretch();
    spectrumLayout->addLayout(spectrumControls);

    // Connect toggle to hide/show content
    connect(spectrumGroup, &QGroupBox::toggled, [spectrumLayout](bool checked) {
        for (int i = 0; i < spectrumLayout->count(); ++i) {
            QLayoutItem* item = spectrumLayout->itemAt(i);
            if (QWidget* widget = item->widget()) {
                widget->setVisible(checked);
            } else if (QLayout* layout = item->layout()) {
                for (int j = 0; j < layout->count(); ++j) {
                    if (QWidget* subWidget = layout->itemAt(j)->widget()) {
                        subWidget->setVisible(checked);
                    }
                }
            }
        }
    });

    mainLayout->addWidget(spectrumGroup);

    // LUFS Loudness Meter (collapsible)
    QGroupBox* lufsGroup = new QGroupBox(tr("LUFS Loudness Meter"), this);
    lufsGroup->setCheckable(true);
    lufsGroup->setChecked(false);  // Hidden by default
    QVBoxLayout* lufsLayout = new QVBoxLayout(lufsGroup);

    m_lufsWidget = new LUFSWidget(this);
    lufsLayout->addWidget(m_lufsWidget);

    // Connect toggle to hide/show content
    connect(lufsGroup, &QGroupBox::toggled, [lufsLayout](bool checked) {
        for (int i = 0; i < lufsLayout->count(); ++i) {
            if (QWidget* widget = lufsLayout->itemAt(i)->widget()) {
                widget->setVisible(checked);
            }
        }
    });

    mainLayout->addWidget(lufsGroup);

    // Stereo Phase Meter (collapsible)
    QGroupBox* phaseGroup = new QGroupBox(tr("Stereo Phase Meter"), this);
    phaseGroup->setCheckable(true);
    phaseGroup->setChecked(false);  // Hidden by default
    QVBoxLayout* phaseLayout = new QVBoxLayout(phaseGroup);

    m_phaseMeterWidget = new PhaseMeterWidget(this);
    // Height will auto-adjust based on width (square aspect ratio)
    phaseLayout->addWidget(m_phaseMeterWidget);

    // Connect toggle to hide/show content
    connect(phaseGroup, &QGroupBox::toggled, [phaseLayout](bool checked) {
        for (int i = 0; i < phaseLayout->count(); ++i) {
            if (QWidget* widget = phaseLayout->itemAt(i)->widget()) {
                widget->setVisible(checked);
            }
        }
    });

    mainLayout->addWidget(phaseGroup);

    // Status label
    m_statusLabel = new QLabel(container);
    m_statusLabel->setStyleSheet("QLabel { padding: 5px; }");
    mainLayout->addWidget(m_statusLabel);

    // Set the container as scroll area's widget
    scrollArea->setWidget(container);

    // Set scroll area as this panel's main widget
    QVBoxLayout* panelLayout = new QVBoxLayout(this);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->addWidget(scrollArea);
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

        // Load waveform with first stream (default -1 = first audio stream)
        m_waveformWidget->setAudioFile(filename, -1);

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
