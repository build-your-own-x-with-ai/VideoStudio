#ifndef SAVESTREAMINFODIALOG_H
#define SAVESTREAMINFODIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QCheckBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>

namespace VideoStudio {

class TSParser;

class SaveStreamInfoDialog : public QDialog {
    Q_OBJECT

public:
    explicit SaveStreamInfoDialog(TSParser* parser, QWidget* parent = nullptr);
    ~SaveStreamInfoDialog();

private slots:
    void onBrowseStreamInfo();
    void onBrowseDump();
    void onSaveStreamInfo();
    void onSaveDump();

private:
    void createUI();
    void createStreamInfoTab();
    void createDumpTab();

    TSParser* m_parser;

    // Stream Info tab widgets
    QCheckBox* m_streamStructureCheck;
    QCheckBox* m_fullStreamInfoCheck;
    QRadioButton* m_rangeAllRadio;
    QRadioButton* m_rangeOffsetRadio;
    QLineEdit* m_offsetStartEdit;
    QLineEdit* m_offsetEndEdit;
    QCheckBox* m_messagesCheck;
    QCheckBox* m_messageDetailsCheck;
    QCheckBox* m_tr101290Check;
    QCheckBox* m_tr101290DetailsCheck;
    QCheckBox* m_headersCheck;
    QCheckBox* m_visibleOnlyCheck;
    QCheckBox* m_headerDetailsCheck;
    QLineEdit* m_streamInfoPathEdit;
    QPushButton* m_streamInfoBrowseButton;
    QPushButton* m_streamInfoSaveButton;

    // Dump tab widgets
    QComboBox* m_dumpStreamCombo;
    QRadioButton* m_dumpRangeAllRadio;
    QRadioButton* m_dumpRangeOffsetRadio;
    QLineEdit* m_dumpOffsetStartEdit;
    QLineEdit* m_dumpOffsetEndEdit;
    QLineEdit* m_dumpPathEdit;
    QPushButton* m_dumpBrowseButton;
    QPushButton* m_dumpSaveButton;

    QTabWidget* m_tabWidget;
};

} // namespace VideoStudio

#endif // SAVESTREAMINFODIALOG_H
