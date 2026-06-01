#ifndef EPGPANEL_H
#define EPGPANEL_H

#include <QWidget>
#include <QTreeWidget>
#include <QToolBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <QComboBox>

namespace VideoStudio {

class TSParser;
class TimeLineWidget;

struct EPGEvent {
    uint16_t serviceId;         // Service ID
    uint16_t eventId;           // Event ID
    QString serviceName;        // Service name
    QString eventName;          // Event name
    QString description;        // Event description
    QDateTime startTime;        // Start time
    QDateTime endTime;          // End time
    int duration;               // Duration in seconds
    QString language;           // Language code
    bool isRunning;             // Currently running
};

class EPGPanel : public QWidget {
    Q_OBJECT

public:
    explicit EPGPanel(QWidget* parent = nullptr);
    ~EPGPanel();

    void setTSParser(TSParser* parser);
    void clear();

    // Get EPG events
    QVector<EPGEvent> getEvents() const { return m_events; }

private slots:
    void onModeChanged(int index);
    void onTimeDistributionIncrease();
    void onTimeDistributionDecrease();
    void onServiceChanged(int index);

private:
    void createUI();
    void updateEventList();
    void parseEITTables();
    void drawTimeLine();

    TSParser* m_parser;
    QVector<EPGEvent> m_events;
    int m_timeDistribution;  // Time spacing in TimeLine mode (minutes per pixel)

    // UI components
    QWidget* m_contentWidget;
    QTreeWidget* m_treeWidget;      // For Text mode
    TimeLineWidget* m_timeLineWidget;      // For TimeLine mode
    QToolBar* m_toolbar;
    QComboBox* m_modeCombo;
    QComboBox* m_serviceCombo;
    QPushButton* m_increaseButton;
    QPushButton* m_decreaseButton;

    enum DisplayMode {
        TextMode,
        TimeLineMode
    };
    DisplayMode m_currentMode;
};

} // namespace VideoStudio

#endif // EPGPANEL_H
