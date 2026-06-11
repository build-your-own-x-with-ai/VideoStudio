#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QComboBox>

namespace VideoStudio {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();

signals:
    void languageChanged(const QString& language);
    void themeChanged(const QString& theme);

private:
    void createUI();
    void loadSettings();
    void saveSettings();

    QComboBox* m_languageCombo;
    QComboBox* m_themeCombo;
};

} // namespace VideoStudio

#endif // SETTINGSDIALOG_H
