#ifndef COMMENTSPANEL_H
#define COMMENTSPANEL_H

#include <QWidget>
#include <QTreeWidget>
#include <QToolBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVector>
#include <QString>
#include <QDateTime>

namespace VideoStudio {

class TSParser;

struct Comment {
    int id;                     // Unique comment ID
    QString author;             // Comment author
    QString text;               // Comment text
    QDateTime timestamp;        // Creation timestamp
    int64_t offset;            // Byte offset in TS file
    int packetIndex;           // Packet index (-1 for frame comments)
    int frameIndex;            // Frame index (-1 for packet comments)
    QString category;          // Category (e.g., "Error", "Info", "Warning")
    bool resolved;             // Whether the comment is resolved
};

class CommentsPanel : public QWidget {
    Q_OBJECT

public:
    explicit CommentsPanel(QWidget* parent = nullptr);
    ~CommentsPanel();

    void setTSParser(TSParser* parser);
    void clear();

    // Comment management
    void addComment(const Comment& comment);
    void removeComment(int commentId);
    void updateComment(int commentId, const Comment& comment);
    QVector<Comment> getComments() const { return m_comments; }

    // Load/Save comments
    bool loadCommentsFromXML(const QString& filePath);
    bool saveCommentsToXML(const QString& filePath);

    // Binding mode (share comments between applications)
    void setBindingMode(bool enabled);
    bool isBindingMode() const { return m_bindingMode; }

signals:
    void commentSelected(int64_t offset, int packetIndex, int frameIndex);
    void commentAdded(const Comment& comment);
    void commentRemoved(int commentId);

private slots:
    void onAddComment();
    void onEditComment();
    void onRemoveComment();
    void onResolveComment();
    void onLoadComments();
    void onSaveComments();
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onItemSelectionChanged();
    void onFilterChanged(const QString& filter);

private:
    void createUI();
    void updateCommentList();
    void addCommentToTree(const Comment& comment);
    int getNextCommentId();
    QString getCurrentUser();

    TSParser* m_parser;
    QVector<Comment> m_comments;
    int m_nextCommentId;
    bool m_bindingMode;

    // UI components
    QTreeWidget* m_treeWidget;
    QToolBar* m_toolbar;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_removeButton;
    QPushButton* m_resolveButton;
    QPushButton* m_loadButton;
    QPushButton* m_saveButton;
    QAction* m_bindingAction;
    QAction* m_filterErrorAction;
    QAction* m_filterWarningAction;
    QAction* m_filterInfoAction;
    QAction* m_showResolvedAction;
};

} // namespace VideoStudio

#endif // COMMENTSPANEL_H
