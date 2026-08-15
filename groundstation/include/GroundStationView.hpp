#ifndef GROUND_STATION_VIEW_HPP
#define GROUND_STATION_VIEW_HPP

#include <QWidget>
#include <string>

#include "GstVideoWidget.hpp"
#include "TopBar.hpp"
#include "LeftRail.hpp"
#include "TelemetryPanel.hpp"
#include "CompassWidget.hpp"
#include "AttitudeHorizonWidget.hpp"
#include "DroneOrientationWidget.hpp"
#include "FlightTimeStrip.hpp"
#include "MapWidget.hpp"
#include "MissionPanel.hpp"
#include "WaypointModel.hpp"
#include "TelemetryClient.hpp"
#include "ChartsPanel.hpp"
#include "NudgePad.hpp"
#include <QTimer>

class GroundStationView: public QWidget{
        Q_OBJECT
    public:
        GroundStationView(QWidget* parent = nullptr);
        ~GroundStationView();

        void start(const std::string &host, int port);
    protected:
        void resizeEvent(QResizeEvent* event) override;
        bool eventFilter(QObject *watched, QEvent *event) override;

    private:
        WaypointModel *waypointModel_ = nullptr;
        MapWidget *mapWidget_ = nullptr;
        GstVideoWidget *videoWidget_ = nullptr;
        QWidget *videoBackdrop_ = nullptr;
        TopBar *topBar_ = nullptr;
        LeftRail *leftRail_ = nullptr;
        MissionPanel *missionPanel_ = nullptr;
        TelemetryPanel *telemetryPanel_ = nullptr;
        CompassWidget *compassWidget_ = nullptr;
        AttitudeHorizonWidget *attitudeHorizonWidget_ = nullptr;
        DroneOrientationWidget *droneOrientationWidget_ = nullptr;
        FlightTimeStrip *flightTimeStrip_ = nullptr;
        TelemetryClient *telemetryClient_ = nullptr;
        ChartsPanel *chartsPanel_ = nullptr;
        NudgePad *nudgePad_ = nullptr;
        bool videoEnlarged_ = false;
        bool chartsVisible_ = false;

        bool armed_ = false;
        int flightSeconds_ = 0;
        QTimer *flightTimer_ = nullptr;
        QTimer *mockJetsonStatsTimer_ = nullptr;
        double lastBatteryPercent_ = 87.0;
        double lastGpuLoad_ = 0.0;
        double lastJetsonTemp_ = 0.0;
        bool telemetryConnected_ = false;

        void positionVideoWidget();
        void positionMissionPanel();
        void tickFlightTime();
        void tickMockJetsonStats();
};

#endif // GROUND_STATION_VIEW_HPP