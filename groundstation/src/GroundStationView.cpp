#include "GroundStationView.hpp"

#include <QDateTime>
#include <cmath>
#include <algorithm>

GroundStationView::GroundStationView(QWidget* parent): QWidget(parent) {
    setObjectName("groundStationView");
    setAttribute(Qt::WA_StyledBackground, true);

    telemetryClient_ = new TelemetryClient(this);

    chartsPanel_ = new ChartsPanel(this);
    chartsPanel_->hide();

    waypointModel_ = new WaypointModel(this);
    waypointModel_->addWaypoint(37.7755, -122.4180);
    waypointModel_->addWaypoint(37.7762, -122.4170);
    waypointModel_->addWaypoint(37.7748, -122.4160);

    mapWidget_ = new MapWidget(waypointModel_, this);

    videoBackdrop_ = new QWidget(this);
    videoBackdrop_->setStyleSheet("background: rgba(3,5,7,0.6);");
    videoBackdrop_->setCursor(Qt::PointingHandCursor);
    videoBackdrop_->installEventFilter(this);
    videoBackdrop_->hide();

    videoWidget_ = new GstVideoWidget(this);
    topBar_ = new TopBar(this);

    connect(topBar_, &TopBar::armToggled, this, [this]{
        armed_ = !armed_;
        qDebug() << "Armed state:" << armed_ << "(not yet wired to telemetry)";
    });

    connect(topBar_, &TopBar::modeSelected, this, [this](const QString &mode){
        qDebug() << "Mode selected:" << mode;
    });

    connect(topBar_, &TopBar::chartsToggled, this, [this]{
        chartsVisible_ = !chartsVisible_;
        chartsPanel_->setVisible(chartsVisible_);
        if (chartsVisible_) chartsPanel_->raise();
    });
    leftRail_ = new LeftRail(this);

    connect(leftRail_, &LeftRail::takeoffClicked, this, [this]{ qDebug() << "Takeoff clicked"; });
    connect(leftRail_, &LeftRail::returnClicked, this, [this]{ qDebug() << "Return/RTL clicked"; });
    connect(leftRail_, &LeftRail::pauseClicked, this, [this]{ qDebug() << "Pause clicked"; });
    connect(leftRail_, &LeftRail::armClicked, this, [this]{ qDebug() << "Arm clicked"; });
    
    missionPanel_ = new MissionPanel(waypointModel_, this);
    connect(missionPanel_, &MissionPanel::uploadRequested, this, [this]{ qDebug() << "Mission upload requested (no backend yet)"; });
    connect(missionPanel_, &MissionPanel::collapsedChanged, this, [this]{ positionMissionPanel(); });

    telemetryPanel_ = new TelemetryPanel(this);

    telemetryPanel_->setAltitude(42.3);
    telemetryPanel_->setGroundSpeed(5.8);
    telemetryPanel_->setHeading(132.0);
    telemetryPanel_->setFlightTime("00:00:00");
    telemetryPanel_->setPitch(-4.0);
    telemetryPanel_->setRoll(8.0);
    telemetryPanel_->setJetsonStats(58.0, 34);

    compassWidget_ = new CompassWidget(this);
    compassWidget_->setHeading(132.0);

    attitudeHorizonWidget_ = new AttitudeHorizonWidget(this);
    attitudeHorizonWidget_->setPitch(-4.0);
    attitudeHorizonWidget_->setRoll(8.0);

    droneOrientationWidget_ = new DroneOrientationWidget(this);
    droneOrientationWidget_->setPitch(-4.0);
    droneOrientationWidget_->setRoll(8.0);

    flightTimeStrip_ = new FlightTimeStrip(this);

    flightTimer_ = new QTimer(this);
    connect(flightTimer_, &QTimer::timeout, this, &GroundStationView::tickFlightTime);
    flightTimer_->start(1000);

    connect(videoWidget_, &GstVideoWidget::clicked, this, [this]{
        videoEnlarged_ = !videoEnlarged_;
        videoWidget_->setEnlarged(videoEnlarged_);
        videoBackdrop_->setVisible(videoEnlarged_);
        if (videoEnlarged_) {
            videoBackdrop_->raise();
            videoWidget_->raise();
        }
        positionVideoWidget();
    });

    connect(telemetryClient_, &TelemetryClient::headingChanged, compassWidget_, &CompassWidget::setHeading);
    connect(telemetryClient_, &TelemetryClient::headingChanged, telemetryPanel_, &TelemetryPanel::setHeading);

    connect(telemetryClient_, &TelemetryClient::pitchChanged, attitudeHorizonWidget_, &AttitudeHorizonWidget::setPitch);
    connect(telemetryClient_, &TelemetryClient::rollChanged, attitudeHorizonWidget_, &AttitudeHorizonWidget::setRoll);
    connect(telemetryClient_, &TelemetryClient::pitchChanged, droneOrientationWidget_, &DroneOrientationWidget::setPitch);
    connect(telemetryClient_, &TelemetryClient::rollChanged, droneOrientationWidget_, &DroneOrientationWidget::setRoll);
    connect(telemetryClient_, &TelemetryClient::pitchChanged, telemetryPanel_, &TelemetryPanel::setPitch);
    connect(telemetryClient_, &TelemetryClient::rollChanged, telemetryPanel_, &TelemetryPanel::setRoll);

    connect(telemetryClient_, &TelemetryClient::altitudeChanged, telemetryPanel_, &TelemetryPanel::setAltitude);
    connect(telemetryClient_, &TelemetryClient::groundSpeedChanged, telemetryPanel_, &TelemetryPanel::setGroundSpeed);

    connect(telemetryClient_, &TelemetryClient::armingStateChanged, this, [this](bool armed){
        armed_ = armed;
        topBar_->setArmed(armed);
    });

    connect(telemetryClient_, &TelemetryClient::connectionStateChanged, topBar_, &TopBar::setConnected);
    connect(telemetryClient_, &TelemetryClient::connectionStateChanged, this, [this](bool connected){
        telemetryConnected_ = connected;
    });

    connect(telemetryClient_, &TelemetryClient::tracksChanged, videoWidget_, &GstVideoWidget::setTracks);

    connect(telemetryClient_, &TelemetryClient::batteryChanged, this, [this](double voltage, double remainingFraction){
        Q_UNUSED(voltage);
        lastBatteryPercent_ = remainingFraction * 100.0;
        topBar_->setBattery(lastBatteryPercent_);
        chartsPanel_->pushBattery(lastBatteryPercent_);
    });

    connect(telemetryClient_, &TelemetryClient::gpsChanged, this, [this](double latitude, double longitude, int satellites){
        topBar_->setSatellites(satellites);
        mapWidget_->setVehiclePosition(latitude, longitude);
    });

    mockJetsonStatsTimer_ = new QTimer(this);
    connect(mockJetsonStatsTimer_, &QTimer::timeout, this, &GroundStationView::tickMockJetsonStats);
    mockJetsonStatsTimer_->start(500);
}

GroundStationView::~GroundStationView(){

}

bool GroundStationView::eventFilter(QObject *watched, QEvent *event){
    if (watched == videoBackdrop_ && event->type() == QEvent::MouseButtonPress) {
        videoEnlarged_ = false;
        videoWidget_->setEnlarged(false);
        videoBackdrop_->hide();
        positionVideoWidget();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void GroundStationView::start(const std::string &host, int port){
    videoWidget_->start(host, port);

    QUrl telemetryUrl(QString("ws://%1:8765/ws").arg(QString::fromStdString(host)));
    telemetryClient_->connectTo(telemetryUrl);
}

void GroundStationView::resizeEvent(QResizeEvent* event){
    mapWidget_->setGeometry(0, 0, width(), height());
    videoBackdrop_->setGeometry(0, 0, width(), height());
    topBar_->setGeometry(0, 0, width(), 52);
    leftRail_->setGeometry(16, 70, 76, 280);
    positionMissionPanel();
    telemetryPanel_->setGeometry(width() - 250 - 16, 70, 250, 290);
    compassWidget_->setGeometry(width() - 150 - 16, height() - 150 - 16, 150, 150);
    attitudeHorizonWidget_->setGeometry(width() - 150 - 14 - 150 - 16, height() - 150 - 16, 150, 150);
    droneOrientationWidget_->setGeometry(width() - 150 - 14 - 150 - 14 - 150 - 16, height() - 150 - 16, 150, 150);
    flightTimeStrip_->setGeometry((width() - 140) / 2, height() - 66 - 16, 140, 66);
    {
        const int chartsWidth = std::min(int(width() * 0.6), 680);
        const int chartsHeight = std::min(int(height() * 0.82), 720);
        chartsPanel_->setGeometry((width() - chartsWidth) / 2, (height() - chartsHeight) / 2, chartsWidth, chartsHeight);
    }
    positionVideoWidget();
}

void GroundStationView::tickFlightTime(){
    if (armed_) {
        flightSeconds_++;
    }
    const int h = flightSeconds_ / 3600;
    const int m = (flightSeconds_ % 3600) / 60;
    const int s = flightSeconds_ % 60;
    const QString formatted = QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
    flightTimeStrip_->setTime(formatted);
    telemetryPanel_->setFlightTime(formatted);
}

void GroundStationView::tickMockJetsonStats(){
    const double now = QDateTime::currentMSecsSinceEpoch() / 1000.0;

    if (!telemetryConnected_) {
        const double mockBattery = std::clamp(87.0 - std::fmod(now, 300.0) / 10.0, 20.0, 100.0);
        chartsPanel_->pushBattery(mockBattery);
    }

    const double gpuLoad = std::clamp(40.0 + std::sin(now / 3.0) * 20.0 + std::sin(now / 0.7) * 5.0, 0.0, 100.0);
    chartsPanel_->pushGpu(gpuLoad);

    const double baseTemps[4] = {52.0, 55.0, 58.0, 61.0};
    double lastCoreTemp = baseTemps[0];
    for (int core = 0; core < 4; ++core) {
        lastCoreTemp = baseTemps[core] + std::sin(now / 4.0 + core) * 6.0;
        chartsPanel_->pushThermalCore(core, lastCoreTemp);
    }

    telemetryPanel_->setJetsonStats(lastCoreTemp, int(gpuLoad));
}

void GroundStationView::positionMissionPanel(){
    const int panelHeight = missionPanel_->isCollapsed()
        ? missionPanel_->collapsedHeight()
        : height() - 100;
    missionPanel_->setGeometry(104, 70, 230, panelHeight);
}

void GroundStationView::positionVideoWidget(){
    if(videoEnlarged_){
        int width_ = std::min(int(width() * 0.72), 860);
        int height_ = std::min(int(height() * 0.72), 600);
        videoWidget_->setGeometry((this->width() - width_) / 2, (this->height() - height_) / 2, width_, height_);
    }else{
        videoWidget_->setGeometry(16, height() - 220 - 16, 220, 220);
    }
}