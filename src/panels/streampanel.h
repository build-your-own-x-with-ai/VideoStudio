#ifndef STREAMPANEL_H
#define STREAMPANEL_H

#include <QWidget>
#include <QTreeWidget>

namespace VideoStudio {

class VideoDecoder;
class TSParser;

class StreamPanel : public QWidget {
    Q_OBJECT

public:
    explicit StreamPanel(QWidget* parent = nullptr);
    ~StreamPanel();

    void setDecoder(VideoDecoder* decoder);
    void setTSParser(TSParser* parser);
    void updateInfo();
    void updateTSInfo();
    void clear();

private:
    void setupUI();
    void addInfoItem(const QString& name, const QString& value, QTreeWidgetItem* parent = nullptr);

    QTreeWidget* m_treeWidget;
    VideoDecoder* m_decoder;
    TSParser* m_tsParser;
};

} // namespace VideoStudio

#endif // STREAMPANEL_H
