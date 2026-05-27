#include <QApplication>
#include <QDebug>
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    qDebug() << "VideoStudio v1.0.0 - Build" << __DATE__ << __TIME__;

    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
