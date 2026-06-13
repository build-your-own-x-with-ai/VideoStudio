#include "compliancedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QHeaderView>
#include <QJsonDocument>

namespace VideoStudio {

ComplianceDialog::ComplianceDialog(const QString& videoFile, QWidget* parent)
    : QDialog(parent)
    , m_videoFile(videoFile)
    , m_validator(new ComplianceValidator(this))
{
    setWindowTitle(tr("H.264/H.265 Compliance Validation"));
    resize(900, 700);
    setupUI();

    connect(m_validator, &ComplianceValidator::validationComplete,
            this, &ComplianceDialog::onValidationComplete);
}

ComplianceDialog::~ComplianceDialog() = default;

void ComplianceDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // File info
    QGroupBox* fileGroup = new QGroupBox(tr("Video File"));
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    QLabel* fileLabel = new QLabel(m_videoFile);
    fileLabel->setWordWrap(true);
    fileLayout->addWidget(fileLabel);
    mainLayout->addWidget(fileGroup);

    // Status
    m_statusLabel = new QLabel(tr("Ready to validate"));
    mainLayout->addWidget(m_statusLabel);

    // Summary
    m_summaryLabel = new QLabel();
    m_summaryLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 10px; border-radius: 5px; }");
    mainLayout->addWidget(m_summaryLabel);

    // Splitter for issues and details
    QSplitter* splitter = new QSplitter(Qt::Vertical);

    // Issues tree
    QGroupBox* issuesGroup = new QGroupBox(tr("Validation Issues"));
    QVBoxLayout* issuesLayout = new QVBoxLayout(issuesGroup);

    m_issuesTree = new QTreeWidget();
    m_issuesTree->setHeaderLabels({tr("Severity"), tr("Category"), tr("Description"), tr("Frame")});
    m_issuesTree->setAlternatingRowColors(true);
    m_issuesTree->setRootIsDecorated(false);
    m_issuesTree->header()->setStretchLastSection(false);
    m_issuesTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_issuesTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_issuesTree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_issuesTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    connect(m_issuesTree, &QTreeWidget::itemClicked,
            this, &ComplianceDialog::onIssueSelected);

    issuesLayout->addWidget(m_issuesTree);
    splitter->addWidget(issuesGroup);

    // Details panel
    QGroupBox* detailsGroup = new QGroupBox(tr("Issue Details"));
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsGroup);
    m_detailsText = new QTextEdit();
    m_detailsText->setReadOnly(true);
    detailsLayout->addWidget(m_detailsText);
    splitter->addWidget(detailsGroup);

    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_validateButton = new QPushButton(tr("Start Validation"));
    connect(m_validateButton, &QPushButton::clicked, this, &ComplianceDialog::startValidation);
    buttonLayout->addWidget(m_validateButton);

    m_exportButton = new QPushButton(tr("Export Report"));
    m_exportButton->setEnabled(false);
    connect(m_exportButton, &QPushButton::clicked, this, &ComplianceDialog::exportReport);
    buttonLayout->addWidget(m_exportButton);

    buttonLayout->addStretch();

    m_closeButton = new QPushButton(tr("Close"));
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);
}

void ComplianceDialog::startValidation() {
    m_validateButton->setEnabled(false);
    m_statusLabel->setText(tr("Validating..."));
    m_issuesTree->clear();
    m_detailsText->clear();

    // Run validation
    bool success = m_validator->validateFile(m_videoFile);

    if (!success) {
        QMessageBox::warning(this, tr("Validation Error"),
                             tr("Failed to validate file. See issues for details."));
    }

    m_statusLabel->setText(tr("Validation complete"));
    m_validateButton->setEnabled(true);
}

void ComplianceDialog::onValidationComplete() {
    updateSummary();
    populateIssuesTree();
    m_exportButton->setEnabled(true);
}

void ComplianceDialog::updateSummary() {
    int errors = m_validator->getErrorCount();
    int warnings = m_validator->getWarningCount();
    int info = m_validator->getInfoCount();

    QString summary = QString("<b>Total Issues:</b> %1 | ")
                          .arg(m_validator->getIssues().size());
    summary += QString("<span style='color: red;'><b>Errors:</b> %1</span> | ").arg(errors);
    summary += QString("<span style='color: orange;'><b>Warnings:</b> %1</span> | ").arg(warnings);
    summary += QString("<span style='color: blue;'><b>Info:</b> %1</span>").arg(info);

    m_summaryLabel->setText(summary);
}

void ComplianceDialog::populateIssuesTree() {
    m_issuesTree->clear();

    for (const auto& issue : m_validator->getIssues()) {
        QTreeWidgetItem* item = new QTreeWidgetItem();

        // Severity
        QString severityText;
        QColor severityColor;
        switch (issue.severity) {
            case IssueSeverity::Critical:
                severityText = tr("CRITICAL");
                severityColor = QColor(139, 0, 0); // Dark red
                break;
            case IssueSeverity::Error:
                severityText = tr("ERROR");
                severityColor = QColor(220, 20, 60); // Crimson
                break;
            case IssueSeverity::Warning:
                severityText = tr("WARNING");
                severityColor = QColor(255, 140, 0); // Dark orange
                break;
            case IssueSeverity::Info:
                severityText = tr("INFO");
                severityColor = QColor(70, 130, 180); // Steel blue
                break;
        }

        item->setText(0, severityText);
        item->setForeground(0, severityColor);
        item->setFont(0, QFont("", -1, QFont::Bold));

        // Category
        QString categoryText;
        switch (issue.category) {
            case IssueCategory::SPS: categoryText = tr("SPS"); break;
            case IssueCategory::PPS: categoryText = tr("PPS"); break;
            case IssueCategory::VPS: categoryText = tr("VPS"); break;
            case IssueCategory::SliceHeader: categoryText = tr("Slice"); break;
            case IssueCategory::NALUnit: categoryText = tr("NAL"); break;
            case IssueCategory::Bitstream: categoryText = tr("Bitstream"); break;
            case IssueCategory::Profile: categoryText = tr("Profile"); break;
            case IssueCategory::Timing: categoryText = tr("Timing"); break;
        }
        item->setText(1, categoryText);

        // Description
        item->setText(2, issue.description);

        // Frame number
        if (issue.frameNumber >= 0) {
            item->setText(3, QString::number(issue.frameNumber));
        } else {
            item->setText(3, "-");
        }

        // Store full issue data
        item->setData(0, Qt::UserRole, QVariant::fromValue(issue));

        m_issuesTree->addTopLevelItem(item);
    }
}

void ComplianceDialog::onIssueSelected(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);

    ComplianceIssue issue = item->data(0, Qt::UserRole).value<ComplianceIssue>();

    QString details;
    details += QString("<h3>%1</h3>").arg(item->text(2));
    details += QString("<p><b>Severity:</b> %1</p>").arg(item->text(0));
    details += QString("<p><b>Category:</b> %1</p>").arg(item->text(1));

    if (!issue.standard.isEmpty()) {
        details += QString("<p><b>Standard Reference:</b> %1</p>").arg(issue.standard);
    }

    if (!issue.suggestion.isEmpty()) {
        details += QString("<p><b>Suggestion:</b> %1</p>").arg(issue.suggestion);
    }

    if (issue.frameNumber >= 0) {
        details += QString("<p><b>Frame Number:</b> %1</p>").arg(issue.frameNumber);
    }

    if (issue.byteOffset >= 0) {
        details += QString("<p><b>Byte Offset:</b> %1</p>").arg(issue.byteOffset);
    }

    m_detailsText->setHtml(details);
}

void ComplianceDialog::exportReport() {
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export Compliance Report"),
        QDir::homePath() + "/compliance_report.json",
        tr("JSON Files (*.json);;Text Files (*.txt);;All Files (*)")
    );

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Error"),
                             tr("Failed to open file for writing"));
        return;
    }

    QTextStream out(&file);

    if (fileName.endsWith(".json", Qt::CaseInsensitive)) {
        QJsonDocument doc(m_validator->toJson());
        out << doc.toJson(QJsonDocument::Indented);
    } else {
        out << m_validator->toTextReport();
    }

    file.close();

    QMessageBox::information(this, tr("Export Complete"),
                             tr("Compliance report exported successfully"));
}

} // namespace VideoStudio
