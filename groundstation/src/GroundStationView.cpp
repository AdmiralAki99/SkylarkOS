#include "GroundStationView.hpp"

GroundStationView::GroundStationView(QWidget* parent): QWidget(parent) {

    telemetryClient_ = new TelemetryClient(this);

    waypointModel_ = new WaypointModel(this);
    waypointModel_->addWaypoint(37.7755, -122.4180);
    waypointModel_->addWaypoint(37.7762, -122.4170);
    waypointModel_->addWaypoint(37.7748, -122.4160);

    mapWidget_ = new MapWidget(waypointModel_, this);

    videoWidget_ = new GstVideoWidget(this);
    topBar_ = new TopBar(this);

    connect(topBar_, &TopBar::armToggled, this, [this]{
        armed_ = !armed_;
        qDebug() << "Armed state:" << armed_ << "(not yet wired to telemetry)";
    });

    connect(topBar_, &TopBar::modeSelected, this, [this](const QString &mode){
        qDebug() << "Mode selected:" << mode;
    });
    leftRail_ = new LeftRail(this);

    connect(leftRail_, &LeftRail::takeoffClicked, this, [this]{ qDebug() << "Takeoff clicked"; });
    connect(leftRail_, &LeftRail::returnClicked, this, [this]{ qDebug() << "Return/RTL clicked"; });
    connect(leftRail_, &LeftRail::pauseClicked, this, [this]{ qDebug() << "Pause clicked"; });
    connect(leftRail_, &LeftRail::armClicked, this, [this]{ qDebug() << "Arm clicked"; });
    
    missionPanel_ = new MissionPanel(waypointModel_, this);
    connect(missionPanel_, &MissionPanel::uploadRequested, this, [this]{ qDebug() << "Mission upload requested (no backend yet)"; });

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

    leftRail_->setStyleSheet("background: rgba(10,14,17,0.82); border-radius: 12px;");
    telemetryPanel_->setStyleSheet("background: rgba(10,14,17,0.85); border-radius: 12px;");
    this->setStyleSheet("background: rgba(10,14,17,0.9);");

    connect(videoWidget_, &GstVideoWidget::clicked, this, [this]{
        videoEnlarged_ = !videoEnlarged_;
        positionVideoWidget();
    });

    // Live telemetry wiring
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
    });

    connect(telemetryClient_, &TelemetryClient::tracksChanged, videoWidget_, &GstVideoWidget::setTracks);

}

GroundStationView::~GroundStationView(){

}

void GroundStationView::start(const std::string &host, int port){
    videoWidget_->start(host, port);

    QUrl telemetryUrl(QString("ws://%1:8765/ws").arg(QString::fromStdString(host)));
    telemetryClient_->connectTo(telemetryUrl);
}

void GroundStationView::resizeEvent(QResizeEvent* event){
    mapWidget_->setGeometry(0, 0, width(), height());
    topBar_->setGeometry(0, 0, width(), 52);
    leftRail_->setGeometry(16, 70, 76, 280);
    missionPanel_->setGeometry(104, 70, 230, height() - 100);
    telemetryPanel_->setGeometry(width() - 250 - 16, 70, 250, 220);
    compassWidget_->setGeometry(width() - 150 - 16, height() - 150 - 16, 150, 150);
    attitudeHorizonWidget_->setGeometry(width() - 150 - 14 - 150 - 16, height() - 150 - 16, 150, 150);
    droneOrientationWidget_->setGeometry(width() - 150 - 14 - 150 - 14 - 150 - 16, height() - 150 - 16, 150, 150);
    flightTimeStrip_->setGeometry((width() - 140) / 2, height() - 66 - 16, 140, 66);
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

void GroundStationView::positionVideoWidget(){
    if(videoEnlarged_){
        int width_ = std::min(int(width() * 0.72), 860);
        int height_ = std::min(int(height() * 0.72), 600);
        videoWidget_->setGeometry((this->width() - width_) / 2, (this->height() - height_) / 2, width_, height_);
    }else{
        videoWidget_->setGeometry(16, height() - 220 - 16, 220, 220);
    }
}