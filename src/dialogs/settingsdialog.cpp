#include "dialogs/settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>

namespace VideoStudio {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    setModal(true);
    resize(400, 200);

    createUI();
    loadSettings();
}

SettingsDialog::~SettingsDialog() {
}

void SettingsDialog::createUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QFormLayout* formLayout = new QFormLayout();

    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItem("English", "en");
    m_languageCombo->addItem("简体中文", "zh_CN");
    formLayout->addRow(tr("Language:"), m_languageCombo);

    m_themeCombo = new QComboBox(this);
    m_themeCombo->addItem(tr("Dark"), "dark");
    m_themeCombo->addItem(tr("Light"), "light");
    formLayout->addRow(tr("Theme:"), m_themeCombo);

    mainLayout->addLayout(formLayout);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton* okButton = new QPushButton(tr("OK"), this);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), this);

    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &SettingsDialog::saveSettings);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void SettingsDialog::loadSettings() {
    QSettings settings("VideoStudio", "VideoStudio");

    QString language = settings.value("language", "en").toString();
    int langIndex = m_languageCombo->findData(language);
    if (langIndex >= 0) {
        m_languageCombo->setCurrentIndex(langIndex);
    }

    QString theme = settings.value("theme", "dark").toString();
    int themeIndex = m_themeCombo->findData(theme);
    if (themeIndex >= 0) {
        m_themeCombo->setCurrentIndex(themeIndex);
    }
}

void SettingsDialog::saveSettings() {
    QSettings settings("VideoStudio", "VideoStudio");

    QString language = m_languageCombo->currentData().toString();
    QString theme = m_themeCombo->currentData().toString();

    QString oldLanguage = settings.value("language", "en").toString();
    QString oldTheme = settings.value("theme", "dark").toString();

    settings.setValue("language", language);
    settings.setValue("theme", theme);

    if (language != oldLanguage) {
        emit languageChanged(language);
    }

    if (theme != oldTheme) {
        emit themeChanged(theme);
    }

    accept();
}

} // namespace VideoStudio
