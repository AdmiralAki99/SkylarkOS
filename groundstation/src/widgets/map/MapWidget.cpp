#include "MapWidget.hpp"
#include "WaypointModel.hpp"

#include <QQmlContext>
#include <QQuickItem>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>

MapWidget::MapWidget(WaypointModel *waypointModel, QWidget *parent) : QQuickWidget(parent) {
    setResizeMode(QQuickWidget::SizeRootObjectToView);

    rootContext()->setContextProperty("waypointModel", waypointModel);

    const QString resourceRoot = QDir(QCoreApplication::applicationDirPath()).filePath("..");
    setSource(QUrl::fromLocalFile(resourceRoot + "/qml/MapView.qml"));

    qDebug() << "[MapWidget] status after setSource:" << status() << "rootObject:" << rootObject();
}

void MapWidget::setVehiclePosition(double lat, double lon){
    bool ok = QMetaObject::invokeMethod(rootObject(), "setVehicleCoordinate",
        Q_ARG(QVariant, lat), Q_ARG(QVariant, lon));
    qDebug() << "[MapWidget] setVehiclePosition(" << lat << "," << lon << ") invokeMethod ok:" << ok;
}
