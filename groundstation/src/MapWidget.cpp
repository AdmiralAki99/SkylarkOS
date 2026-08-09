#include "MapWidget.hpp"
#include "WaypointModel.hpp"

#include <QQmlContext>

MapWidget::MapWidget(WaypointModel *waypointModel, QWidget *parent) : QQuickWidget(parent) {
    setResizeMode(QQuickWidget::SizeRootObjectToView);

    // Context properties must be set before setSource() so the QML sees
    // waypointModel from its first load.
    rootContext()->setContextProperty("waypointModel", waypointModel);

    // Relative to the executable's working directory (run from build/);
    // switch to an embedded qrc resource once packaged for deployment.
    setSource(QUrl::fromLocalFile(QStringLiteral("../qml/MapView.qml")));
}
