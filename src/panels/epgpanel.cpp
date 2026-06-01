#include "panels/epgpanel.h"
#include "core/tsparser.h"
#include "core/tsdata.h"
#include <QHeaderView>
#include <QDebug>
#include <QPainter>
#include <QScrollArea>
#include <QLabel>
#include <QStackedWidget>

namespace VideoStudio {

// Custom widget for TimeLine mode
class TimeLineWidget : public QWidget {
public:
    TimeLineWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(400);
    }

    void setEvents(const QVector<EPGEvent>& events, int timeDistribution) {
        m_events = events;
        m_timeDistribution = timeDistribution;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        if (m_events.isEmpty()) {
            painter.drawText(rect(), Qt::AlignCenter, "No EPG events to display");
            return;
        }

        // Find time range
        QDateTime minTime = m_events.first().startTime;
        QDateTime maxTime = m_events.first().endTime;
        for (const EPGEvent& evt : m_events) {
            if (evt.startTime < minTime) minTime = evt.startTime;
            if (evt.endTime > maxTime) maxTime = evt.endTime;
        }

        int totalMinutes = minTime.secsTo(maxTime) / 60;
        if (totalMinutes == 0) totalMinutes = 60;

        // Calculate dimensions
        int margin = 50;
        int timelineY = 30;
        int eventHeight = 40;
        int eventSpacing = 10;

        // Apply time distribution scaling (pixels per minute)
        // Higher m_timeDistribution = more horizontal space per minute
        int pixelsPerMinute = m_timeDistribution;
        int totalWidth = totalMinutes * pixelsPerMinute;

        // Update widget width to accommodate the timeline
        setMinimumWidth(totalWidth + 2 * margin);

        // Draw time axis
        painter.setPen(QPen(Qt::black, 2));
        painter.drawLine(margin, timelineY, totalWidth + margin, timelineY);

        // Draw time labels
        int numLabels = qMin(10, totalMinutes / 10);
        if (numLabels == 0) numLabels = 1;
        for (int i = 0; i <= numLabels; ++i) {
            int x = margin + totalWidth * i / numLabels;
            painter.drawLine(x, timelineY - 5, x, timelineY + 5);

            QDateTime labelTime = minTime.addSecs(totalMinutes * 60 * i / numLabels);
            QString label = labelTime.toString("HH:mm");
            painter.drawText(x - 20, timelineY - 10, 40, 20, Qt::AlignCenter, label);
        }

        // Group events by service
        QMap<uint16_t, QVector<EPGEvent>> eventsByService;
        for (const EPGEvent& evt : m_events) {
            eventsByService[evt.serviceId].append(evt);
        }

        // Draw events
        int yOffset = timelineY + 30;
        for (auto it = eventsByService.begin(); it != eventsByService.end(); ++it) {
            uint16_t serviceId = it.key();
            const QVector<EPGEvent>& serviceEvents = it.value();

            // Draw service label
            painter.setPen(Qt::black);
            painter.drawText(5, yOffset + eventHeight / 2, QString("Service %1").arg(serviceId));

            // Draw events for this service
            for (const EPGEvent& evt : serviceEvents) {
                int startMinutes = minTime.secsTo(evt.startTime) / 60;
                int durationMinutes = evt.duration / 60;

                int x = margin + startMinutes * pixelsPerMinute;
                int w = durationMinutes * pixelsPerMinute;

                // Draw event box
                QColor color = evt.isRunning ? QColor(100, 255, 100) : QColor(150, 200, 255);
                painter.fillRect(x, yOffset, w, eventHeight, color);
                painter.setPen(Qt::black);
                painter.drawRect(x, yOffset, w, eventHeight);

                // Draw event name
                painter.drawText(x + 5, yOffset, w - 10, eventHeight,
                               Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                               evt.eventName);
            }

            yOffset += eventHeight + eventSpacing;
        }

        // Update widget height
        setMinimumHeight(yOffset + margin);
    }

private:
    QVector<EPGEvent> m_events;
    int m_timeDistribution;
};

EPGPanel::EPGPanel(QWidget* parent)
    : QWidget(parent)
    , m_parser(nullptr)
    , m_timeDistribution(10)
    , m_currentMode(TextMode)
{
    createUI();
}

EPGPanel::~EPGPanel() {
}

void EPGPanel::createUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Toolbar
    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(16, 16));

    // Mode selection
    m_toolbar->addWidget(new QLabel("Mode:", this));
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem("Text");
    m_modeCombo->addItem("TimeLine");
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EPGPanel::onModeChanged);
    m_toolbar->addWidget(m_modeCombo);

    m_toolbar->addSeparator();

    // Service selection
    m_toolbar->addWidget(new QLabel("Service:", this));
    m_serviceCombo = new QComboBox(this);
    m_serviceCombo->addItem("All Services");
    connect(m_serviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EPGPanel::onServiceChanged);
    m_toolbar->addWidget(m_serviceCombo);

    m_toolbar->addSeparator();

    // Time distribution controls (for TimeLine mode)
    m_toolbar->addWidget(new QLabel("Time Distribution:", this));
    m_decreaseButton = new QPushButton("-", this);
    m_decreaseButton->setMaximumWidth(30);
    connect(m_decreaseButton, &QPushButton::clicked,
            this, &EPGPanel::onTimeDistributionDecrease);
    m_toolbar->addWidget(m_decreaseButton);

    m_increaseButton = new QPushButton("+", this);
    m_increaseButton->setMaximumWidth(30);
    connect(m_increaseButton, &QPushButton::clicked,
            this, &EPGPanel::onTimeDistributionIncrease);
    m_toolbar->addWidget(m_increaseButton);

    mainLayout->addWidget(m_toolbar);

    // Content widget (stacked: tree widget for Text mode, custom widget for TimeLine mode)
    QStackedWidget* stackedWidget = new QStackedWidget(this);

    // Text mode: Tree widget
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels(QStringList() << "Service" << "Event" << "Start Time" << "End Time" << "Duration" << "Description");
    m_treeWidget->setColumnWidth(0, 150);
    m_treeWidget->setColumnWidth(1, 200);
    m_treeWidget->setColumnWidth(2, 150);
    m_treeWidget->setColumnWidth(3, 150);
    m_treeWidget->setColumnWidth(4, 80);
    m_treeWidget->setColumnWidth(5, 300);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setMouseTracking(true);  // Enable mouse tracking for hover
    connect(m_treeWidget, &QTreeWidget::itemEntered, this, [this](QTreeWidgetItem* item, int column) {
        if (item) {
            // Show tooltip with full event details
            QString tooltip = QString(
                "<b>Service:</b> %1<br>"
                "<b>Event:</b> %2<br>"
                "<b>Start:</b> %3<br>"
                "<b>End:</b> %4<br>"
                "<b>Duration:</b> %5<br>"
                "<b>Description:</b> %6"
            ).arg(item->text(0))
             .arg(item->text(1))
             .arg(item->text(2))
             .arg(item->text(3))
             .arg(item->text(4))
             .arg(item->text(5));
            item->setToolTip(column, tooltip);
        }
    });
    stackedWidget->addWidget(m_treeWidget);

    // TimeLine mode: Custom widget with scroll area
    QScrollArea* scrollArea = new QScrollArea(this);
    m_timeLineWidget = new TimeLineWidget(this);
    scrollArea->setWidget(m_timeLineWidget);
    scrollArea->setWidgetResizable(true);
    stackedWidget->addWidget(scrollArea);

    mainLayout->addWidget(stackedWidget);
    m_contentWidget = stackedWidget;
}

void EPGPanel::setTSParser(TSParser* parser) {
    m_parser = parser;

    if (m_parser) {
        parseEITTables();
        updateEventList();
    }
}

void EPGPanel::clear() {
    m_parser = nullptr;
    m_events.clear();
    m_treeWidget->clear();
    m_serviceCombo->clear();
    m_serviceCombo->addItem("All Services");
}

void EPGPanel::parseEITTables() {
    if (!m_parser) {
        return;
    }

    m_events.clear();

    // Get all PSI tables from parser
    const auto& tables = m_parser->getPSITables();

    // Parse EIT tables (PID 0x12)
    for (const auto& table : tables) {
        if (table.type == PSITableType::EIT && table.pid == 0x0012) {
            // Parse EIT table data
            const QByteArray& data = table.data;

            if (data.size() < 14) {
                continue;  // Invalid EIT table
            }

            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());

            // EIT table structure:
            // table_id (8 bits)
            // section_syntax_indicator (1 bit) + reserved (3 bits) + section_length (12 bits)
            // service_id (16 bits)
            // reserved (2 bits) + version_number (5 bits) + current_next_indicator (1 bit)
            // section_number (8 bits)
            // last_section_number (8 bits)
            // transport_stream_id (16 bits)
            // original_network_id (16 bits)
            // segment_last_section_number (8 bits)
            // last_table_id (8 bits)

            uint16_t serviceId = (bytes[3] << 8) | bytes[4];
            uint8_t tableId = bytes[0];

            // Parse events (starting at byte 14)
            int pos = 14;
            while (pos + 12 <= data.size()) {
                EPGEvent event;
                event.serviceId = serviceId;
                event.serviceName = QString("Service %1").arg(serviceId);

                // event_id (16 bits)
                event.eventId = (bytes[pos] << 8) | bytes[pos + 1];
                pos += 2;

                // start_time (40 bits - MJD + BCD time)
                uint16_t mjd = (bytes[pos] << 8) | bytes[pos + 1];
                uint8_t hour = bytes[pos + 2];
                uint8_t minute = bytes[pos + 3];
                uint8_t second = bytes[pos + 4];
                pos += 5;

                // Convert MJD to QDate
                int y_prime = (mjd - 15078.2) / 365.25;
                int m_prime = (mjd - 14956.1 - int(y_prime * 365.25)) / 30.6001;
                int day = mjd - 14956 - int(y_prime * 365.25) - int(m_prime * 30.6001);
                int month = (m_prime <= 13) ? m_prime - 1 : m_prime - 13;
                int year = (month <= 2) ? y_prime + 1901 : y_prime + 1900;

                // Convert BCD to decimal
                int hourDec = ((hour >> 4) * 10) + (hour & 0x0F);
                int minuteDec = ((minute >> 4) * 10) + (minute & 0x0F);
                int secondDec = ((second >> 4) * 10) + (second & 0x0F);

                event.startTime = QDateTime(QDate(year, month, day), QTime(hourDec, minuteDec, secondDec));

                qDebug() << "Parsed event start time:" << event.startTime.toString("yyyy-MM-dd HH:mm:ss");

                // Validate time range (reject events outside reasonable range: 2000-2100)
                if (year < 2000 || year > 2100) {
                    qDebug() << "Skipping event with invalid year:" << year;
                    break;  // Stop parsing this EIT table
                }

                // duration (24 bits - BCD time)
                uint8_t durationHour = bytes[pos];
                uint8_t durationMinute = bytes[pos + 1];
                uint8_t durationSecond = bytes[pos + 2];
                pos += 3;

                int durationHourDec = ((durationHour >> 4) * 10) + (durationHour & 0x0F);
                int durationMinuteDec = ((durationMinute >> 4) * 10) + (durationMinute & 0x0F);
                int durationSecondDec = ((durationSecond >> 4) * 10) + (durationSecond & 0x0F);

                event.duration = durationHourDec * 3600 + durationMinuteDec * 60 + durationSecondDec;
                event.endTime = event.startTime.addSecs(event.duration);

                // running_status (3 bits) + free_CA_mode (1 bit) + descriptors_loop_length (12 bits)
                uint8_t runningStatus = (bytes[pos] >> 5) & 0x07;
                uint16_t descriptorsLength = ((bytes[pos] & 0x0F) << 8) | bytes[pos + 1];
                pos += 2;

                event.isRunning = (runningStatus == 4);  // 4 = running
                event.eventName = QString("Event %1").arg(event.eventId);
                event.description = "No description available";
                event.language = "eng";

                // Parse descriptors
                int descEnd = pos + descriptorsLength;
                while (pos + 2 <= descEnd && pos + 2 <= data.size()) {
                    uint8_t descriptorTag = bytes[pos];
                    uint8_t descriptorLength = bytes[pos + 1];
                    pos += 2;

                    if (pos + descriptorLength > data.size()) {
                        break;
                    }

                    // Short event descriptor (0x4D)
                    if (descriptorTag == 0x4D && descriptorLength >= 4) {
                        // ISO_639_language_code (3 bytes)
                        event.language = QString::fromLatin1(reinterpret_cast<const char*>(&bytes[pos]), 3);
                        int descPos = pos + 3;

                        // event_name_length (1 byte)
                        if (descPos < pos + descriptorLength) {
                            uint8_t eventNameLength = bytes[descPos];
                            descPos++;

                            // event_name
                            if (descPos + eventNameLength <= pos + descriptorLength) {
                                event.eventName = QString::fromUtf8(reinterpret_cast<const char*>(&bytes[descPos]), eventNameLength);
                                descPos += eventNameLength;
                            }

                            // text_length (1 byte)
                            if (descPos < pos + descriptorLength) {
                                uint8_t textLength = bytes[descPos];
                                descPos++;

                                // text
                                if (descPos + textLength <= pos + descriptorLength) {
                                    event.description = QString::fromUtf8(reinterpret_cast<const char*>(&bytes[descPos]), textLength);
                                }
                            }
                        }
                    }

                    pos += descriptorLength;
                }

                pos = descEnd;  // Skip to next event

                m_events.append(event);

                if (pos >= data.size()) {
                    break;
                }
            }
        }
    }

    qDebug() << "Parsed" << m_events.size() << "EPG events";

    // Update service combo box
    m_serviceCombo->clear();
    m_serviceCombo->addItem("All Services");

    QSet<uint16_t> services;
    for (const EPGEvent& event : m_events) {
        services.insert(event.serviceId);
    }

    for (uint16_t serviceId : services) {
        m_serviceCombo->addItem(QString("Service %1").arg(serviceId), serviceId);
    }
}

void EPGPanel::updateEventList() {
    m_treeWidget->clear();

    // Get selected service filter
    int selectedService = -1;  // -1 means all services
    if (m_serviceCombo->currentIndex() > 0) {
        selectedService = m_serviceCombo->currentData().toInt();
    }

    if (m_currentMode == TextMode) {
        // Text mode: display events in tree widget
        for (const EPGEvent& event : m_events) {
            // Apply service filter
            if (selectedService != -1 && event.serviceId != selectedService) {
                continue;
            }

            QTreeWidgetItem* item = new QTreeWidgetItem();
            item->setText(0, event.serviceName);
            item->setText(1, event.eventName);
            item->setText(2, event.startTime.toString("yyyy-MM-dd HH:mm:ss"));
            item->setText(3, event.endTime.toString("yyyy-MM-dd HH:mm:ss"));
            item->setText(4, QString("%1 min").arg(event.duration / 60));
            item->setText(5, event.description);

            // Highlight running events
            if (event.isRunning) {
                for (int i = 0; i < 6; ++i) {
                    item->setForeground(i, QBrush(QColor(100, 255, 100)));
                }
            }

            m_treeWidget->addTopLevelItem(item);
        }
    } else {
        // TimeLine mode: update timeline widget
        QVector<EPGEvent> filteredEvents;
        for (const EPGEvent& event : m_events) {
            if (selectedService == -1 || event.serviceId == selectedService) {
                filteredEvents.append(event);
            }
        }
        m_timeLineWidget->setEvents(filteredEvents, m_timeDistribution);
    }
}

void EPGPanel::drawTimeLine() {
    // Get selected service filter
    int selectedService = -1;
    if (m_serviceCombo->currentIndex() > 0) {
        selectedService = m_serviceCombo->currentData().toInt();
    }

    // Filter events
    QVector<EPGEvent> filteredEvents;
    for (const EPGEvent& event : m_events) {
        if (selectedService == -1 || event.serviceId == selectedService) {
            filteredEvents.append(event);
        }
    }

    m_timeLineWidget->setEvents(filteredEvents, m_timeDistribution);
}

void EPGPanel::onModeChanged(int index) {
    m_currentMode = (index == 0) ? TextMode : TimeLineMode;

    QStackedWidget* stackedWidget = qobject_cast<QStackedWidget*>(m_contentWidget);
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(index);
    }

    updateEventList();
    qDebug() << "EPG mode changed to" << (m_currentMode == TextMode ? "Text" : "TimeLine");
}

void EPGPanel::onTimeDistributionIncrease() {
    m_timeDistribution += 5;
    if (m_timeDistribution > 60) {
        m_timeDistribution = 60;
    }
    qDebug() << "Time distribution increased to" << m_timeDistribution;
    drawTimeLine();
}

void EPGPanel::onTimeDistributionDecrease() {
    m_timeDistribution -= 5;
    if (m_timeDistribution < 5) {
        m_timeDistribution = 5;
    }
    qDebug() << "Time distribution decreased to" << m_timeDistribution;
    drawTimeLine();
}

void EPGPanel::onServiceChanged(int index) {
    Q_UNUSED(index);
    updateEventList();
    qDebug() << "Service filter changed";
}

} // namespace VideoStudio
