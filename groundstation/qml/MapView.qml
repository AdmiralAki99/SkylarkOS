import QtQuick
import QtLocation
import QtPositioning

Item {
    Plugin {
        id: mapPlugin
        name: "osm"
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(37.7749, -122.4194)
        zoomLevel: 15

        MapCircle {
            id: geofence
            center: QtPositioning.coordinate(37.7749, -122.4194)
            radius: 200.0 // meters
            color: Qt.rgba(0.35, 0.66, 1.0, 0.15)
            border.color: "#5aa9ff"
            border.width: 2
        }

        MapPolyline {
            id: pathLine
            line.width: 2
            line.color: "#dfe8ef"

            function rebuild() {
                if (!waypointModel) return;
                var pts = [];
                for (var i = 0; i < waypointModel.count(); i++) {
                    pts.push(waypointModel.coordinateAt(i));
                }
                pathLine.path = pts;
            }

            Component.onCompleted: rebuild()
            Connections {
                target: waypointModel
                function onWaypointsChanged() { pathLine.rebuild(); }
            }
        }

        // waypointModel.count()/coordinateAt() are invokable methods, not
        // properties, so bindings that call them don't reliably
        // re-evaluate — rebuilt imperatively into a plain ListModel instead.
        ListModel {
            id: segmentModel
        }

        function rebuildSegments() {
            segmentModel.clear();
            if (!waypointModel) return;
            const n = waypointModel.count();
            for (var i = 0; i < n - 1; i++) {
                const from = waypointModel.coordinateAt(i);
                const to = waypointModel.coordinateAt(i + 1);
                segmentModel.append({
                    midLat: (from.latitude + to.latitude) / 2,
                    midLon: (from.longitude + to.longitude) / 2,
                    distanceText: Math.round(from.distanceTo(to)) + " m"
                });
            }
        }

        Component.onCompleted: map.rebuildSegments()
        Connections {
            target: waypointModel
            function onWaypointsChanged() { map.rebuildSegments(); }
        }

        MapItemView {
            model: segmentModel
            delegate: MapQuickItem {
                required property double midLat
                required property double midLon
                required property string distanceText
                coordinate: QtPositioning.coordinate(midLat, midLon)
                anchorPoint.x: distLabel.width / 2
                anchorPoint.y: distLabel.height / 2
                sourceItem: Rectangle {
                    id: distLabel
                    color: "#060a0d"
                    opacity: 0.75
                    radius: 4
                    width: distText.width + 8
                    height: distText.height + 4
                    Text {
                        id: distText
                        anchors.centerIn: parent
                        color: "#e7edf2"
                        font.pixelSize: 10
                        text: distanceText
                    }
                }
            }
        }

        MapItemView {
            model: waypointModel
            delegate: MapQuickItem {
                id: wpItem
                required property int index
                required property string label
                required property double latitude
                required property double longitude
                coordinate: QtPositioning.coordinate(latitude, longitude)
                anchorPoint.x: wpIcon.width / 2
                anchorPoint.y: wpIcon.height / 2
                sourceItem: Rectangle {
                    id: wpIcon
                    width: 24
                    height: 24
                    radius: 12
                    color: "#1a4f8f"
                    border.color: "#5aa9ff"
                    border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: wpItem.label
                        color: "white"
                        font.pixelSize: 11
                        font.bold: true
                    }
                    MouseArea {
                        anchors.fill: parent
                        drag.target: parent
                        onReleased: {
                            var scenePos = wpIcon.mapToItem(map, wpIcon.width / 2, wpIcon.height / 2);
                            var coord = map.toCoordinate(scenePos);
                            waypointModel.setCoordinate(wpItem.index, coord.latitude, coord.longitude);
                            // Model update above re-centers the item itself.
                            wpIcon.x = 0;
                            wpIcon.y = 0;
                        }
                    }
                }
            }
        }

        MapQuickItem {
            id: droneMarker
            coordinate: QtPositioning.coordinate(37.7749, -122.4194)
            anchorPoint.x: droneIcon.width / 2
            anchorPoint.y: droneIcon.height / 2
            sourceItem: Rectangle {
                id: droneIcon
                width: 20
                height: 20
                radius: 10
                color: "#46c88c"
                border.color: "#ffffff"
                border.width: 2
            }
        }

        MapQuickItem {
            id: laptopMarker
            coordinate: QtPositioning.coordinate(37.7745, -122.4198)
            anchorPoint.x: laptopIcon.width / 2
            anchorPoint.y: laptopIcon.height / 2
            sourceItem: Rectangle {
                id: laptopIcon
                width: 18
                height: 18
                color: "#5aa9ff"
                border.color: "#ffffff"
                border.width: 2
            }
        }

        MapQuickItem {
            id: watchMarker
            coordinate: QtPositioning.coordinate(37.7752, -122.4190)
            anchorPoint.x: watchIcon.width / 2
            anchorPoint.y: watchIcon.height / 2
            sourceItem: Rectangle {
                id: watchIcon
                width: 18
                height: 18
                radius: 9
                color: "#ffb020"
                border.color: "#ffffff"
                border.width: 2
            }
        }

        // Right-click, not left, since left-drag pans the map.
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: (mouse) => {
                var coord = map.toCoordinate(Qt.point(mouse.x, mouse.y));
                waypointModel.addWaypoint(coord.latitude, coord.longitude);
            }
        }
    }
}
