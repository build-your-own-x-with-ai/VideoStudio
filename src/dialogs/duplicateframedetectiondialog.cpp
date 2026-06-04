#include "dialogs/duplicateframedetectiondialog.h"
#include "core/videodecoder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QtConcurrent>
#include <QFutureWatcher>

namespace VideoStudio {

DuplicateFrameDetectionDialog::DuplicateFrameDetectionDialog(VideoDecoder* decoder, QWidget* parent)
    : QDialog(parent)
    , m_decoder(decoder)
    , m_detector(new DuplicateFrameDetector(this))
    , m_selectedGroupIndex(-1)
{
    setWindowTitle(tr("Duplicate Frame Detection"));
    resize(800, 600);

    setupUI();

    // Connect detector signals
    connect(m_detector, &DuplicateFrameDetector::progressUpdated,
            this, &DuplicateFrameDetectionDialog::onProgressUpdated);
    connect(m_detector, &DuplicateFrameDetector::analysisCompleted,
            this, &DuplicateFrameDetectionDialog::onAnalysisCompleted);
}

DuplicateFrameDetectionDialog::~DuplicateFrameDetectionDialog() {
}

void DuplicateFrameDetectionDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Analysis section
    QGroupBox* analysisGroup = new QGroupBox(tr("Analysis"), this);
    QVBoxLayout* analysisLayout = new QVBoxLayout(analysisGroup);

    m_videoInfoLabel = new QLabel(this);
    if (m_decoder && m_decoder->isOpen()) {
        QString info = QString("Video: %1\nTotal Frames: %2")
            .arg(m_decoder->getFileName())
            .arg(m_decoder->getFrameCount());
        m_videoInfoLabel->setText(info);
    }
    analysisLayout->addWidget(m_videoInfoLabel);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_startButton = new QPushButton(tr("Start Analysis"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_cancelButton->setEnabled(false);
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addStretch();
    analysisLayout->addLayout(buttonLayout);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    analysisLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel(tr("Ready to analyze"), this);
    analysisLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(analysisGroup);

    // Results section
    QGroupBox* resultsGroup = new QGroupBox(tr("Results"), this);
    QVBoxLayout* resultsLayout = new QVBoxLayout(resultsGroup);

    m_totalFramesLabel = new QLabel(tr("Total Frames: -"), this);
    m_uniqueFramesLabel = new QLabel(tr("Unique Frames: -"), this);
    m_duplicateFramesLabel = new QLabel(tr("Duplicate Frames: -"), this);
    m_duplicateGroupsLabel = new QLabel(tr("Duplicate Groups: -"), this);
    m_longestFreezeLabel = new QLabel(tr("Longest Freeze: -"), this);

    resultsLayout->addWidget(m_totalFramesLabel);
    resultsLayout->addWidget(m_uniqueFramesLabel);
    resultsLayout->addWidget(m_duplicateFramesLabel);
    resultsLayout->addWidget(m_duplicateGroupsLabel);
    resultsLayout->addWidget(m_longestFreezeLabel);

    mainLayout->addWidget(resultsGroup);

    // Duplicate groups table
    QGroupBox* groupsGroup = new QGroupBox(tr("Duplicate Groups"), this);
    QVBoxLayout* groupsLayout = new QVBoxLayout(groupsGroup);

    m_groupsTable = new QTableWidget(this);
    m_groupsTable->setColumnCount(4);
    m_groupsTable->setHorizontalHeaderLabels({tr("Group"), tr("Occurrences"), tr("Type"), tr("Frame Numbers")});
    m_groupsTable->horizontalHeader()->setStretchLastSection(true);
    m_groupsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_groupsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_groupsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    groupsLayout->addWidget(m_groupsTable);

    mainLayout->addWidget(groupsGroup);

    // Actions section
    QGroupBox* actionsGroup = new QGroupBox(tr("Actions"), this);
    QHBoxLayout* actionsLayout = new QHBoxLayout(actionsGroup);

    m_goToFrameButton = new QPushButton(tr("Go to Frame"), this);
    m_goToFrameButton->setEnabled(false);
    m_exportButton = new QPushButton(tr("Export Report"), this);
    m_exportButton->setEnabled(false);
    m_closeButton = new QPushButton(tr("Close"), this);

    actionsLayout->addWidget(m_goToFrameButton);
    actionsLayout->addWidget(m_exportButton);
    actionsLayout->addStretch();
    actionsLayout->addWidget(m_closeButton);

    mainLayout->addWidget(actionsGroup);

    // Connect signals
    connect(m_startButton, &QPushButton::clicked, this, &DuplicateFrameDetectionDialog::startAnalysis);
    connect(m_cancelButton, &QPushButton::clicked, this, &DuplicateFrameDetectionDialog::cancelAnalysis);
    connect(m_groupsTable, &QTableWidget::cellClicked, this, &DuplicateFrameDetectionDialog::onGroupSelected);
    connect(m_groupsTable, &QTableWidget::cellDoubleClicked, this, &DuplicateFrameDetectionDialog::goToFrame);
    connect(m_goToFrameButton, &QPushButton::clicked, this, &DuplicateFrameDetectionDialog::goToFrame);
    connect(m_exportButton, &QPushButton::clicked, this, &DuplicateFrameDetectionDialog::exportReport);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void DuplicateFrameDetectionDialog::startAnalysis() {
    if (!m_decoder || !m_decoder->isOpen()) {
        QMessageBox::warning(this, tr("Error"), tr("No video file loaded."));
        return;
    }

    // Disable start button, enable cancel
    m_startButton->setEnabled(false);
    m_cancelButton->setEnabled(true);
    m_goToFrameButton->setEnabled(false);
    m_exportButton->setEnabled(false);

    // Clear previous results
    m_groupsTable->setRowCount(0);
    m_selectedGroupIndex = -1;

    // Reset progress
    m_progressBar->setValue(0);
    m_statusLabel->setText(tr("Starting analysis..."));

    // Run analysis in background thread
    QFuture<bool> future = QtConcurrent::run([this]() {
        return m_detector->analyzeVideo(m_decoder);
    });

    // Watch for completion (analysis completed signal will be emitted)
}

void DuplicateFrameDetectionDialog::cancelAnalysis() {
    m_detector->cancel();
    m_statusLabel->setText(tr("Analysis cancelled"));
    m_startButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
}

void DuplicateFrameDetectionDialog::onProgressUpdated(int current, int total, const QString& status) {
    if (total > 0) {
        int percentage = (current * 100) / total;
        m_progressBar->setValue(percentage);
    }
    m_statusLabel->setText(status);
}

void DuplicateFrameDetectionDialog::onAnalysisCompleted(const DetectionResult& result) {
    m_currentResult = result;
    displayResults(result);

    m_startButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
    m_exportButton->setEnabled(true);
    m_progressBar->setValue(100);
    m_statusLabel->setText(tr("Analysis complete"));
}

void DuplicateFrameDetectionDialog::displayResults(const DetectionResult& result) {
    // Update summary labels
    m_totalFramesLabel->setText(tr("Total Frames: %1").arg(result.totalFrames));
    m_uniqueFramesLabel->setText(tr("Unique Frames: %1").arg(result.uniqueFrames));
    m_duplicateFramesLabel->setText(tr("Consecutive Duplicate Frames: %1 (%2%)")
        .arg(result.duplicateFrames)
        .arg(result.duplicatePercentage, 0, 'f', 1));
    m_duplicateGroupsLabel->setText(tr("Freeze Frame Groups: %1").arg(result.duplicateGroupCount));

    if (result.maxConsecutiveDuplicates > 0) {
        // Find the group with max consecutive duplicates
        QString freezeInfo;
        for (const auto& group : result.duplicateGroups) {
            if (group.occurrences == result.maxConsecutiveDuplicates) {
                freezeInfo = QString("%1 frames (frames %2-%3)")
                    .arg(result.maxConsecutiveDuplicates)
                    .arg(group.frameNumbers.first())
                    .arg(group.frameNumbers.last());
                break;
            }
        }
        m_longestFreezeLabel->setText(tr("Longest Freeze: %1").arg(freezeInfo));
    } else {
        m_longestFreezeLabel->setText(tr("Longest Freeze: -"));
    }

    // Populate table
    m_groupsTable->setRowCount(result.duplicateGroups.size());
    for (int i = 0; i < result.duplicateGroups.size(); ++i) {
        const DuplicateGroup& group = result.duplicateGroups[i];

        // Group number
        QTableWidgetItem* groupItem = new QTableWidgetItem(QString::number(i + 1));
        m_groupsTable->setItem(i, 0, groupItem);

        // Occurrences
        QTableWidgetItem* occItem = new QTableWidgetItem(QString::number(group.occurrences));
        m_groupsTable->setItem(i, 1, occItem);

        // Type (always consecutive for adjacent frame comparison)
        QString type = tr("Consecutive");
        QTableWidgetItem* typeItem = new QTableWidgetItem(type);
        m_groupsTable->setItem(i, 2, typeItem);

        // Frame numbers
        QString frameList = formatFrameList(group.frameNumbers);
        QTableWidgetItem* framesItem = new QTableWidgetItem(frameList);
        m_groupsTable->setItem(i, 3, framesItem);

        // Color coding for long consecutive sequences (>= 10 frames)
        if (group.occurrences >= 10) {
            for (int col = 0; col < 4; ++col) {
                m_groupsTable->item(i, col)->setBackground(QColor(255, 200, 200));
            }
        }
    }

    m_groupsTable->resizeColumnsToContents();
}

QString DuplicateFrameDetectionDialog::formatFrameList(const QVector<int>& frames) const {
    if (frames.isEmpty()) {
        return QString();
    }

    QString result;
    if (frames.size() <= 5) {
        // Show all frames
        for (int i = 0; i < frames.size(); ++i) {
            if (i > 0) result += ", ";
            result += QString::number(frames[i]);
        }
    } else {
        // Check if consecutive
        bool consecutive = true;
        for (int i = 1; i < frames.size(); ++i) {
            if (frames[i] != frames[i - 1] + 1) {
                consecutive = false;
                break;
            }
        }

        if (consecutive) {
            // Show range
            result = QString("%1-%2").arg(frames.first()).arg(frames.last());
        } else {
            // Show first 3 and last 2
            result = QString("%1, %2, %3, ..., %4, %5")
                .arg(frames[0])
                .arg(frames[1])
                .arg(frames[2])
                .arg(frames[frames.size() - 2])
                .arg(frames[frames.size() - 1]);
        }
    }

    return result;
}

void DuplicateFrameDetectionDialog::onGroupSelected(int row, int column) {
    Q_UNUSED(column);
    m_selectedGroupIndex = row;
    m_goToFrameButton->setEnabled(row >= 0);
}

void DuplicateFrameDetectionDialog::goToFrame() {
    if (m_selectedGroupIndex < 0 || m_selectedGroupIndex >= m_currentResult.duplicateGroups.size()) {
        return;
    }

    const DuplicateGroup& group = m_currentResult.duplicateGroups[m_selectedGroupIndex];
    if (!group.frameNumbers.isEmpty()) {
        // Navigate to first frame in group
        emit seekToFrame(group.frameNumbers.first());
    }
}

void DuplicateFrameDetectionDialog::exportReport() {
    if (m_currentResult.duplicateGroups.isEmpty()) {
        QMessageBox::information(this, tr("No Data"), tr("No duplicate frames to export."));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Duplicate Frame Report"),
        "duplicate_frames_report.csv",
        tr("CSV Files (*.csv);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export Failed"),
            tr("Failed to open file for writing:\n%1").arg(fileName));
        return;
    }

    QTextStream out(&file);

    // Write summary
    out << "# Consecutive Duplicate Frame Detection Report\n";
    out << "# Video: " << (m_decoder ? m_decoder->getFileName() : "Unknown") << "\n";
    out << "\n";
    out << "Total Frames," << m_currentResult.totalFrames << "\n";
    out << "Unique Frames," << m_currentResult.uniqueFrames << "\n";
    out << "Consecutive Duplicate Frames," << m_currentResult.duplicateFrames << "\n";
    out << "Duplicate Percentage," << QString::number(m_currentResult.duplicatePercentage, 'f', 2) << "%\n";
    out << "Freeze Frame Groups," << m_currentResult.duplicateGroupCount << "\n";
    out << "Max Consecutive Duplicates," << m_currentResult.maxConsecutiveDuplicates << "\n";
    out << "\n";

    // Write table header
    out << "Group,Occurrences,Type,Frame Numbers\n";

    // Write duplicate groups
    for (int i = 0; i < m_currentResult.duplicateGroups.size(); ++i) {
        const DuplicateGroup& group = m_currentResult.duplicateGroups[i];
        QString type = "Consecutive";

        // Build frame list
        QString frameList;
        for (int j = 0; j < group.frameNumbers.size(); ++j) {
            if (j > 0) frameList += " ";
            frameList += QString::number(group.frameNumbers[j]);
        }

        out << (i + 1) << "," << group.occurrences << "," << type << "," << frameList << "\n";
    }

    file.close();

    QMessageBox::information(this, tr("Export Complete"),
        tr("Report exported successfully to:\n%1").arg(fileName));
}

} // namespace VideoStudio
