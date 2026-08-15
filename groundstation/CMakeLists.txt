cmake_minimum_required(VERSION 3.16)
project(groundstation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebSockets Quick QuickWidgets Positioning Location Charts)

add_executable(groundstation
    src/main.cpp
    src/GstVideoWidget.cpp
    include/GstVideoWidget.hpp
    src/GroundStationView.cpp
    include/GroundStationView.hpp
    src/TopBar.cpp
    include/TopBar.hpp
    src/LeftRail.cpp
    include/LeftRail.hpp
    src/TelemetryPanel.cpp
    include/TelemetryPanel.hpp
    src/CompassWidget.cpp
    include/CompassWidget.hpp
    src/AttitudeHorizonWidget.cpp
    include/AttitudeHorizonWidget.hpp
    src/DroneOrientationWidget.cpp
    include/DroneOrientationWidget.hpp
    src/FlightTimeStrip.cpp
    include/FlightTimeStrip.hpp
    src/MapWidget.cpp
    include/MapWidget.hpp
    src/MissionPanel.cpp
    include/MissionPanel.hpp
    src/WaypointModel.cpp
    include/WaypointModel.hpp
    include/TelemetryClient.hpp
    src/TelemetryClient.cpp
    src/ChartsPanel.cpp
    include/ChartsPanel.hpp
    src/MetricHistory.cpp
    include/MetricHistory.hpp
    src/NudgePad.cpp
    include/NudgePad.hpp
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
    PkgConfig::GST
)

target_include_directories(groundstation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
