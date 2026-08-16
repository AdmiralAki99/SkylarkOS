#include "MapWidget.hpp"
#include "WaypointModel.hpp"

#include <QQmlContext>
#include <QQuickItem>
#include <QDebug>

MapWidget::MapWidget(WaypointModel *waypointModel, QWidget *parent) : QQuickWidget(parent) {
    setResizeMode(QQuickWidget::SizeRootObjectToView);

    rootContext()->setContextProperty("waypointModel", waypointModel);

    setSource(QUrl::fromLocalFile(QStringLiteral("../qml/MapView.qml")));

    qDebug() << "[MapWidget] status after setSource:" << status() << "rootObject:" << rootObject();
}

void MapWidget::setVehiclePosition(double lat, double lon){
    bool ok = QMetaObject::invokeMethod(rootObject(), "setVehicleCoordinate",
        Q_ARG(QVariant, lat), Q_ARG(QVariant, lon));
    qDebug() << "[MapWidget] setVehiclePosition(" << lat << "," << lon << ") invokeMethod ok:" << ok;
}
