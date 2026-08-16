#include "WaypointModel.hpp"

WaypointModel::WaypointModel(QObject *parent) : QAbstractListModel(parent) {
}

int WaypointModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return waypoints_.size();
}

QVariant WaypointModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= waypoints_.size()) {
        return QVariant();
    }
    const Waypoint &wp = waypoints_.at(index.row());
    switch (role) {
        case LabelRole: return wp.label;
        case LatitudeRole: return wp.latitude;
        case LongitudeRole: return wp.longitude;
        case AltitudeRole: return wp.altitude;
        default: return QVariant();
    }
}

QHash<int, QByteArray> WaypointModel::roleNames() const {
    return {
        {LabelRole, "label"},
        {LatitudeRole, "latitude"},
        {LongitudeRole, "longitude"},
        {AltitudeRole, "altitude"}
    };
}

QString WaypointModel::nextLabel() const {
    int n = waypoints_.size();
    QString label;
    do {
        label.prepend(QChar('A' + (n % 26)));
        n = n / 26 - 1;
    } while (n >= 0);
    return label;
}

int WaypointModel::addWaypoint(double latitude, double longitude) {
    Waypoint wp;
    wp.label = nextLabel();
    wp.latitude = latitude;
    wp.longitude = longitude;
    wp.altitude = 30.0;

    const int row = waypoints_.size();
    beginInsertRows(QModelIndex(), row, row);
    waypoints_.append(wp);
    endInsertRows();
    emit waypointsChanged();
    return row;
}

void WaypointModel::setCoordinate(int index, double latitude, double longitude) {
    if (index < 0 || index >= waypoints_.size()) return;
    waypoints_[index].latitude = latitude;
    waypoints_[index].longitude = longitude;
    const QModelIndex idx = createIndex(index, 0);
    emit dataChanged(idx, idx, {LatitudeRole, LongitudeRole});
    emit waypointsChanged();
}

void WaypointModel::setAltitude(int index, double altitude) {
    if (index < 0 || index >= waypoints_.size()) return;
    waypoints_[index].altitude = altitude;
    const QModelIndex idx = createIndex(index, 0);
    emit dataChanged(idx, idx, {AltitudeRole});
    emit waypointsChanged();
}

void WaypointModel::removeWaypoint(int index) {
    if (index < 0 || index >= waypoints_.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    waypoints_.removeAt(index);
    endRemoveRows();
    emit waypointsChanged();
}

int WaypointModel::count() const {
    return waypoints_.size();
}

QGeoCoordinate WaypointModel::coordinateAt(int index) const {
    if (index < 0 || index >= waypoints_.size()) return QGeoCoordinate();
    return QGeoCoordinate(waypoints_.at(index).latitude, waypoints_.at(index).longitude);
}

QString WaypointModel::labelAt(int index) const {
    if (index < 0 || index >= waypoints_.size()) return QString();
    return waypoints_.at(index).label;
}

double WaypointModel::altitudeAt(int index) const {
    if (index < 0 || index >= waypoints_.size()) return 0.0;
    return waypoints_.at(index).altitude;
}
