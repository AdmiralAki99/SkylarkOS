cmake_minimum_required(VERSION 3.16)
project(groundstation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebSockets Quick QuickWidgets Positioning Location Charts Svg)

add_executable(groundstation
    src/main.cpp

    src/core/GroundStationView.cpp
    src/core/GroundStationView.hpp
    src/core/TelemetryClient.cpp
    src/core/TelemetryClient.hpp

    src/widgets/video/GstVideoWidget.cpp
    src/widgets/video/GstVideoWidget.hpp

    src/widgets/telemetry/TelemetryPanel.cpp
    src/widgets/telemetry/TelemetryPanel.hpp
    src/widgets/telemetry/CompassWidget.cpp
    src/widgets/telemetry/CompassWidget.hpp
    src/widgets/telemetry/AttitudeHorizonWidget.cpp
    src/widgets/telemetry/AttitudeHorizonWidget.hpp

    src/widgets/charts/ChartsPanel.cpp
    src/widgets/charts/ChartsPanel.hpp
    src/widgets/charts/MetricHistory.cpp
    src/widgets/charts/MetricHistory.hpp

    src/widgets/mission/MissionPanel.cpp
    src/widgets/mission/MissionPanel.hpp
    src/widgets/mission/WaypointModel.cpp
    src/widgets/mission/WaypointModel.hpp

    src/widgets/map/MapWidget.cpp
    src/widgets/map/MapWidget.hpp

    src/widgets/controls/TopBar.cpp
    src/widgets/controls/TopBar.hpp
    src/widgets/controls/LeftRail.cpp
    src/widgets/controls/LeftRail.hpp
    src/widgets/controls/FlightTimeStrip.cpp
    src/widgets/controls/FlightTimeStrip.hpp
    src/widgets/controls/DroneOrientationWidget.cpp
    src/widgets/controls/DroneOrientationWidget.hpp
    src/widgets/controls/NudgePad.cpp
    src/widgets/controls/NudgePad.hpp
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED IMPORTED_TARGET
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-video-1.0
)

target_link_libraries(groundstation PRIVATE
    Qt6::Widgets
    Qt6::WebSockets
    Qt6::Quick
    Qt6::QuickWidgets
    Qt6::Positioning
    Qt6::Location
    Qt6::Charts
    Qt6::Svg
    PkgConfig::GST
)

target_include_directories(groundstation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/video
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/charts
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/mission
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/map
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/controls
)
