#ifndef LOGVIEWER_H
#define LOGVIEWER_H

#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHBoxLayout>

namespace VideoStudio {

class LogViewer : public QWidget {
    Q_OBJECT

public:
    explicit LogViewer(QWidget* parent = nullptr);
    ~LogViewer() = default;

public slots:
    void appendLog(const QString& message, QtMsgType type);
    void clear();

private:
    QTextEdit* m_textEdit;
    QPushButton* m_clearButton;

    QString formatMessage(const QString& message, QtMsgType type);
    QString getColorForType(QtMsgType type);
};

} // namespace VideoStudio

#endif // LOGVIEWER_H
