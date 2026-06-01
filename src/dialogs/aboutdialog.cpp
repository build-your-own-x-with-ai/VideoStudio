#include "aboutdialog.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QDesktopServices>
#include <QTextBrowser>
#include <QTabWidget>
#include <QFile>
#include <QTextStream>

namespace VideoStudio {

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
    , m_imageLabel(nullptr)
    , m_networkManager(nullptr)
{
    setWindowTitle(tr("关于 VideoStudio"));
    setMinimumSize(600, 700);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // Create tab widget
    QTabWidget* tabWidget = new QTabWidget(this);

    // ===== About Tab =====
    QWidget* aboutTab = new QWidget();
    QVBoxLayout* aboutLayout = new QVBoxLayout(aboutTab);
    aboutLayout->setSpacing(15);
    aboutLayout->setContentsMargins(20, 20, 20, 20);

    // Title
    QLabel* titleLabel = new QLabel("<h2>VideoStudio</h2>", aboutTab);
    titleLabel->setAlignment(Qt::AlignCenter);
    aboutLayout->addWidget(titleLabel);

    // Version
    QLabel* versionLabel = new QLabel("<p><b>版本:</b> 1.1</p>", aboutTab);
    versionLabel->setAlignment(Qt::AlignCenter);
    aboutLayout->addWidget(versionLabel);

    // Description
    QLabel* descLabel = new QLabel(
        "<p>专业视频流分析工具</p>"
        "<p>基于 Qt 6 和 FFmpeg 7 开发</p>"
        "<p>提供视频编解码分析、传输流分析、质量指标评估等专业功能</p>",
        aboutTab
    );
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    aboutLayout->addWidget(descLabel);

    // Separator
    QFrame* line1 = new QFrame(aboutTab);
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    aboutLayout->addWidget(line1);

    // Features
    QLabel* featuresLabel = new QLabel(
        "<p><b>主要功能:</b></p>"
        "<ul>"
        "<li>视频解码与帧级分析</li>"
        "<li>传输流分析 (MPEG-TS, TR 101-290)</li>"
        "<li>容器格式解析 (MP4, MKV, AVI, FLV)</li>"
        "<li>质量指标分析 (PSNR, SSIM)</li>"
        "<li>GOP 结构可视化</li>"
        "<li>比特率分析与可视化</li>"
        "<li>YUV 帧导出</li>"
        "<li>PTS/DTS/PCR 时序分析</li>"
        "</ul>",
        aboutTab
    );
    featuresLabel->setWordWrap(true);
    aboutLayout->addWidget(featuresLabel);

    // Separator
    QFrame* line2 = new QFrame(aboutTab);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    aboutLayout->addWidget(line2);

    // Author info
    QLabel* authorLabel = new QLabel("<p><b>作者:</b> AIDevLog</p>", aboutTab);
    authorLabel->setAlignment(Qt::AlignCenter);
    aboutLayout->addWidget(authorLabel);

    // GitHub link
    QLabel* githubLabel = new QLabel(
        "<p><b>GitHub:</b> <a href='https://github.com/build-your-own-x-with-ai/VideoStudio'>"
        "https://github.com/build-your-own-x-with-ai/VideoStudio</a></p>",
        aboutTab
    );
    githubLabel->setAlignment(Qt::AlignCenter);
    githubLabel->setOpenExternalLinks(true);
    githubLabel->setWordWrap(true);
    aboutLayout->addWidget(githubLabel);

    // WeChat label
    QLabel* wechatLabel = new QLabel("<p><b>微信公众号:</b> AI开发日志</p>", aboutTab);
    wechatLabel->setAlignment(Qt::AlignCenter);
    aboutLayout->addWidget(wechatLabel);

    // Image placeholder
    m_imageLabel = new QLabel(aboutTab);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setMinimumSize(200, 200);
    m_imageLabel->setText(tr("正在加载图片..."));
    aboutLayout->addWidget(m_imageLabel);

    aboutLayout->addStretch();

    tabWidget->addTab(aboutTab, tr("关于"));

    // ===== Help Tab =====
    QWidget* helpTab = new QWidget();
    QVBoxLayout* helpLayout = new QVBoxLayout(helpTab);
    helpLayout->setContentsMargins(0, 0, 0, 0);

    QTextBrowser* helpBrowser = new QTextBrowser(helpTab);
    helpBrowser->setOpenExternalLinks(true);
    helpBrowser->setHtml(
        "<h2>VideoStudio 使用帮助</h2>"

        "<h3>快速开始</h3>"
        "<ol>"
        "<li><b>打开视频文件:</b> 文件 → 打开 (Ctrl+O)</li>"
        "<li><b>查看日志:</b> 加载过程中，日志查看器会显示在窗口中央</li>"
        "<li><b>视频播放:</b> 使用播放/暂停按钮或按空格键</li>"
        "<li><b>帧导航:</b> Alt+左/右箭头逐帧前进/后退</li>"
        "</ol>"

        "<h3>主要功能</h3>"

        "<h4>1. 视频分析</h4>"
        "<ul>"
        "<li><b>比特率图表:</b> 顶部显示每帧大小和类型（红色=I帧，蓝色=P帧，绿色=B帧）</li>"
        "<li><b>GOP 结构:</b> 查看 GOP 结构和关键帧分布</li>"
        "<li><b>缩略图导航:</b> 底部缩略图栏快速跳转到任意帧</li>"
        "<li><b>流信息面板:</b> 右侧显示编解码器、分辨率、帧率等详细信息</li>"
        "</ul>"

        "<h4>2. 传输流分析 (TS 文件)</h4>"
        "<ul>"
        "<li><b>Explorer 面板:</b> 左侧显示 TS 流的层次结构（程序、PID、流类型）</li>"
        "<li><b>Packet List:</b> 中央标签页显示所有 TS 包（偏移、PID、类型）</li>"
        "<li><b>Property 面板:</b> 右侧显示选中包的详细信息</li>"
        "<li><b>Hex Viewer:</b> 底部显示原始十六进制数据</li>"
        "<li><b>TR 101-290 面板:</b> 显示传输流合规性检查结果</li>"
        "<li><b>Time Dynamics:</b> 显示 PTS/DTS/PCR 时序分析图表</li>"
        "<li><b>Messages 面板:</b> 汇总所有错误和警告信息</li>"
        "</ul>"

        "<h4>3. 质量指标分析</h4>"
        "<ul>"
        "<li><b>打开参考视频:</b> 文件 → 打开</li>"
        "<li><b>启动分析:</b> 工具 → 质量指标 (PSNR/SSIM) 或按 Ctrl+Q</li>"
        "<li><b>选择失真视频:</b> 选择要比较的压缩/失真版本</li>"
        "<li><b>配置参数:</b> 设置帧范围和要计算的指标</li>"
        "<li><b>查看结果:</b>"
        "  <ul>"
        "  <li>PSNR: >40 dB (优秀), 30-40 dB (良好), <30 dB (较差)</li>"
        "  <li>SSIM: >0.95 (优秀), 0.85-0.95 (良好), <0.85 (较差)</li>"
        "  </ul>"
        "</li>"
        "</ul>"
        "<p><b>注意:</b> 两个视频必须是相同内容的不同质量版本，且分辨率必须相同。</p>"

        "<h4>4. YUV 帧导出</h4>"
        "<ul>"
        "<li><b>导出当前帧:</b> 文件 → 导出帧为 YUV (Ctrl+E)</li>"
        "<li><b>导出帧范围:</b> 文件 → 导出帧范围为 YUV (Ctrl+Shift+E)</li>"
        "<li>选择输出位置和格式</li>"
        "</ul>"

        "<h3>快捷键</h3>"
        "<table border='1' cellpadding='5' cellspacing='0'>"
        "<tr><th>功能</th><th>快捷键</th></tr>"
        "<tr><td>打开文件</td><td>Ctrl+O</td></tr>"
        "<tr><td>播放/暂停</td><td>空格</td></tr>"
        "<tr><td>下一帧</td><td>Alt+右箭头</td></tr>"
        "<tr><td>上一帧</td><td>Alt+左箭头</td></tr>"
        "<tr><td>质量指标分析</td><td>Ctrl+Q</td></tr>"
        "<tr><td>导出当前帧</td><td>Ctrl+E</td></tr>"
        "<tr><td>导出帧范围</td><td>Ctrl+Shift+E</td></tr>"
        "<tr><td>保存流信息</td><td>Ctrl+S</td></tr>"
        "<tr><td>运动矢量叠加</td><td>Alt+3</td></tr>"
        "<tr><td>分区叠加</td><td>Alt+2</td></tr>"
        "<tr><td>帧类型叠加</td><td>Alt+4</td></tr>"
        "</table>"

        "<h3>技术支持</h3>"
        "<p>如有问题或建议，请访问:</p>"
        "<ul>"
        "<li><b>GitHub:</b> <a href='https://github.com/build-your-own-x-with-ai/VideoStudio'>"
        "https://github.com/build-your-own-x-with-ai/VideoStudio</a></li>"
        "<li><b>文档:</b> 查看项目根目录的 README.md 和 CLAUDE.md</li>"
        "</ul>"

        "<h3>参考文档</h3>"
        "<ul>"
        "<li>EStreamEye_UG_Mac.pdf - Elecard StreamEye 用户指南</li>"
        "<li>EStreamAnalyzer_UG_Mac.pdf - Elecard StreamAnalyzer 用户指南</li>"
        "</ul>"
    );
    helpLayout->addWidget(helpBrowser);

    tabWidget->addTab(helpTab, tr("帮助"));

    // ===== README Tab =====
    QWidget* readmeTab = new QWidget();
    QVBoxLayout* readmeLayout = new QVBoxLayout(readmeTab);
    readmeLayout->setContentsMargins(0, 0, 0, 0);

    QTextBrowser* readmeBrowser = new QTextBrowser(readmeTab);
    readmeBrowser->setOpenExternalLinks(true);

    // Load README.md
    QFile readmeFile("../README.md");
    if (readmeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&readmeFile);
        QString readmeContent = in.readAll();
        readmeBrowser->setMarkdown(readmeContent);
        readmeFile.close();
    } else {
        readmeBrowser->setHtml(
            "<h2>README.md</h2>"
            "<p>无法加载 README.md 文件。</p>"
            "<p>请访问 GitHub 查看完整文档:</p>"
            "<p><a href='https://github.com/build-your-own-x-with-ai/VideoStudio'>"
            "https://github.com/build-your-own-x-with-ai/VideoStudio</a></p>"
        );
    }

    readmeLayout->addWidget(readmeBrowser);

    tabWidget->addTab(readmeTab, tr("README"));

    mainLayout->addWidget(tabWidget);

    // OK button
    QPushButton* okButton = new QPushButton(tr("确定"), this);
    okButton->setDefault(true);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Download image
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &AboutDialog::onImageDownloaded);

    QNetworkRequest request(QUrl("https://2019.iosdevlog.com/uploads/AIDevLog.jpg"));
    m_networkManager->get(request);
}

void AboutDialog::onImageDownloaded(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();
        QPixmap pixmap;
        if (pixmap.loadFromData(imageData)) {
            // Scale image to fit
            QPixmap scaledPixmap = pixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_imageLabel->setPixmap(scaledPixmap);
        } else {
            m_imageLabel->setText(tr("图片加载失败"));
        }
    } else {
        m_imageLabel->setText(tr("图片加载失败"));
    }
    reply->deleteLater();
}

void AboutDialog::openGitHub() {
    QDesktopServices::openUrl(QUrl("https://github.com/build-your-own-x-with-ai/VideoStudio"));
}

void AboutDialog::openReadme() {
    // This is handled by the README tab
}

} // namespace VideoStudio
