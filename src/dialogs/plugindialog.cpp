#include "plugindialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QFile>
#include <QApplication>

namespace VideoStudio {

PluginDialog::PluginDialog(const QString& videoFile, QWidget* parent)
    : QDialog(parent)
    , m_videoFile(videoFile)
{
    setWindowTitle(tr("Plugin Manager"));
    resize(900, 600);

    setupUI();
    loadPlugins();
}

PluginDialog::~PluginDialog() = default;

void PluginDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Top: Video file label
    QLabel* fileLabel = new QLabel(tr("Video: %1").arg(m_videoFile), this);
    fileLabel->setStyleSheet("QLabel { font-weight: bold; padding: 5px; }");
    mainLayout->addWidget(fileLabel);

    // Splitter for three-panel layout
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // Left panel: Plugin list
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* pluginLabel = new QLabel(tr("Available Plugins:"), leftPanel);
    leftLayout->addWidget(pluginLabel);

    m_pluginList = new QListWidget(leftPanel);
    leftLayout->addWidget(m_pluginList);

    m_refreshButton = new QPushButton(tr("Refresh Plugins"), leftPanel);
    leftLayout->addWidget(m_refreshButton);

    splitter->addWidget(leftPanel);

    // Middle panel: Plugin details
    QWidget* middlePanel = new QWidget();
    QVBoxLayout* middleLayout = new QVBoxLayout(middlePanel);
    middleLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* detailsLabel = new QLabel(tr("Plugin Details:"), middlePanel);
    middleLayout->addWidget(detailsLabel);

    m_detailsText = new QTextEdit(middlePanel);
    m_detailsText->setReadOnly(true);
    middleLayout->addWidget(m_detailsText);

    m_runButton = new QPushButton(tr("Run Plugin"), middlePanel);
    m_runButton->setEnabled(false);
    middleLayout->addWidget(m_runButton);

    splitter->addWidget(middlePanel);

    // Right panel: Results
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* resultsLabel = new QLabel(tr("Analysis Results:"), rightPanel);
    rightLayout->addWidget(resultsLabel);

    m_resultsText = new QTextEdit(rightPanel);
    m_resultsText->setReadOnly(true);
    m_resultsText->setPlaceholderText(tr("Results will appear here after running a plugin..."));
    rightLayout->addWidget(m_resultsText);

    m_exportButton = new QPushButton(tr("Export Results"), rightPanel);
    m_exportButton->setEnabled(false);
    rightLayout->addWidget(m_exportButton);

    splitter->addWidget(rightPanel);

    // Set splitter sizes
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 2);

    mainLayout->addWidget(splitter);

    // Bottom: Status label
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("QLabel { padding: 5px; }");
    mainLayout->addWidget(m_statusLabel);

    // Bottom: Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton* closeButton = new QPushButton(tr("Close"), this);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(m_pluginList, &QListWidget::currentItemChanged,
            this, &PluginDialog::onPluginSelected);
    connect(m_runButton, &QPushButton::clicked,
            this, &PluginDialog::onRunPlugin);
    connect(m_exportButton, &QPushButton::clicked,
            this, &PluginDialog::onExportResults);
    connect(m_refreshButton, &QPushButton::clicked,
            this, &PluginDialog::onRefreshPlugins);
    connect(closeButton, &QPushButton::clicked,
            this, &QDialog::accept);
}

void PluginDialog::loadPlugins() {
    m_pluginList->clear();
    m_detailsText->clear();
    m_resultsText->clear();

    PluginManager& manager = PluginManager::instance();

    // Scan for plugins
    QString pluginsDir = manager.getPluginsDirectory();
    m_statusLabel->setText(tr("Scanning for plugins in: %1").arg(pluginsDir));

    manager.scanPlugins(pluginsDir);

    QVector<PluginMetadata> plugins = manager.getPluginMetadataList();

    if (plugins.isEmpty()) {
        m_statusLabel->setText(tr("No plugins found. Place plugin files in: %1").arg(pluginsDir));
        return;
    }

    // Populate plugin list
    for (const PluginMetadata& metadata : plugins) {
        QListWidgetItem* item = new QListWidgetItem(metadata.name);
        item->setData(Qt::UserRole, metadata.id);

        // Add category badge
        QString tooltip = QString("%1\nCategory: %2\nVersion: %3")
            .arg(metadata.description)
            .arg(metadata.category)
            .arg(metadata.version);
        item->setToolTip(tooltip);

        m_pluginList->addItem(item);
    }

    m_statusLabel->setText(tr("Found %1 plugin(s)").arg(plugins.size()));
}

void PluginDialog::onPluginSelected(QListWidgetItem* current, QListWidgetItem* previous) {
    Q_UNUSED(previous);

    if (!current) {
        m_runButton->setEnabled(false);
        m_detailsText->clear();
        return;
    }

    QString pluginId = current->data(Qt::UserRole).toString();
    m_currentPluginId = pluginId;
    m_runButton->setEnabled(true);

    updatePluginDetails(pluginId);
}

void PluginDialog::updatePluginDetails(const QString& pluginId) {
    PluginManager& manager = PluginManager::instance();
    IAnalyzerPlugin* plugin = manager.getPlugin(pluginId);

    if (!plugin) {
        m_detailsText->setPlainText(tr("Plugin not found."));
        return;
    }

    PluginMetadata metadata = plugin->getMetadata();

    QString details;
    details += QString("<h3>%1</h3>").arg(metadata.name);
    details += QString("<p><b>ID:</b> %1</p>").arg(metadata.id);
    details += QString("<p><b>Version:</b> %1</p>").arg(metadata.version);
    details += QString("<p><b>Author:</b> %1</p>").arg(metadata.author);
    details += QString("<p><b>Category:</b> %1</p>").arg(metadata.category);
    details += QString("<p><b>Description:</b> %1</p>").arg(metadata.description);

    if (!metadata.tags.isEmpty()) {
        details += QString("<p><b>Tags:</b> %1</p>").arg(metadata.tags.join(", "));
    }

    // Show default options
    QVariantMap options = plugin->getDefaultOptions();
    if (!options.isEmpty()) {
        details += "<p><b>Default Options:</b></p><ul>";
        for (auto it = options.constBegin(); it != options.constEnd(); ++it) {
            details += QString("<li>%1 = %2</li>")
                .arg(it.key())
                .arg(it.value().toString());
        }
        details += "</ul>";
    }

    // Show supported codecs/formats
    QStringList codecs = plugin->getSupportedCodecs();
    if (!codecs.isEmpty()) {
        details += QString("<p><b>Supported Codecs:</b> %1</p>").arg(codecs.join(", "));
    }

    QStringList formats = plugin->getSupportedFormats();
    if (!formats.isEmpty()) {
        details += QString("<p><b>Supported Formats:</b> %1</p>").arg(formats.join(", "));
    }

    m_detailsText->setHtml(details);
}

void PluginDialog::onRunPlugin() {
    if (m_currentPluginId.isEmpty()) {
        return;
    }

    m_statusLabel->setText(tr("Running plugin..."));
    m_runButton->setEnabled(false);
    m_resultsText->clear();

    QApplication::processEvents(); // Update UI

    runSelectedPlugin();

    m_runButton->setEnabled(true);
}

void PluginDialog::runSelectedPlugin() {
    PluginManager& manager = PluginManager::instance();

    // Use default options for now
    IAnalyzerPlugin* plugin = manager.getPlugin(m_currentPluginId);
    QVariantMap options = plugin ? plugin->getDefaultOptions() : QVariantMap();

    m_lastResult = manager.runPlugin(m_currentPluginId, m_videoFile, options);

    if (!m_lastResult.success) {
        m_statusLabel->setText(tr("Plugin failed: %1").arg(m_lastResult.error));
        m_resultsText->setPlainText(tr("Error: %1").arg(m_lastResult.error));
        m_exportButton->setEnabled(false);
        QMessageBox::critical(this, tr("Plugin Error"),
            tr("Plugin execution failed:\n%1").arg(m_lastResult.error));
        return;
    }

    // Display text report
    m_resultsText->setPlainText(m_lastResult.textReport);
    m_exportButton->setEnabled(true);
    m_statusLabel->setText(tr("Plugin completed successfully"));
}

void PluginDialog::onExportResults() {
    if (!m_lastResult.success) {
        return;
    }

    QString defaultName = QString("plugin_report_%1.json")
        .arg(m_currentPluginId.section('.', -1));

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Plugin Results"),
        defaultName,
        tr("JSON Files (*.json);;Text Files (*.txt);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export Error"),
            tr("Failed to open file for writing:\n%1").arg(fileName));
        return;
    }

    QTextStream out(&file);

    if (fileName.endsWith(".json")) {
        QJsonDocument doc(m_lastResult.data);
        out << doc.toJson(QJsonDocument::Indented);
    } else {
        out << m_lastResult.textReport;
    }

    file.close();

    m_statusLabel->setText(tr("Results exported to: %1").arg(fileName));
    QMessageBox::information(this, tr("Export Successful"),
        tr("Plugin results exported to:\n%1").arg(fileName));
}

void PluginDialog::onRefreshPlugins() {
    loadPlugins();
}

} // namespace VideoStudio
