import QtQuick
import QtLocation
import QtPositioning

Item {
    property bool hasCenteredOnVehicle: false

    Plugin {
        id: mapPlugin
        name: "osm"
        PluginParameter { name: "osm.mapping.providersrepository.disabled"; value: true }
        PluginParameter { name: "osm.mapping.host"; value: "https://tile.openstreetmap.org/" }
        PluginParameter { name: "osm.useragent"; value: "SkylarkGroundStation/1.0" }
        PluginParameter { name: "osm.mapping.custom.host"; value: "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%z/%y/%x" }
        PluginParameter { name: "osm.mapping.custom.mapcopyright"; value: "Esri, Maxar, Earthstar Geographics" }
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(37.7749, -122.4194)
        zoomLevel: 15

        WheelHandler {
            target: null
            onWheel: (event) => {
                const step = event.angleDelta.y > 0 ? 0.5 : -0.5;
                map.zoomLevel = Math.max(map.minimumZoomLevel, Math.min(map.maximumZoomLevel, map.zoomLevel + step));
            }
        }

        MapCircle {
            id: geofence
            center: QtPositioning.coordinate(37.7749, -122.4194)
            radius: 200.0
            color: Qt.rgba(0.35, 0.66, 1.0, 0.15)
            border.color: "#5aa9ff"
            border.width: 2
        }

        MapQuickItem {
            id: geofenceHandle
            coordinate: geofence.center.atDistanceAndAzimuth(geofence.radius, 90)
            anchorPoint.x: handleIcon.width / 2
            anchorPoint.y: handleIcon.height / 2
            sourceItem: Rectangle {
                id: handleIcon
                width: 16
                height: 16
                radius: 8
                color: "#5aa9ff"
                border.color: "#ffffff"
                border.width: 2
                MouseArea {
                    anchors.fill: parent
                    drag.target: parent
                    onPositionChanged: {
                        if (!drag.active) return;
                        var scenePos = handleIcon.mapToItem(map, handleIcon.width / 2, handleIcon.height / 2);
                        var coord = map.toCoordinate(scenePos);
                        var newRadius = geofence.center.distanceTo(coord);
                        if (newRadius > 10) geofence.radius = newRadius;
                        handleIcon.x = 0;
                        handleIcon.y = 0;
                    }
                }
            }
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

        Component.onCompleted: {
            map.rebuildSegments();
            for (var i = 0; i < map.supportedMapTypes.length; i++) {
                if (map.supportedMapTypes[i].style === MapType.CustomMap) {
                    map.activeMapType = map.supportedMapTypes[i];
                    break;
                }
            }
        }
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
                        font.family: "IBM Plex Mono"
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
                        font.family: "Space Grotesk"
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

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: (mouse) => {
                var coord = map.toCoordinate(Qt.point(mouse.x, mouse.y));
                waypointModel.addWaypoint(coord.latitude, coord.longitude);
            }
        }

        Column {
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 376
            spacing: 6
            z: 10

            Rectangle {
                width: 32
                height: 32
                radius: 8
                color: "#0a0e11"
                opacity: 0.9
                border.color: "#1c242a"
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: "+"
                    color: "#e7edf2"
                    font.family: "Space Grotesk"
                    font.pixelSize: 18
                    font.bold: true
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: map.zoomLevel = Math.min(map.maximumZoomLevel, map.zoomLevel + 1)
                }
            }

            Rectangle {
                width: 32
                height: 32
                radius: 8
                color: "#0a0e11"
                opacity: 0.9
                border.color: "#1c242a"
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: "−"
                    color: "#e7edf2"
                    font.family: "Space Grotesk"
                    font.pixelSize: 18
                    font.bold: true
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: map.zoomLevel = Math.max(map.minimumZoomLevel, map.zoomLevel - 1)
                }
            }
        }
    }

    function setVehicleCoordinate(lat, lon) {
        const coord = QtPositioning.coordinate(lat, lon);
        droneMarker.coordinate = coord;
        if (!hasCenteredOnVehicle) {
            map.center = coord;
            hasCenteredOnVehicle = true;
        }
    }
}
