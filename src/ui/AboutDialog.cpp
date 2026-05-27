#include "AboutDialog.h"
#include <QNetworkRequest>
#include <QUrl>

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("关于 VideoStudio");
    setFixedSize(500, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // Title
    QLabel* titleLabel = new QLabel("<h2>VideoStudio 1.0</h2>", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Description
    QLabel* descLabel = new QLabel(
        "<p>专业视频编解码分析工具</p>"
        "<p>基于 Qt 和 FFmpeg 开发</p>"
        "<p>用于深度分析视频流的编解码参数、比特率、帧类型、GOP 结构等专业指标</p>",
        this
    );
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);

    // Separator
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    // Author info
    QLabel* authorLabel = new QLabel("<p><b>作者:</b> AIDevLog</p>", this);
    authorLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(authorLabel);

    // GitHub link
    QLabel* githubLabel = new QLabel(
        "<p><b>GitHub:</b> <a href='https://github.com/build-your-own-x-with-ai/VideoStudio'>"
        "https://github.com/build-your-own-x-with-ai/VideoStudio</a></p>",
        this
    );
    githubLabel->setAlignment(Qt::AlignCenter);
    githubLabel->setOpenExternalLinks(true);
    githubLabel->setWordWrap(true);
    mainLayout->addWidget(githubLabel);

    // WeChat label
    QLabel* wechatLabel = new QLabel("<p><b>微信公众号:</b> AI开发日志</p>", this);
    wechatLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(wechatLabel);

    // Image placeholder
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(200, 200);
    imageLabel->setText("正在加载图片...");
    mainLayout->addWidget(imageLabel);

    mainLayout->addStretch();

    // OK button
    QPushButton* okButton = new QPushButton("确定", this);
    okButton->setDefault(true);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Download image
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &AboutDialog::onImageDownloaded);

    QNetworkRequest request(QUrl("https://2019.iosdevlog.com/uploads/AIDevLog.jpg"));
    networkManager->get(request);
}

void AboutDialog::onImageDownloaded(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();
        QPixmap pixmap;
        if (pixmap.loadFromData(imageData)) {
            // Scale image to fit
            QPixmap scaledPixmap = pixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            imageLabel->setPixmap(scaledPixmap);
        } else {
            imageLabel->setText("图片加载失败");
        }
    } else {
        imageLabel->setText("图片加载失败");
    }
    reply->deleteLater();
}
