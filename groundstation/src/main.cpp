#include <QApplication>
#include <QMainWindow>
#include <QFontDatabase>
#include <QFont>
#include <QFile>

#include "GroundStationView.hpp"

int main(int argc, char *argv[]){
    QApplication app(argc, argv);

    QFontDatabase::addApplicationFont("../assets/fonts/SpaceGrotesk-Medium.ttf");
    QFontDatabase::addApplicationFont("../assets/fonts/IBMPlexMono-Regular.ttf");
    QFontDatabase::addApplicationFont("../assets/fonts/IBMPlexMono-Medium.ttf");
    QFontDatabase::addApplicationFont("../assets/fonts/IBMPlexMono-SemiBold.ttf");

    app.setFont(QFont("Space Grotesk", 10));

    QFile styleFile("../style.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    QMainWindow mainWindow;
    GroundStationView *view = new GroundStationView();
    mainWindow.setCentralWidget(view);
    view->start("127.0.0.1", 5600);
    mainWindow.show();
    return app.exec();
}