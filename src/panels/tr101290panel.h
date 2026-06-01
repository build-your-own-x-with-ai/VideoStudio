#ifndef TR101290PANEL_H
#define TR101290PANEL_H

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QMap>
#include "core/tr101290data.h"

namespace VideoStudio {

class TSParser;

class TR101290Panel : public QWidget {
    Q_OBJECT

public:
    explicit TR101290Panel(QWidget* parent = nullptr);
    ~TR101290Panel();

    void setTSParser(TSParser* parser);
    void clear();
    void analyzeStream();

    // Get errors for Messages Panel
    const QVector<TR101290Error>& getErrors() const { return m_errors; }

signals:
    void errorDoubleClicked(int packetIndex);

private slots:
    void onErrorDoubleClicked(QTreeWidgetItem* item, int column);

private:
    void createUI();
    void buildErrorTree();
    QString getErrorTypeName(TR101290ErrorType type) const;
    QString getPriorityName(TR101290Priority priority) const;
    QColor getPriorityColor(TR101290Priority priority) const;

    // First priority checks
    void checkTSSyncLoss();
    void checkSyncByteError();
    void checkPATError();
    void checkContinuityCountError();
    void checkPMTError();
    void checkPIDError();

    // Second priority checks
    void checkTransportError();
    void checkCRCError();
    void checkPCRRepetitionError();
    void checkPCRDiscontinuityError();
    void checkPCRAccuracyError();
    void checkPTSError();
    void checkCATError();

    // Third priority checks
    void checkNITActualError();
    void checkSIRepetitionError();
    void checkUnreferencedPID();
    void checkSDTActualError();
    void checkEITActualError();

    TSParser* m_parser;
    QVector<TR101290Error> m_errors;

    // UI components
    QTreeWidget* m_errorTree;
    QLabel* m_firstPriorityLabel;
    QLabel* m_secondPriorityLabel;
    QLabel* m_thirdPriorityLabel;
    QLabel* m_statusLabel;

    // Error counts
    int m_firstPriorityCount;
    int m_secondPriorityCount;
    int m_thirdPriorityCount;
};

} // namespace VideoStudio

#endif // TR101290PANEL_H
