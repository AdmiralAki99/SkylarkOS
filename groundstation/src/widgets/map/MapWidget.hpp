#ifndef MAP_WIDGET_HPP
#define MAP_WIDGET_HPP

#include <QQuickWidget>

class WaypointModel;

class MapWidget : public QQuickWidget {
    Q_OBJECT
public:
    explicit MapWidget(WaypointModel *waypointModel, QWidget *parent = nullptr);
    void setVehiclePosition(double lat, double lon);
};

#endif // MAP_WIDGET_HPP
