#ifndef FLIGHT_TIME_STRIP_HPP
#define FLIGHT_TIME_STRIP_HPP

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class FlightTimeStrip : public QWidget {
    Q_OBJECT
public:
    explicit FlightTimeStrip(QWidget *parent = nullptr);

    void setTime(const QString &hhmmss);

private:
    QVBoxLayout *layout_ = nullptr;
    QLabel *captionLabel_ = nullptr;
    QLabel *timeValue_ = nullptr;
};

#endif // FLIGHT_TIME_STRIP_HPP
