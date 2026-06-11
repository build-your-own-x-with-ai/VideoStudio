#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QSettings>
#include <QLocale>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("VideoStudio");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("VideoStudio");

    // Load translation
    QTranslator translator;
    QSettings settings("VideoStudio", "VideoStudio");
    QString language = settings.value("language", "en").toString();

    if (language == "zh_CN") {
        if (translator.load("videostudio_zh_CN", ":/i18n")) {
            app.installTranslator(&translator);
        }
    }

    VideoStudio::MainWindow window;
    window.show();

    return app.exec();
}
