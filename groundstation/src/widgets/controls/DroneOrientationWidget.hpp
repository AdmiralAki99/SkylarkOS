#ifndef DRONE_ORIENTATION_WIDGET_HPP
#define DRONE_ORIENTATION_WIDGET_HPP

#include <QWidget>

class DroneOrientationWidget : public QWidget {
    Q_OBJECT
public:
    explicit DroneOrientationWidget(QWidget *parent = nullptr);

    void setPitch(double degrees);
    void setRoll(double degrees);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double pitch_ = 0.0;
    double roll_ = 0.0;
};

#endif // DRONE_ORIENTATION_WIDGET_HPP
