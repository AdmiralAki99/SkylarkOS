#include <QApplication>
#include <QMainWindow>

#include "GroundStationView.hpp"

int main(int argc, char *argv[]){
    QApplication app(argc, argv);
    QMainWindow mainWindow;
    GroundStationView *view = new GroundStationView();
    mainWindow.setCentralWidget(view);
    view->start("127.0.0.1", 5600);
    mainWindow.show();
    return app.exec();
}