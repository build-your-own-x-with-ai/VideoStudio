#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace VideoStudio {

class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);

private slots:
    void onImageDownloaded(QNetworkReply* reply);
    void openGitHub();
    void openReadme();

private:
    QLabel* m_imageLabel;
    QNetworkAccessManager* m_networkManager;
};

} // namespace VideoStudio

#endif // ABOUTDIALOG_H
