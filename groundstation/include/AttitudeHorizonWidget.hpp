#ifndef ATTITUDE_HORIZON_WIDGET_HPP
#define ATTITUDE_HORIZON_WIDGET_HPP

#include <QWidget>

class AttitudeHorizonWidget : public QWidget {
    Q_OBJECT
public:
    explicit AttitudeHorizonWidget(QWidget *parent = nullptr);

    void setPitch(double degrees);
    void setRoll(double degrees);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double pitch_ = 0.0;
    double roll_ = 0.0;
};

#endif // ATTITUDE_HORIZON_WIDGET_HPP
