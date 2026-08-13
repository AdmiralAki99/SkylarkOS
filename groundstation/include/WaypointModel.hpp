#ifndef WAYPOINT_MODEL_HPP
#define WAYPOINT_MODEL_HPP

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QVector>

struct Waypoint {
    QString label;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 30.0;
};

class WaypointModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        LabelRole = Qt::UserRole + 1,
        LatitudeRole,
        LongitudeRole,
        AltitudeRole
    };

    explicit WaypointModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE int addWaypoint(double latitude, double longitude);
    Q_INVOKABLE void setCoordinate(int index, double latitude, double longitude);
    Q_INVOKABLE void setAltitude(int index, double altitude);
    Q_INVOKABLE void removeWaypoint(int index);
    Q_INVOKABLE int count() const;
    Q_INVOKABLE QGeoCoordinate coordinateAt(int index) const;

    QString labelAt(int index) const;
    double altitudeAt(int index) const;

signals:
    void waypointsChanged();

private:
    QVector<Waypoint> waypoints_;
    QString nextLabel() const;
};

#endif // WAYPOINT_MODEL_HPP
