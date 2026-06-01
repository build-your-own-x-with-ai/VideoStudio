#include "panels/overlaypanel.h"
#include "widgets/videooutput.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QCheckBox>

namespace VideoStudio {

OverlayPanel::OverlayPanel(QWidget* parent)
    : QWidget(parent)
    , m_videoOutput(nullptr)
    , m_motionVectorsCheckBox(nullptr)
    , m_partitionsCheckBox(nullptr)
    , m_frameTypesCheckBox(nullptr)
{
    createUI();
}

OverlayPanel::~OverlayPanel() {
}

void OverlayPanel::setVideoOutput(VideoOutput* videoOutput) {
    m_videoOutput = videoOutput;
}

void OverlayPanel::setMotionVectorsChecked(bool checked) {
    m_motionVectorsCheckBox->setChecked(checked);
}

void OverlayPanel::setPartitionsChecked(bool checked) {
    m_partitionsCheckBox->setChecked(checked);
}

void OverlayPanel::setFrameTypesChecked(bool checked) {
    m_frameTypesCheckBox->setChecked(checked);
}

void OverlayPanel::createUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Overlay options group
    QGroupBox* overlayGroup = new QGroupBox(tr("Video Overlays"), this);
    QVBoxLayout* overlayLayout = new QVBoxLayout(overlayGroup);

    m_motionVectorsCheckBox = new QCheckBox(tr("Motion Vectors (ALT+3)"), overlayGroup);
    connect(m_motionVectorsCheckBox, &QCheckBox::toggled, this, &OverlayPanel::onMotionVectorsToggled);
    overlayLayout->addWidget(m_motionVectorsCheckBox);

    m_partitionsCheckBox = new QCheckBox(tr("Partitions/Blocks (ALT+2)"), overlayGroup);
    connect(m_partitionsCheckBox, &QCheckBox::toggled, this, &OverlayPanel::onPartitionsToggled);
    overlayLayout->addWidget(m_partitionsCheckBox);

    m_frameTypesCheckBox = new QCheckBox(tr("Frame Type Info (ALT+4)"), overlayGroup);
    connect(m_frameTypesCheckBox, &QCheckBox::toggled, this, &OverlayPanel::onFrameTypesToggled);
    overlayLayout->addWidget(m_frameTypesCheckBox);

    mainLayout->addWidget(overlayGroup);
    mainLayout->addStretch();
}

void OverlayPanel::onMotionVectorsToggled(bool checked) {
    if (m_videoOutput) {
        m_videoOutput->setOverlay(OverlayType::MotionVectors, checked);
    }
    emit motionVectorsToggled(checked);
}

void OverlayPanel::onPartitionsToggled(bool checked) {
    if (m_videoOutput) {
        m_videoOutput->setOverlay(OverlayType::Partitions, checked);
    }
    emit partitionsToggled(checked);
}

void OverlayPanel::onFrameTypesToggled(bool checked) {
    if (m_videoOutput) {
        m_videoOutput->setOverlay(OverlayType::FrameTypes, checked);
    }
    emit frameTypesToggled(checked);
}

} // namespace VideoStudio
