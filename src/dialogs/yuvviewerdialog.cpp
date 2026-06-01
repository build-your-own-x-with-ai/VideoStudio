#include "yuvviewerdialog.h"
#include "core/yuvreader.h"
#include "widgets/videooutput.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

namespace VideoStudio {

YUVViewerDialog::YUVViewerDialog(QWidget* parent)
    : QDialog(parent)
    , m_reader(std::make_unique<YUVReader>())
    , m_playbackTimer(new QTimer(this))
    , m_isPlaying(false)
    , m_currentFrame(0)
{
    setWindowTitle(tr("YUV Viewer"));
    resize(900, 700);
    setupUI();

    connect(m_playbackTimer, &QTimer::timeout, this, &YUVViewerDialog::onPlaybackTimer);
}

YUVViewerDialog::~YUVViewerDialog() = default;

void YUVViewerDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // File selection
    QHBoxLayout* fileLayout = new QHBoxLayout();
    QLabel* fileLabel = new QLabel(tr("File:"));
    m_filePathEdit = new QLineEdit();
    m_filePathEdit->setPlaceholderText(tr("Select a raw YUV file..."));
    m_filePathEdit->setReadOnly(true);
    m_browseButton = new QPushButton(tr("Browse..."));
    connect(m_browseButton, &QPushButton::clicked, this, &YUVViewerDialog::browseFile);
    fileLayout->addWidget(fileLabel);
    fileLayout->addWidget(m_filePathEdit, 1);
    fileLayout->addWidget(m_browseButton);
    mainLayout->addLayout(fileLayout);

    // Format parameters group
    QGroupBox* formatGroup = new QGroupBox(tr("Format Parameters"));
    QFormLayout* formatLayout = new QFormLayout(formatGroup);

    m_widthSpin = new QSpinBox();
    m_widthSpin->setRange(1, 7680);
    m_widthSpin->setValue(1920);
    m_widthSpin->setSingleStep(2);
    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &YUVViewerDialog::onWidthChanged);
    formatLayout->addRow(tr("Width:"), m_widthSpin);

    m_heightSpin = new QSpinBox();
    m_heightSpin->setRange(1, 4320);
    m_heightSpin->setValue(1080);
    m_heightSpin->setSingleStep(2);
    connect(m_heightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &YUVViewerDialog::onHeightChanged);
    formatLayout->addRow(tr("Height:"), m_heightSpin);

    m_formatCombo = new QComboBox();
    m_formatCombo->addItem(YUVReader::formatToString(YUVPixelFormat::I420), static_cast<int>(YUVPixelFormat::I420));
    m_formatCombo->addItem(YUVReader::formatToString(YUVPixelFormat::NV12), static_cast<int>(YUVPixelFormat::NV12));
    m_formatCombo->addItem(YUVReader::formatToString(YUVPixelFormat::I422), static_cast<int>(YUVPixelFormat::I422));
    m_formatCombo->addItem(YUVReader::formatToString(YUVPixelFormat::YUY2), static_cast<int>(YUVPixelFormat::YUY2));
    m_formatCombo->addItem(YUVReader::formatToString(YUVPixelFormat::I444), static_cast<int>(YUVPixelFormat::I444));
    m_formatCombo->addItem(YUVReader::formatToString(YUVPixelFormat::NV21), static_cast<int>(YUVPixelFormat::NV21));
    m_formatCombo->addItem(YUVReader::formatToString(YUVPixelFormat::UYVY), static_cast<int>(YUVPixelFormat::UYVY));
    m_formatCombo->addItem(YUVReader::formatToString(YUVPixelFormat::YV12), static_cast<int>(YUVPixelFormat::YV12));
    m_formatCombo->addItem(YUVReader::formatToString(YUVPixelFormat::RGB24), static_cast<int>(YUVPixelFormat::RGB24));
    m_formatCombo->addItem(YUVReader::formatToString(YUVPixelFormat::RGB32), static_cast<int>(YUVPixelFormat::RGB32));
    m_formatCombo->addItem(YUVReader::formatToString(YUVPixelFormat::GRAY), static_cast<int>(YUVPixelFormat::GRAY));
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &YUVViewerDialog::onFormatChanged);
    formatLayout->addRow(tr("Format:"), m_formatCombo);

    m_frameCountLabel = new QLabel(tr("Frames: 0"));
    formatLayout->addRow(m_frameCountLabel);

    QPushButton* openButton = new QPushButton(tr("Open"));
    connect(openButton, &QPushButton::clicked, this, &YUVViewerDialog::openYUVFile);
    formatLayout->addRow(openButton);

    mainLayout->addWidget(formatGroup);

    // Video display
    QGroupBox* displayGroup = new QGroupBox(tr("Video Display"));
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);
    m_videoOutput = new VideoOutput(this);
    m_videoOutput->setMinimumSize(640, 360);
    displayLayout->addWidget(m_videoOutput);
    mainLayout->addWidget(displayGroup);

    // Playback controls
    QGroupBox* controlsGroup = new QGroupBox(tr("Playback Controls"));
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsGroup);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    m_stepBackButton = new QPushButton(tr("◄"));
    m_stepBackButton->setEnabled(false);
    connect(m_stepBackButton, &QPushButton::clicked, this, &YUVViewerDialog::stepBackward);
    buttonsLayout->addWidget(m_stepBackButton);

    m_playButton = new QPushButton(tr("▶"));
    m_playButton->setEnabled(false);
    connect(m_playButton, &QPushButton::clicked, this, &YUVViewerDialog::playPause);
    buttonsLayout->addWidget(m_playButton);

    m_stepForwardButton = new QPushButton(tr("►"));
    m_stepForwardButton->setEnabled(false);
    connect(m_stepForwardButton, &QPushButton::clicked, this, &YUVViewerDialog::stepForward);
    buttonsLayout->addWidget(m_stepForwardButton);

    QLabel* frameLabel = new QLabel(tr("Frame:"));
    buttonsLayout->addWidget(frameLabel);

    m_frameNumberSpin = new QSpinBox();
    m_frameNumberSpin->setRange(0, 0);
    m_frameNumberSpin->setEnabled(false);
    connect(m_frameNumberSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &YUVViewerDialog::seekToFrame);
    buttonsLayout->addWidget(m_frameNumberSpin);

    m_frameInfoLabel = new QLabel(tr("/ 0"));
    buttonsLayout->addWidget(m_frameInfoLabel);

    buttonsLayout->addStretch();
    controlsLayout->addLayout(buttonsLayout);

    m_frameSlider = new QSlider(Qt::Horizontal);
    m_frameSlider->setRange(0, 0);
    m_frameSlider->setEnabled(false);
    connect(m_frameSlider, &QSlider::valueChanged, this, &YUVViewerDialog::seekToFrame);
    controlsLayout->addWidget(m_frameSlider);

    mainLayout->addWidget(controlsGroup);

    // Bottom buttons
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();

    m_exportButton = new QPushButton(tr("Export Frame"));
    m_exportButton->setEnabled(false);
    connect(m_exportButton, &QPushButton::clicked, this, &YUVViewerDialog::exportCurrentFrame);
    bottomLayout->addWidget(m_exportButton);

    m_closeButton = new QPushButton(tr("Close"));
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    bottomLayout->addWidget(m_closeButton);

    mainLayout->addLayout(bottomLayout);
}

void YUVViewerDialog::browseFile() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Select Raw YUV File"),
        QString(),
        tr("YUV Files (*.yuv *.raw);;All Files (*)")
    );

    if (!fileName.isEmpty()) {
        m_filePathEdit->setText(fileName);
    }
}

void YUVViewerDialog::openYUVFile() {
    QString filePath = m_filePathEdit->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, tr("No File Selected"), tr("Please select a YUV file first."));
        return;
    }

    int width = m_widthSpin->value();
    int height = m_heightSpin->value();
    YUVPixelFormat format = static_cast<YUVPixelFormat>(m_formatCombo->currentData().toInt());

    if (m_reader->openFile(filePath, width, height, format)) {
        // Update UI
        int frameCount = m_reader->getFrameCount();
        m_frameCountLabel->setText(tr("Frames: %1").arg(frameCount));
        m_frameSlider->setRange(0, frameCount - 1);
        m_frameSlider->setEnabled(true);
        m_frameNumberSpin->setRange(0, frameCount - 1);
        m_frameNumberSpin->setEnabled(true);
        m_frameInfoLabel->setText(tr("/ %1").arg(frameCount));
        m_playButton->setEnabled(true);
        m_stepBackButton->setEnabled(true);
        m_stepForwardButton->setEnabled(true);
        m_exportButton->setEnabled(true);

        // Display first frame
        m_currentFrame = 0;
        updateFrameDisplay();

        QMessageBox::information(this, tr("File Opened"),
            tr("Successfully opened YUV file:\n%1\n\nResolution: %2x%3\nFormat: %4\nFrames: %5")
            .arg(filePath)
            .arg(width)
            .arg(height)
            .arg(YUVReader::formatToString(format))
            .arg(frameCount));
    } else {
        // Error message already emitted by YUVReader
        QMessageBox::critical(this, tr("Error"), tr("Failed to open YUV file. Check console for details."));
    }
}

void YUVViewerDialog::onFormatChanged(int index) {
    Q_UNUSED(index);
    validateAndUpdateFrameCount();
}

void YUVViewerDialog::onWidthChanged(int width) {
    Q_UNUSED(width);
    validateAndUpdateFrameCount();
}

void YUVViewerDialog::onHeightChanged(int height) {
    Q_UNUSED(height);
    validateAndUpdateFrameCount();
}

void YUVViewerDialog::validateAndUpdateFrameCount() {
    QString filePath = m_filePathEdit->text();
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.exists()) {
        return;
    }

    int width = m_widthSpin->value();
    int height = m_heightSpin->value();
    YUVPixelFormat format = static_cast<YUVPixelFormat>(m_formatCombo->currentData().toInt());

    int64_t fileSize = file.size();
    int bytesPerFrame = YUVReader::calculateBytesPerFrame(width, height, format);

    if (bytesPerFrame > 0 && fileSize % bytesPerFrame == 0) {
        int frameCount = fileSize / bytesPerFrame;
        m_frameCountLabel->setText(tr("Frames: %1 ✓").arg(frameCount));
    } else if (bytesPerFrame > 0) {
        double expectedFrames = static_cast<double>(fileSize) / bytesPerFrame;
        m_frameCountLabel->setText(tr("Frames: %1 ✗ (mismatch)").arg(expectedFrames, 0, 'f', 2));
    } else {
        m_frameCountLabel->setText(tr("Frames: 0"));
    }
}

void YUVViewerDialog::playPause() {
    if (m_isPlaying) {
        m_isPlaying = false;
        m_playbackTimer->stop();
        m_playButton->setText(tr("▶"));
    } else {
        m_isPlaying = true;
        m_playbackTimer->start(33);  // ~30 fps
        m_playButton->setText(tr("⏸"));
    }
}

void YUVViewerDialog::stepForward() {
    if (!m_reader->isOpen()) {
        return;
    }

    if (m_currentFrame < m_reader->getFrameCount() - 1) {
        m_currentFrame++;
        updateFrameDisplay();
    }
}

void YUVViewerDialog::stepBackward() {
    if (!m_reader->isOpen()) {
        return;
    }

    if (m_currentFrame > 0) {
        m_currentFrame--;
        updateFrameDisplay();
    }
}

void YUVViewerDialog::seekToFrame(int frameNumber) {
    if (!m_reader->isOpen()) {
        return;
    }

    if (frameNumber >= 0 && frameNumber < m_reader->getFrameCount()) {
        m_currentFrame = frameNumber;
        updateFrameDisplay();
    }
}

void YUVViewerDialog::onPlaybackTimer() {
    if (!m_reader->isOpen()) {
        playPause();  // Stop playback
        return;
    }

    if (m_currentFrame < m_reader->getFrameCount() - 1) {
        m_currentFrame++;
        updateFrameDisplay();
    } else {
        playPause();  // Stop at end
    }
}

void YUVViewerDialog::updateFrameDisplay() {
    if (!m_reader->isOpen()) {
        return;
    }

    AVFrame* frame = m_reader->readFrame(m_currentFrame);
    if (frame) {
        m_videoOutput->displayFrame(frame);

        // Update UI
        m_frameSlider->blockSignals(true);
        m_frameSlider->setValue(m_currentFrame);
        m_frameSlider->blockSignals(false);

        m_frameNumberSpin->blockSignals(true);
        m_frameNumberSpin->setValue(m_currentFrame);
        m_frameNumberSpin->blockSignals(false);
    }
}

void YUVViewerDialog::exportCurrentFrame() {
    if (!m_reader->isOpen()) {
        return;
    }

    QString defaultFileName = QString("frame_%1.png").arg(m_currentFrame);
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export Frame"),
        defaultFileName,
        tr("PNG Image (*.png);;JPEG Image (*.jpg *.jpeg);;All Files (*)")
    );

    if (fileName.isEmpty()) {
        return;
    }

    QImage image = m_videoOutput->getCurrentImage();
    if (image.isNull()) {
        QMessageBox::warning(this, tr("Export Failed"), tr("No frame available to export."));
        return;
    }

    QString format = "PNG";
    if (fileName.endsWith(".jpg", Qt::CaseInsensitive) || fileName.endsWith(".jpeg", Qt::CaseInsensitive)) {
        format = "JPEG";
    }

    if (image.save(fileName, format.toUtf8().constData())) {
        QMessageBox::information(this, tr("Export Successful"), tr("Frame exported to:\n%1").arg(fileName));
    } else {
        QMessageBox::critical(this, tr("Export Failed"), tr("Failed to save frame to:\n%1").arg(fileName));
    }
}

} // namespace VideoStudio
