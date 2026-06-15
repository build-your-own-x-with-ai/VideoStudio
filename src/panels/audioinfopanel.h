#ifndef AUDIOINFOPANEL_H
#define AUDIOINFOPANEL_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include "core/audioanalyzer.h"
#include "widgets/waveformwidget.h"
#include "widgets/audiolevelwidget.h"

namespace VideoStudio {

class AudioInfoPanel : public QWidget {
    Q_OBJECT

public:
    explicit AudioInfoPanel(QWidget* parent = nullptr);
    ~AudioInfoPanel() override;

    void setAudioFile(const QString& filename);
    void clear();

private slots:
    void onStreamSelected(int index);

private:
    void setupUI();
    void displayStreamInfo(const AudioStreamInfo& info);
    void populateStreamList();

    QString m_currentFile;
    std::unique_ptr<AudioAnalyzer> m_analyzer;

    // UI components
    QComboBox* m_streamCombo;
    QTableWidget* m_infoTable;
    QLabel* m_statusLabel;

    // Visualization widgets
    WaveformWidget* m_waveformWidget;
    AudioLevelWidget* m_levelMeterLeft;
    AudioLevelWidget* m_levelMeterRight;
    QPushButton* m_showWaveformButton;
};

} // namespace VideoStudio

#endif // AUDIOINFOPANEL_H
