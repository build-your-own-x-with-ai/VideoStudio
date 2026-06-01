#include "src/core/tsparser.h"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    VideoStudio::TSParser parser;
    if (!parser.parseFile("build/test_with_eit.ts")) {
        qDebug() << "Failed to parse TS file";
        return 1;
    }
    
    qDebug() << "Parsed successfully";
    
    const auto& programs = parser.getPrograms();
    qDebug() << "Programs:" << programs.size();
    
    for (const auto& program : programs) {
        qDebug() << "Program" << program.programNumber;
        qDebug() << "  PMT PID:" << QString::number(program.pmtPid, 16);
        qDebug() << "  Elementary PIDs count:" << program.elementaryPIDs.size();
        
        for (uint16_t pid : program.elementaryPIDs) {
            qDebug() << "    PID:" << QString::number(pid, 16);
        }
    }
    
    return 0;
}
