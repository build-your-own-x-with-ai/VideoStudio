#ifndef NALUNITVIEW_H
#define NALUNITVIEW_H

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include "core/nalunitparser.h"

namespace VideoStudio {

class NALUnitView : public QWidget {
    Q_OBJECT

public:
    // Arrow drawing info (public so ArrowOverlayWidget can use it)
    struct ArrowInfo {
        QTreeWidgetItem* sourceItem;
        QTreeWidgetItem* targetItem;
        QColor color;
    };

    explicit NALUnitView(QWidget* parent = nullptr);
    ~NALUnitView();

    // Set NAL unit parser data
    void setNALUnitParser(NALUnitParser* parser);

    // Build/rebuild the NAL unit list
    void buildNALUnitList();

    // Clear view
    void clear();

    // Set current NAL unit
    void setCurrentNALUnit(int nalIndex);

signals:
    void nalUnitSelected(int nalIndex);
    void nalUnitDoubleClicked(int nalIndex);
    void frameSelected(int frameNumber);
    void audioFrameSelected(int audioIndex);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onExportToCSV();

private:
    QString formatProperties(const NALUnitInfo& info);
    QColor getNALUnitColor(const NALUnitInfo& info);
    void exportNALUnitsToCSV(const QString& filePath);
    void highlightReferencedFrames(int nalIndex);
    void clearReferenceHighlights();
    int findNALUnitByFrameNumber(int frameNumber);

    QTreeWidget* m_treeWidget;
    QPushButton* m_exportButton;
    NALUnitParser* m_parser;
    int m_currentNALIndex;
    QVector<QTreeWidgetItem*> m_highlightedItems;
    QWidget* m_arrowOverlay;  // Pointer to arrow overlay widget
    QVector<ArrowInfo> m_arrows;
};

} // namespace VideoStudio

#endif // NALUNITVIEW_H
