#ifndef PLUGINDIALOG_H
#define PLUGINDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include "plugins/pluginmanager.h"

namespace VideoStudio {

class PluginDialog : public QDialog {
    Q_OBJECT

public:
    explicit PluginDialog(const QString& videoFile, QWidget* parent = nullptr);
    ~PluginDialog() override;

private slots:
    void onPluginSelected(QListWidgetItem* current, QListWidgetItem* previous);
    void onRunPlugin();
    void onExportResults();
    void onRefreshPlugins();

private:
    void setupUI();
    void loadPlugins();
    void updatePluginDetails(const QString& pluginId);
    void runSelectedPlugin();

    QString m_videoFile;
    QString m_currentPluginId;
    AnalysisResult m_lastResult;

    // UI components
    QListWidget* m_pluginList;
    QTextEdit* m_detailsText;
    QTextEdit* m_resultsText;
    QPushButton* m_runButton;
    QPushButton* m_exportButton;
    QPushButton* m_refreshButton;
    QLabel* m_statusLabel;
};

} // namespace VideoStudio

#endif // PLUGINDIALOG_H
