#ifndef TELEMETRY_CLIENT_HPP
#define TELEMETRY_CLIENT_HPP

#include <QVector>
#include <QUrl>
#include <QObject>
#include <QTimer>

class QWebSocket;

struct Track{
    int id;
    int classId;
    float x1;
    float y1;
    float x2;
    float y2;
};

class TelemetryClient: public QObject{
    Q_OBJECT
    public:
        TelemetryClient(QObject *parent = nullptr);
        ~TelemetryClient();

        void connectTo(const QUrl& url);
        void connectCommand(const QUrl& url);
        void sendCommand(const QString& cmd);

        signals:
            void connectionStateChanged(bool connected);
            void altitudeChanged(double meters);
            void groundSpeedChanged(double);
            void headingChanged(double);
            void pitchChanged(double);
            void rollChanged(double);
            void armingStateChanged(bool armed);
            void batteryChanged(double voltage, double remainingFraction);
            void gpsChanged(double lat, double lon, int satellites);
            void tracksChanged(const QVector<Track>&);
            void gpuLoadChanged(double percent);
            void jetsonTempChanged(double celsius);
            void coreTempsChanged(const QVector<double>& celsius);

        private:
            void onConnected();
            void onDisconnected();
            void onTextMessageReceived(const QString& message);
            void parseAndEmit(const QByteArray& json);
            void onCommandConnected();
            void onCommandDisconnected();
            void sendHeartbeat();

            QWebSocket *socket_ = nullptr;
            QWebSocket *commandSocket_ = nullptr;
            QTimer *heartbeatTimer_ = nullptr;

};


#endif