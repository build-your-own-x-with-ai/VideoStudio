#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("VideoStudio");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("VideoStudio");

    VideoStudio::MainWindow window;
    window.show();

    return app.exec();
}
