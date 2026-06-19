#ifndef MESSAGESPANEL_H
#define MESSAGESPANEL_H

#include <QWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QMap>
#include "core/tr101290data.h"

namespace VideoStudio {

class TSParser;
class TR101290Panel;

enum class MessageLevel {
    All,
    Message,
    Warning,
    Error
};

struct Message {
    MessageLevel level;
    TR101290ErrorType type;
    int64_t offset;
    uint16_t pid;
    int packetIndex;
    QString description;
};

class MessagesPanel : public QWidget {
    Q_OBJECT

public:
    explicit MessagesPanel(QWidget* parent = nullptr);
    ~MessagesPanel();

    void setTSParser(TSParser* parser);
    void setTR101290Panel(TR101290Panel* panel);
    void clear();
    void refresh();

signals:
    void messageDoubleClicked(int packetIndex);

private slots:
    void onLevelFilterChanged(int index);
    void onTypeFilterChanged(int state);
    void onMessageDoubleClicked(QTreeWidgetItem* item, int column);

private:
    void createUI();
    void buildMessageList();
    void updateMessageList();
    QString getMessageTypeName(TR101290ErrorType type) const;
    QString getMessageLevelName(MessageLevel level) const;
    QColor getMessageColor(MessageLevel level) const;
    MessageLevel getMessageLevel(TR101290Priority priority) const;

    TSParser* m_parser;
    TR101290Panel* m_tr101290Panel;
    QVector<Message> m_messages;

    // UI components
    QComboBox* m_levelFilter;
    QTreeWidget* m_messageTree;
    QMap<TR101290ErrorType, QCheckBox*> m_typeFilters;
    QLabel* m_messageCountLabel;
    QLabel* m_warningCountLabel;
    QLabel* m_errorCountLabel;

    // Filter state
    MessageLevel m_currentLevel;
    QSet<TR101290ErrorType> m_enabledTypes;
};

} // namespace VideoStudio

#endif // MESSAGESPANEL_H
