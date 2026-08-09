#ifndef TELEMETRY_PANEL_HPP
#define TELEMETRY_PANEL_HPP

#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>

class TelemetryPanel : public QWidget {
    Q_OBJECT
public:
    explicit TelemetryPanel(QWidget *parent = nullptr);

    // Setters for later wiring to the WebSocket telemetry client
    void setAltitude(double meters);
    void setGroundSpeed(double mps);
    void setHeading(double degrees);
    void setFlightTime(const QString &hhmmss);
    void setPitch(double degrees);
    void setRoll(double degrees);
    void setJetsonStats(double tempC, int gpuLoadPct);
    
private:
    QGridLayout *layout_ = nullptr;
    QLabel *altitudeValue_ = nullptr;
    QLabel *groundSpeedValue_ = nullptr;
    QLabel *headingValue_ = nullptr;
    QLabel *flightTimeValue_ = nullptr;
    QLabel *pitchValue_ = nullptr;
    QLabel *rollValue_ = nullptr;
    QLabel *jetsonStatsValue_ = nullptr;

    QLabel* addField(const QString &caption, int row, int col);
};

#endif