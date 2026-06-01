#include "panels/commentspanel.h"
#include "core/tsparser.h"
#include <QHeaderView>
#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QFile>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>

namespace VideoStudio {

CommentsPanel::CommentsPanel(QWidget* parent)
    : QWidget(parent)
    , m_parser(nullptr)
    , m_nextCommentId(1)
    , m_bindingMode(false)
{
    createUI();
}

CommentsPanel::~CommentsPanel() {
}

void CommentsPanel::createUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Toolbar
    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(16, 16));

    m_addButton = new QPushButton("Add", this);
    connect(m_addButton, &QPushButton::clicked, this, &CommentsPanel::onAddComment);
    m_toolbar->addWidget(m_addButton);

    m_editButton = new QPushButton("Edit", this);
    m_editButton->setEnabled(false);
    connect(m_editButton, &QPushButton::clicked, this, &CommentsPanel::onEditComment);
    m_toolbar->addWidget(m_editButton);

    m_removeButton = new QPushButton("Remove", this);
    m_removeButton->setEnabled(false);
    connect(m_removeButton, &QPushButton::clicked, this, &CommentsPanel::onRemoveComment);
    m_toolbar->addWidget(m_removeButton);

    m_resolveButton = new QPushButton("Resolve", this);
    m_resolveButton->setEnabled(false);
    connect(m_resolveButton, &QPushButton::clicked, this, &CommentsPanel::onResolveComment);
    m_toolbar->addWidget(m_resolveButton);

    m_toolbar->addSeparator();

    m_loadButton = new QPushButton("Load", this);
    connect(m_loadButton, &QPushButton::clicked, this, &CommentsPanel::onLoadComments);
    m_toolbar->addWidget(m_loadButton);

    m_saveButton = new QPushButton("Save", this);
    connect(m_saveButton, &QPushButton::clicked, this, &CommentsPanel::onSaveComments);
    m_toolbar->addWidget(m_saveButton);

    m_toolbar->addSeparator();

    // Binding mode action
    m_bindingAction = m_toolbar->addAction("Binding");
    m_bindingAction->setCheckable(true);
    m_bindingAction->setToolTip("Share comments between applications");
    connect(m_bindingAction, &QAction::triggered, this, [this](bool checked) {
        setBindingMode(checked);
    });

    m_toolbar->addSeparator();

    // Filter actions
    m_filterErrorAction = m_toolbar->addAction("Errors");
    m_filterErrorAction->setCheckable(true);
    m_filterErrorAction->setChecked(true);

    m_filterWarningAction = m_toolbar->addAction("Warnings");
    m_filterWarningAction->setCheckable(true);
    m_filterWarningAction->setChecked(true);

    m_filterInfoAction = m_toolbar->addAction("Info");
    m_filterInfoAction->setCheckable(true);
    m_filterInfoAction->setChecked(true);

    m_showResolvedAction = m_toolbar->addAction("Show Resolved");
    m_showResolvedAction->setCheckable(true);
    m_showResolvedAction->setChecked(false);

    connect(m_filterErrorAction, &QAction::triggered, this, [this]() { updateCommentList(); });
    connect(m_filterWarningAction, &QAction::triggered, this, [this]() { updateCommentList(); });
    connect(m_filterInfoAction, &QAction::triggered, this, [this]() { updateCommentList(); });
    connect(m_showResolvedAction, &QAction::triggered, this, [this]() { updateCommentList(); });

    mainLayout->addWidget(m_toolbar);

    // Tree widget
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels(QStringList() << "ID" << "Category" << "Author" << "Text" << "Offset" << "Time" << "Status");
    m_treeWidget->setColumnWidth(0, 50);
    m_treeWidget->setColumnWidth(1, 80);
    m_treeWidget->setColumnWidth(2, 100);
    m_treeWidget->setColumnWidth(3, 300);
    m_treeWidget->setColumnWidth(4, 100);
    m_treeWidget->setColumnWidth(5, 150);
    m_treeWidget->setColumnWidth(6, 80);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &CommentsPanel::onItemDoubleClicked);
    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged,
            this, &CommentsPanel::onItemSelectionChanged);

    mainLayout->addWidget(m_treeWidget);
}

void CommentsPanel::setTSParser(TSParser* parser) {
    m_parser = parser;
}

void CommentsPanel::clear() {
    m_parser = nullptr;
    m_comments.clear();
    m_treeWidget->clear();
    m_nextCommentId = 1;
}

void CommentsPanel::addComment(const Comment& comment) {
    Comment newComment = comment;
    if (newComment.id == 0) {
        newComment.id = getNextCommentId();
    }
    if (newComment.timestamp.isNull()) {
        newComment.timestamp = QDateTime::currentDateTime();
    }
    if (newComment.author.isEmpty()) {
        newComment.author = getCurrentUser();
    }

    m_comments.append(newComment);
    addCommentToTree(newComment);

    emit commentAdded(newComment);
    qDebug() << "Comment added:" << newComment.id << newComment.text;
}

void CommentsPanel::removeComment(int commentId) {
    for (int i = 0; i < m_comments.size(); ++i) {
        if (m_comments[i].id == commentId) {
            m_comments.removeAt(i);
            updateCommentList();
            emit commentRemoved(commentId);
            qDebug() << "Comment removed:" << commentId;
            return;
        }
    }
}

void CommentsPanel::updateComment(int commentId, const Comment& comment) {
    for (int i = 0; i < m_comments.size(); ++i) {
        if (m_comments[i].id == commentId) {
            m_comments[i] = comment;
            m_comments[i].id = commentId; // Preserve ID
            updateCommentList();
            qDebug() << "Comment updated:" << commentId;
            return;
        }
    }
}

void CommentsPanel::setBindingMode(bool enabled) {
    m_bindingMode = enabled;
    m_bindingAction->setChecked(enabled);
    qDebug() << "Binding mode:" << (enabled ? "enabled" : "disabled");
}

void CommentsPanel::updateCommentList() {
    m_treeWidget->clear();

    for (const Comment& comment : m_comments) {
        // Apply filters
        if (!m_showResolvedAction->isChecked() && comment.resolved) {
            continue;
        }

        if (comment.category == "Error" && !m_filterErrorAction->isChecked()) {
            continue;
        }
        if (comment.category == "Warning" && !m_filterWarningAction->isChecked()) {
            continue;
        }
        if (comment.category == "Info" && !m_filterInfoAction->isChecked()) {
            continue;
        }

        addCommentToTree(comment);
    }
}

void CommentsPanel::addCommentToTree(const Comment& comment) {
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, QString::number(comment.id));
    item->setText(1, comment.category);
    item->setText(2, comment.author);
    item->setText(3, comment.text);
    item->setText(4, QString("0x%1").arg(comment.offset, 0, 16));
    item->setText(5, comment.timestamp.toString("yyyy-MM-dd HH:mm:ss"));
    item->setText(6, comment.resolved ? "Resolved" : "Open");

    // Color code by category
    if (comment.category == "Error") {
        item->setForeground(1, QBrush(QColor(255, 100, 100)));
    } else if (comment.category == "Warning") {
        item->setForeground(1, QBrush(QColor(255, 200, 100)));
    } else {
        item->setForeground(1, QBrush(QColor(100, 200, 255)));
    }

    // Gray out resolved comments
    if (comment.resolved) {
        for (int i = 0; i < 7; ++i) {
            item->setForeground(i, QBrush(QColor(128, 128, 128)));
        }
    }

    item->setData(0, Qt::UserRole, comment.id);
    m_treeWidget->addTopLevelItem(item);
}

int CommentsPanel::getNextCommentId() {
    return m_nextCommentId++;
}

QString CommentsPanel::getCurrentUser() {
    // Get current user from environment
    QString user = qgetenv("USER");
    if (user.isEmpty()) {
        user = qgetenv("USERNAME");
    }
    if (user.isEmpty()) {
        user = "Unknown";
    }
    return user;
}

void CommentsPanel::onAddComment() {
    if (!m_parser) {
        QMessageBox::warning(this, "No File", "Please open a TS file first.");
        return;
    }

    // Create dialog for adding comment
    QDialog dialog(this);
    dialog.setWindowTitle("Add Comment");
    dialog.resize(400, 250);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Category
    QHBoxLayout* categoryLayout = new QHBoxLayout();
    categoryLayout->addWidget(new QLabel("Category:", &dialog));
    QComboBox* categoryCombo = new QComboBox(&dialog);
    categoryCombo->addItems(QStringList() << "Info" << "Warning" << "Error");
    categoryLayout->addWidget(categoryCombo);
    layout->addLayout(categoryLayout);

    // Offset
    QHBoxLayout* offsetLayout = new QHBoxLayout();
    offsetLayout->addWidget(new QLabel("Offset:", &dialog));
    QLineEdit* offsetEdit = new QLineEdit("0", &dialog);
    offsetLayout->addWidget(offsetEdit);
    layout->addLayout(offsetLayout);

    // Text
    layout->addWidget(new QLabel("Comment:", &dialog));
    QLineEdit* textEdit = new QLineEdit(&dialog);
    layout->addWidget(textEdit);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("OK", &dialog);
    QPushButton* cancelButton = new QPushButton("Cancel", &dialog);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    if (dialog.exec() == QDialog::Accepted) {
        Comment comment;
        comment.id = 0;  // Will be assigned by addComment()
        comment.category = categoryCombo->currentText();
        comment.text = textEdit->text();
        comment.offset = offsetEdit->text().toLongLong(nullptr, 0);
        comment.packetIndex = -1;
        comment.frameIndex = -1;
        comment.resolved = false;

        addComment(comment);
    }
}

void CommentsPanel::onEditComment() {
    QTreeWidgetItem* item = m_treeWidget->currentItem();
    if (!item) return;

    int commentId = item->data(0, Qt::UserRole).toInt();
    Comment* comment = nullptr;
    for (Comment& c : m_comments) {
        if (c.id == commentId) {
            comment = &c;
            break;
        }
    }

    if (!comment) return;

    // Create edit dialog (similar to add dialog)
    QDialog dialog(this);
    dialog.setWindowTitle("Edit Comment");
    dialog.resize(400, 250);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Category
    QHBoxLayout* categoryLayout = new QHBoxLayout();
    categoryLayout->addWidget(new QLabel("Category:", &dialog));
    QComboBox* categoryCombo = new QComboBox(&dialog);
    categoryCombo->addItems(QStringList() << "Info" << "Warning" << "Error");
    categoryCombo->setCurrentText(comment->category);
    categoryLayout->addWidget(categoryCombo);
    layout->addLayout(categoryLayout);

    // Offset
    QHBoxLayout* offsetLayout = new QHBoxLayout();
    offsetLayout->addWidget(new QLabel("Offset:", &dialog));
    QLineEdit* offsetEdit = new QLineEdit(QString("0x%1").arg(comment->offset, 0, 16), &dialog);
    offsetLayout->addWidget(offsetEdit);
    layout->addLayout(offsetLayout);

    // Text
    layout->addWidget(new QLabel("Comment:", &dialog));
    QLineEdit* textEdit = new QLineEdit(comment->text, &dialog);
    layout->addWidget(textEdit);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("OK", &dialog);
    QPushButton* cancelButton = new QPushButton("Cancel", &dialog);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    if (dialog.exec() == QDialog::Accepted) {
        comment->category = categoryCombo->currentText();
        comment->text = textEdit->text();
        comment->offset = offsetEdit->text().toLongLong(nullptr, 0);
        updateCommentList();
    }
}

void CommentsPanel::onRemoveComment() {
    QTreeWidgetItem* item = m_treeWidget->currentItem();
    if (!item) return;

    int commentId = item->data(0, Qt::UserRole).toInt();

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Remove Comment",
        "Are you sure you want to remove this comment?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        removeComment(commentId);
    }
}

void CommentsPanel::onResolveComment() {
    QTreeWidgetItem* item = m_treeWidget->currentItem();
    if (!item) return;

    int commentId = item->data(0, Qt::UserRole).toInt();

    for (Comment& comment : m_comments) {
        if (comment.id == commentId) {
            comment.resolved = !comment.resolved;
            updateCommentList();
            qDebug() << "Comment" << commentId << (comment.resolved ? "resolved" : "reopened");
            break;
        }
    }
}

void CommentsPanel::onLoadComments() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Load Comments"), QString(),
        tr("XML Files (*.xml);;All Files (*)"));

    if (!fileName.isEmpty()) {
        if (loadCommentsFromXML(fileName)) {
            QMessageBox::information(this, "Success",
                QString("Loaded %1 comments").arg(m_comments.size()));
        } else {
            QMessageBox::warning(this, "Error", "Failed to load comments");
        }
    }
}

void CommentsPanel::onSaveComments() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Comments"), QString(),
        tr("XML Files (*.xml);;All Files (*)"));

    if (!fileName.isEmpty()) {
        if (saveCommentsToXML(fileName)) {
            QMessageBox::information(this, "Success",
                QString("Saved %1 comments").arg(m_comments.size()));
        } else {
            QMessageBox::warning(this, "Error", "Failed to save comments");
        }
    }
}

void CommentsPanel::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);

    int commentId = item->data(0, Qt::UserRole).toInt();

    for (const Comment& comment : m_comments) {
        if (comment.id == commentId) {
            emit commentSelected(comment.offset, comment.packetIndex, comment.frameIndex);
            qDebug() << "Comment selected:" << commentId << "offset:" << comment.offset;
            break;
        }
    }
}

void CommentsPanel::onItemSelectionChanged() {
    bool hasSelection = m_treeWidget->currentItem() != nullptr;
    m_editButton->setEnabled(hasSelection);
    m_removeButton->setEnabled(hasSelection);
    m_resolveButton->setEnabled(hasSelection);
}

void CommentsPanel::onFilterChanged(const QString& filter) {
    Q_UNUSED(filter);
    updateCommentList();
}

bool CommentsPanel::loadCommentsFromXML(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for reading:" << filePath;
        return false;
    }

    m_comments.clear();
    m_treeWidget->clear();

    QXmlStreamReader xml(&file);

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == QString("comment")) {
            Comment comment;
            comment.id = xml.attributes().value("id").toInt();
            comment.author = xml.attributes().value("author").toString();
            comment.category = xml.attributes().value("category").toString();
            comment.offset = xml.attributes().value("offset").toLongLong();
            comment.packetIndex = xml.attributes().value("packetIndex").toInt();
            comment.frameIndex = xml.attributes().value("frameIndex").toInt();
            comment.resolved = xml.attributes().value("resolved").toString() == "true";
            comment.timestamp = QDateTime::fromString(
                xml.attributes().value("timestamp").toString(), Qt::ISODate);
            comment.text = xml.readElementText();

            m_comments.append(comment);

            if (comment.id >= m_nextCommentId) {
                m_nextCommentId = comment.id + 1;
            }
        }
    }

    file.close();

    if (xml.hasError()) {
        qDebug() << "XML parsing error:" << xml.errorString();
        return false;
    }

    updateCommentList();
    qDebug() << "Loaded" << m_comments.size() << "comments from" << filePath;
    return true;
}

bool CommentsPanel::saveCommentsToXML(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    xml.writeStartElement("comments");
    xml.writeAttribute("version", "1.0");

    for (const Comment& comment : m_comments) {
        xml.writeStartElement("comment");
        xml.writeAttribute("id", QString::number(comment.id));
        xml.writeAttribute("author", comment.author);
        xml.writeAttribute("category", comment.category);
        xml.writeAttribute("offset", QString::number(comment.offset));
        xml.writeAttribute("packetIndex", QString::number(comment.packetIndex));
        xml.writeAttribute("frameIndex", QString::number(comment.frameIndex));
        xml.writeAttribute("resolved", comment.resolved ? "true" : "false");
        xml.writeAttribute("timestamp", comment.timestamp.toString(Qt::ISODate));
        xml.writeCharacters(comment.text);
        xml.writeEndElement(); // comment
    }

    xml.writeEndElement(); // comments
    xml.writeEndDocument();

    file.close();

    qDebug() << "Saved" << m_comments.size() << "comments to" << filePath;
    return true;
}

} // namespace VideoStudio
