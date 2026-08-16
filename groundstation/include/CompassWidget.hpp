#ifndef COMPASS_WIDGET_HPP
#define COMPASS_WIDGET_HPP

#include <QWidget>

class CompassWidget : public QWidget {
    Q_OBJECT
public:
    explicit CompassWidget(QWidget *parent = nullptr);

    void setHeading(double degrees);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double heading_ = 0.0;
};

#endif // COMPASS_WIDGET_HPP
