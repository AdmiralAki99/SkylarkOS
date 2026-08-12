#include "TelemetryClient.hpp"

#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <cmath>

TelemetryClient::TelemetryClient(QObject *parent): QObject(parent){
    socket_ = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    connect(socket_, &QWebSocket::connected, this, &TelemetryClient::onConnected);
    connect(socket_, &QWebSocket::disconnected, this, &TelemetryClient::onDisconnected);
    connect(socket_, &QWebSocket::textMessageReceived, this, &TelemetryClient::onTextMessageReceived);

    // TEMPORARY debug logging — remove once telemetry wiring is confirmed working.
    connect(socket_, &QWebSocket::errorOccurred, this, [](QAbstractSocket::SocketError error){
        qDebug() << "[TelemetryClient] WebSocket error:" << error;
    });
}

TelemetryClient::~TelemetryClient(){

}

void TelemetryClient::parseAndEmit(const QByteArray& json){
    QJsonDocument document = QJsonDocument::fromJson(json);
    const QJsonObject jsonObject = document.object();

    float positionX = jsonObject["position"]["x"].toDouble();
    float positionY = jsonObject["position"]["y"].toDouble();
    float positionZ = jsonObject["position"]["z"].toDouble();
    float velocityX = jsonObject["velocity"]["x"].toDouble();
    float velocityY = jsonObject["velocity"]["y"].toDouble();
    float velocityZ = jsonObject["velocity"]["z"].toDouble();

    int armingState = jsonObject["arming_state"].toInt();
    int navState = jsonObject["nav_state"].toInt();
    
    float batteryVoltage = jsonObject["battery_voltage"].toDouble();
    float batteryRemaining = jsonObject["battery_remaining"].toDouble();

    float altitude = jsonObject["altitude"].toDouble();

    QVector<Track> tracks;

    for(const QJsonValue &track : jsonObject["tracks"].toArray()){
        tracks.append({
            .id= track["id"].toInt(),
            .classId=track["class_id"].toInt(),
            .x1=static_cast<float>(track["x1"].toDouble()),
            .y1=static_cast<float>(track["y1"].toDouble()),
            .x2=static_cast<float>(track["x2"].toDouble()),
            .y2=static_cast<float>(track["y2"].toDouble())
        });
    }

    QString gesture = jsonObject["gesture"].toString();

    float heading = jsonObject["heading"].toDouble() * 180 / M_PI;
    float pitch = jsonObject["pitch"].toDouble() * 180 / M_PI;
    float roll = jsonObject["roll"].toDouble() * 180 / M_PI;

    float latitude = jsonObject["latitude"].toDouble();
    float longitude = jsonObject["longitude"].toDouble();

    int satellites = jsonObject["satellites"].toInt();

    float groundSpeed = sqrt((velocityX*velocityX + velocityY*velocityY));

    emit armingStateChanged(armingState == 2);
    emit groundSpeedChanged(groundSpeed);
    emit pitchChanged(pitch);
    emit headingChanged(heading);
    emit rollChanged(roll);
    emit altitudeChanged(altitude);
    emit batteryChanged(batteryVoltage, batteryRemaining);
    emit gpsChanged(latitude,longitude,satellites);
    emit tracksChanged(tracks);
}

void TelemetryClient::onConnected(){
    qDebug() << "[TelemetryClient] connected";
    emit connectionStateChanged(true);
}

void TelemetryClient::onDisconnected(){
    qDebug() << "[TelemetryClient] disconnected";
    emit connectionStateChanged(false);
}

void TelemetryClient::onTextMessageReceived(const QString& message){
    qDebug() << "[TelemetryClient] message received, length:" << message.length();
    parseAndEmit(message.toUtf8());
}

void TelemetryClient::connectTo(const QUrl& url){
    qDebug() << "[TelemetryClient] connecting to" << url;
    socket_->open(url);
}

void TelemetryClient::sendCommand(const QString& cmd){
    QJsonObject jsonObject{{"cmd", cmd}};
    QByteArray jsonBytes = QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
    socket_->sendTextMessage(QString::fromUtf8(jsonBytes));
}