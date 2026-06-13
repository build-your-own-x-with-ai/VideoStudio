#ifndef COMPLIANCEDIALOG_H
#define COMPLIANCEDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QTreeWidget>
#include <QPushButton>
#include <QLabel>
#include "core/compliancevalidator.h"

namespace VideoStudio {

class ComplianceDialog : public QDialog {
    Q_OBJECT

public:
    explicit ComplianceDialog(const QString& videoFile, QWidget* parent = nullptr);
    ~ComplianceDialog();

private slots:
    void startValidation();
    void onValidationComplete();
    void exportReport();
    void onIssueSelected(QTreeWidgetItem* item, int column);

private:
    void setupUI();
    void updateSummary();
    void populateIssuesTree();

    QString m_videoFile;
    ComplianceValidator* m_validator;

    QLabel* m_statusLabel;
    QLabel* m_summaryLabel;
    QTreeWidget* m_issuesTree;
    QTextEdit* m_detailsText;
    QPushButton* m_validateButton;
    QPushButton* m_exportButton;
    QPushButton* m_closeButton;
};

} // namespace VideoStudio

#endif // COMPLIANCEDIALOG_H
