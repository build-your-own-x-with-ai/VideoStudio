#include "panels/messagespanel.h"
#include "panels/tr101290panel.h"
#include "core/tsparser.h"
#include <QSplitter>
#include <QScrollArea>
#include <QHeaderView>
#include <QDebug>

namespace VideoStudio {

MessagesPanel::MessagesPanel(QWidget* parent)
    : QWidget(parent)
    , m_parser(nullptr)
    , m_tr101290Panel(nullptr)
    , m_currentLevel(MessageLevel::All)
{
    createUI();
}

MessagesPanel::~MessagesPanel() {
}

void MessagesPanel::createUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // Left panel: Filters
    QWidget* filterWidget = new QWidget(this);
    QVBoxLayout* filterLayout = new QVBoxLayout(filterWidget);

    // Level filter
    QGroupBox* levelGroup = new QGroupBox("Level Filter", filterWidget);
    QVBoxLayout* levelLayout = new QVBoxLayout(levelGroup);
    m_levelFilter = new QComboBox(levelGroup);
    m_levelFilter->addItem("All");
    m_levelFilter->addItem("Message");
    m_levelFilter->addItem("Warning");
    m_levelFilter->addItem("Error");
    connect(m_levelFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MessagesPanel::onLevelFilterChanged);
    levelLayout->addWidget(m_levelFilter);
    filterLayout->addWidget(levelGroup);

    // Type filter
    QGroupBox* typeGroup = new QGroupBox("Type Filter", filterWidget);
    QVBoxLayout* typeLayout = new QVBoxLayout(typeGroup);

    QScrollArea* scrollArea = new QScrollArea(typeGroup);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* scrollWidget = new QWidget(scrollArea);
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);

    // Add checkboxes for each message type
    QVector<QPair<QString, TR101290ErrorType>> errorTypes = {
        {"TS Sync Loss", TR101290ErrorType::TSSyncLoss},
        {"Sync Byte Error", TR101290ErrorType::SyncByteError},
        {"PAT Error", TR101290ErrorType::PATError},
        {"Continuity Count Error", TR101290ErrorType::ContinuityCountError},
        {"PMT Error", TR101290ErrorType::PMTError},
        {"PID Error", TR101290ErrorType::PIDError},
        {"Transport Error", TR101290ErrorType::TransportError},
        {"CRC Error", TR101290ErrorType::CRCError},
        {"PCR Repetition Error", TR101290ErrorType::PCRRepetitionError},
        {"PCR Discontinuity Error", TR101290ErrorType::PCRDiscontinuityError},
        {"PCR Accuracy Error", TR101290ErrorType::PCRAccuracyError},
        {"PTS Error", TR101290ErrorType::PTSError},
        {"CAT Error", TR101290ErrorType::CATError},
        {"NIT Actual Error", TR101290ErrorType::NITActualError},
        {"SI Repetition Error", TR101290ErrorType::SIRepetitionError},
        {"Unreferenced PID", TR101290ErrorType::UnreferencedPID},
        {"SDT Actual Error", TR101290ErrorType::SDTActualError},
        {"EIT Actual Error", TR101290ErrorType::EITActualError}
    };

    for (const auto& pair : errorTypes) {
        QCheckBox* checkbox = new QCheckBox(pair.first, scrollWidget);
        checkbox->setChecked(true);
        m_typeFilters[pair.second] = checkbox;
        m_enabledTypes.insert(pair.second);
        connect(checkbox, &QCheckBox::checkStateChanged,
                this, &MessagesPanel::onTypeFilterChanged);
        scrollLayout->addWidget(checkbox);
    }

    scrollLayout->addStretch();
    scrollArea->setWidget(scrollWidget);
    typeLayout->addWidget(scrollArea);
    filterLayout->addWidget(typeGroup);

    // Statistics
    QGroupBox* statsGroup = new QGroupBox("Statistics", filterWidget);
    QVBoxLayout* statsLayout = new QVBoxLayout(statsGroup);
    m_messageCountLabel = new QLabel("Messages: 0", statsGroup);
    m_warningCountLabel = new QLabel("Warnings: 0", statsGroup);
    m_errorCountLabel = new QLabel("Errors: 0", statsGroup);
    statsLayout->addWidget(m_messageCountLabel);
    statsLayout->addWidget(m_warningCountLabel);
    statsLayout->addWidget(m_errorCountLabel);
    filterLayout->addWidget(statsGroup);

    filterLayout->addStretch();
    filterWidget->setMaximumWidth(250);

    // Right panel: Message list
    m_messageTree = new QTreeWidget(this);
    m_messageTree->setHeaderLabels(QStringList() << "Level" << "Type" << "Offset" << "PID" << "Description");
    m_messageTree->setColumnWidth(0, 80);
    m_messageTree->setColumnWidth(1, 150);
    m_messageTree->setColumnWidth(2, 100);
    m_messageTree->setColumnWidth(3, 80);
    m_messageTree->setColumnWidth(4, 300);
    m_messageTree->setAlternatingRowColors(true);
    m_messageTree->setRootIsDecorated(false);
    m_messageTree->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(m_messageTree, &QTreeWidget::itemDoubleClicked,
            this, &MessagesPanel::onMessageDoubleClicked);

    splitter->addWidget(filterWidget);
    splitter->addWidget(m_messageTree);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
}

void MessagesPanel::setTSParser(TSParser* parser) {
    m_parser = parser;
}

void MessagesPanel::setTR101290Panel(TR101290Panel* panel) {
    m_tr101290Panel = panel;
    refresh();
}

void MessagesPanel::clear() {
    m_messages.clear();
    m_messageTree->clear();
    m_parser = nullptr;
    m_tr101290Panel = nullptr;
    m_messageCountLabel->setText("Messages: 0");
    m_warningCountLabel->setText("Warnings: 0");
    m_errorCountLabel->setText("Errors: 0");
}

void MessagesPanel::refresh() {
    if (!m_tr101290Panel) {
        return;
    }

    m_messages.clear();

    qDebug() << "Refreshing messages from TR 101-290 panel...";

    // Get errors from TR 101-290 panel
    const auto& errors = m_tr101290Panel->getErrors();

    for (const TR101290Error& error : errors) {
        Message msg;
        msg.level = getMessageLevel(error.priority);
        msg.type = error.type;
        msg.offset = error.offset;
        msg.pid = error.pid;
        msg.packetIndex = error.packetIndex;
        msg.description = error.description;
        m_messages.append(msg);
    }

    qDebug() << "Found" << m_messages.size() << "messages";

    // Update statistics
    int messageCount = 0;
    int warningCount = 0;
    int errorCount = 0;

    for (const Message& msg : m_messages) {
        switch (msg.level) {
            case MessageLevel::Message:
                messageCount++;
                break;
            case MessageLevel::Warning:
                warningCount++;
                break;
            case MessageLevel::Error:
                errorCount++;
                break;
            default:
                break;
        }
    }

    m_messageCountLabel->setText(QString("Messages: %1").arg(messageCount));
    m_warningCountLabel->setText(QString("Warnings: %1").arg(warningCount));
    m_errorCountLabel->setText(QString("Errors: %1").arg(errorCount));

    buildMessageList();
}

MessageLevel MessagesPanel::getMessageLevel(TR101290Priority priority) const {
    switch (priority) {
        case TR101290Priority::First:
            return MessageLevel::Error;
        case TR101290Priority::Second:
            return MessageLevel::Warning;
        case TR101290Priority::Third:
            return MessageLevel::Message;
        default:
            return MessageLevel::Message;
    }
}

void MessagesPanel::buildMessageList() {
    updateMessageList();
}

void MessagesPanel::updateMessageList() {
    m_messageTree->clear();

    for (const Message& msg : m_messages) {
        // Apply level filter
        if (m_currentLevel != MessageLevel::All && msg.level != m_currentLevel) {
            continue;
        }

        // Apply type filter
        if (!m_enabledTypes.contains(msg.type)) {
            continue;
        }

        QTreeWidgetItem* item = new QTreeWidgetItem(m_messageTree);

        // Level
        item->setText(0, getMessageLevelName(msg.level));
        QColor color = getMessageColor(msg.level);
        item->setForeground(0, QBrush(color));

        // Type
        item->setText(1, getMessageTypeName(msg.type));

        // Offset
        if (msg.offset >= 0) {
            item->setText(2, QString("0x%1").arg(msg.offset, 8, 16, QChar('0')));
        } else {
            item->setText(2, "-");
        }

        // PID
        item->setText(3, QString("0x%1").arg(msg.pid, 4, 16, QChar('0')));

        // Description
        item->setText(4, msg.description);

        // Store packet index
        item->setData(0, Qt::UserRole, msg.packetIndex);
    }
}

QString MessagesPanel::getMessageTypeName(TR101290ErrorType type) const {
    switch (type) {
        case TR101290ErrorType::TSSyncLoss: return "TS Sync Loss";
        case TR101290ErrorType::SyncByteError: return "Sync Byte Error";
        case TR101290ErrorType::PATError: return "PAT Error";
        case TR101290ErrorType::ContinuityCountError: return "Continuity Count Error";
        case TR101290ErrorType::PMTError: return "PMT Error";
        case TR101290ErrorType::PIDError: return "PID Error";
        case TR101290ErrorType::TransportError: return "Transport Error";
        case TR101290ErrorType::CRCError: return "CRC Error";
        case TR101290ErrorType::PCRRepetitionError: return "PCR Repetition Error";
        case TR101290ErrorType::PCRDiscontinuityError: return "PCR Discontinuity Error";
        case TR101290ErrorType::PCRAccuracyError: return "PCR Accuracy Error";
        case TR101290ErrorType::PTSError: return "PTS Error";
        case TR101290ErrorType::CATError: return "CAT Error";
        case TR101290ErrorType::NITActualError: return "NIT Actual Error";
        case TR101290ErrorType::SIRepetitionError: return "SI Repetition Error";
        case TR101290ErrorType::UnreferencedPID: return "Unreferenced PID";
        case TR101290ErrorType::SDTActualError: return "SDT Actual Error";
        case TR101290ErrorType::EITActualError: return "EIT Actual Error";
        default: return "Unknown";
    }
}

QString MessagesPanel::getMessageLevelName(MessageLevel level) const {
    switch (level) {
        case MessageLevel::Message: return "Message";
        case MessageLevel::Warning: return "Warning";
        case MessageLevel::Error: return "Error";
        default: return "All";
    }
}

QColor MessagesPanel::getMessageColor(MessageLevel level) const {
    switch (level) {
        case MessageLevel::Error: return QColor(200, 0, 0); // Dark red
        case MessageLevel::Warning: return QColor(50, 50, 200); // Dark blue
        case MessageLevel::Message: return QColor(0, 150, 0); // Dark green
        default: return Qt::black;
    }
}

void MessagesPanel::onLevelFilterChanged(int index) {
    m_currentLevel = static_cast<MessageLevel>(index);
    updateMessageList();
}

void MessagesPanel::onTypeFilterChanged(Qt::CheckState state) {
    QCheckBox* checkbox = qobject_cast<QCheckBox*>(sender());
    if (!checkbox) {
        return;
    }

    // Find the message type for this checkbox
    for (auto it = m_typeFilters.begin(); it != m_typeFilters.end(); ++it) {
        if (it.value() == checkbox) {
            if (state == Qt::Checked) {
                m_enabledTypes.insert(it.key());
            } else {
                m_enabledTypes.remove(it.key());
            }
            break;
        }
    }

    updateMessageList();
}

void MessagesPanel::onMessageDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);

    if (!item) {
        return;
    }

    QVariant data = item->data(0, Qt::UserRole);
    if (data.isValid()) {
        int packetIndex = data.toInt();
        if (packetIndex >= 0) {
            emit messageDoubleClicked(packetIndex);
            qDebug() << "Message double-clicked, jumping to packet:" << packetIndex;
        }
    }
}

} // namespace VideoStudio
