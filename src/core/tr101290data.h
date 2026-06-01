#ifndef TR101290DATA_H
#define TR101290DATA_H

#include <QString>
#include <QVector>
#include <cstdint>

namespace VideoStudio {

enum class TR101290Priority {
    First,
    Second,
    Third
};

enum class TR101290ErrorType {
    // First priority errors
    TSSyncLoss,
    SyncByteError,
    PATError,
    ContinuityCountError,
    PMTError,
    PIDError,

    // Second priority errors
    TransportError,
    CRCError,
    PCRRepetitionError,
    PCRDiscontinuityError,
    PCRAccuracyError,
    PTSError,
    CATError,

    // Third priority errors
    NITActualError,
    NITOtherError,
    SIRepetitionError,
    UnreferencedPID,
    SDTActualError,
    SDTOtherError,
    EITActualError,
    EITOtherError,
    RSTError,
    EITPFError
};

struct TR101290Error {
    TR101290ErrorType type;
    TR101290Priority priority;
    int64_t offset;
    uint16_t pid;
    int packetIndex;
    QString description;
    int64_t timestamp; // For time-based errors
};

} // namespace VideoStudio

#endif // TR101290DATA_H
