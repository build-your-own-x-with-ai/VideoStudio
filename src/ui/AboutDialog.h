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

class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);

private slots:
    void onImageDownloaded(QNetworkReply* reply);

private:
    QLabel* imageLabel;
    QNetworkAccessManager* networkManager;
};

#endif // ABOUTDIALOG_H
