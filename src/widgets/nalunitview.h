#ifndef NALUNITVIEW_H
#define NALUNITVIEW_H

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include "core/nalunitparser.h"

namespace VideoStudio {

class NALUnitView : public QWidget {
    Q_OBJECT

public:
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

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);

private:
    QString formatProperties(const NALUnitInfo& info);
    QColor getNALUnitColor(const NALUnitInfo& info);

    QTreeWidget* m_treeWidget;
    NALUnitParser* m_parser;
    int m_currentNALIndex;
};

} // namespace VideoStudio

#endif // NALUNITVIEW_H
