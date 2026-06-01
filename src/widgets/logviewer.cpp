#include "widgets/logviewer.h"
#include <QDateTime>
#include <QScrollBar>
#include <QTimer>

namespace VideoStudio {

LogViewer::LogViewer(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create text edit for log display
    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setStyleSheet(
        "QTextEdit {"
        "    background-color: #1e1e1e;"
        "    color: #d4d4d4;"
        "    font-family: 'Courier New', monospace;"
        "    font-size: 11px;"
        "    border: none;"
        "}"
    );

    // Create toolbar
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    m_clearButton = new QPushButton(tr("Clear"), this);
    m_clearButton->setMaximumWidth(80);
    connect(m_clearButton, &QPushButton::clicked, this, &LogViewer::clear);

    toolbarLayout->addWidget(m_clearButton);
    toolbarLayout->addStretch();

    layout->addLayout(toolbarLayout);
    layout->addWidget(m_textEdit);
}

void LogViewer::appendLog(const QString& message, QtMsgType type) {
    QString formattedMessage = formatMessage(message, type);

    // Append the message immediately
    m_textEdit->append(formattedMessage);

    // Scroll immediately without throttling
    QScrollBar* scrollBar = m_textEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());

    QTextCursor cursor = m_textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_textEdit->setTextCursor(cursor);
    m_textEdit->ensureCursorVisible();
}

void LogViewer::clear() {
    m_textEdit->clear();
}

QString LogViewer::formatMessage(const QString& message, QtMsgType type) {
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString color = getColorForType(type);
    QString typeStr;

    switch (type) {
        case QtDebugMsg:
            typeStr = "DEBUG";
            break;
        case QtInfoMsg:
            typeStr = "INFO";
            break;
        case QtWarningMsg:
            typeStr = "WARN";
            break;
        case QtCriticalMsg:
            typeStr = "ERROR";
            break;
        case QtFatalMsg:
            typeStr = "FATAL";
            break;
    }

    return QString("<span style='color: #808080;'>%1</span> "
                   "<span style='color: %2;'>[%3]</span> "
                   "<span style='color: #d4d4d4;'>%4</span>")
        .arg(timestamp)
        .arg(color)
        .arg(typeStr)
        .arg(message.toHtmlEscaped());
}

QString LogViewer::getColorForType(QtMsgType type) {
    switch (type) {
        case QtDebugMsg:
            return "#4ec9b0";  // Cyan
        case QtInfoMsg:
            return "#4fc1ff";  // Blue
        case QtWarningMsg:
            return "#dcdcaa";  // Yellow
        case QtCriticalMsg:
            return "#f48771";  // Orange
        case QtFatalMsg:
            return "#f14c4c";  // Red
        default:
            return "#d4d4d4";  // White
    }
}

} // namespace VideoStudio
