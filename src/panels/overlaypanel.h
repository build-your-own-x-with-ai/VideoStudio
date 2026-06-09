#ifndef OVERLAYPANEL_H
#define OVERLAYPANEL_H

#include <QWidget>
#include <QCheckBox>
#include <QGroupBox>

namespace VideoStudio {

class VideoOutput;

class OverlayPanel : public QWidget {
    Q_OBJECT

public:
    explicit OverlayPanel(QWidget* parent = nullptr);
    ~OverlayPanel();

    void setVideoOutput(VideoOutput* videoOutput);

    void setMotionVectorsChecked(bool checked);
    void setPartitionsChecked(bool checked);
    void setFrameTypesChecked(bool checked);
    void setQPHeatmapChecked(bool checked);

signals:
    void motionVectorsToggled(bool checked);
    void partitionsToggled(bool checked);
    void frameTypesToggled(bool checked);
    void qpHeatmapToggled(bool checked);

private slots:
    void onMotionVectorsToggled(bool checked);
    void onPartitionsToggled(bool checked);
    void onFrameTypesToggled(bool checked);
    void onQPHeatmapToggled(bool checked);

private:
    void createUI();

    VideoOutput* m_videoOutput;
    QCheckBox* m_motionVectorsCheckBox;
    QCheckBox* m_partitionsCheckBox;
    QCheckBox* m_frameTypesCheckBox;
    QCheckBox* m_qpHeatmapCheckBox;
};

} // namespace VideoStudio

#endif // OVERLAYPANEL_H
