#include "DroneOrientationWidget.hpp"

#include <QPainter>
#include <QRadialGradient>
#include <QLinearGradient>
#include <cmath>
#include <algorithm>

DroneOrientationWidget::DroneOrientationWidget(QWidget *parent) : QWidget(parent) {
}

QSize DroneOrientationWidget::sizeHint() const {
    return QSize(150, 150);
}

void DroneOrientationWidget::setPitch(double degrees) {
    pitch_ = degrees;
    update();
}

void DroneOrientationWidget::setRoll(double degrees) {
    roll_ = degrees;
    update();
}

void DroneOrientationWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF panelRect = rect();

    painter.setPen(QPen(QColor("#1c242a"), 1));
    painter.setBrush(QColor(10, 14, 17, 217));
    painter.drawRoundedRect(panelRect.adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);

    QFont captionFont = painter.font();
    captionFont.setFamily("IBM Plex Mono");
    captionFont.setPointSizeF(7.5);
    painter.setFont(captionFont);
    painter.setPen(QColor("#5f707c"));
    painter.drawText(QRectF(0, 8, panelRect.width(), 12), Qt::AlignHCenter, "ORIENTATION");

    painter.save();
    painter.translate(panelRect.center());

    painter.rotate(roll_);
    const double pitchClamped = std::clamp(pitch_ * 2.5, -80.0, 80.0);
    const double pitchScale = std::cos(pitchClamped * M_PI / 180.0);
    painter.scale(1.0, std::max(0.15, std::abs(pitchScale)));

    QLinearGradient armGrad(0, -5, 0, 5);
    armGrad.setColorAt(0, QColor("#3a4a56"));
    armGrad.setColorAt(1, QColor("#1c262c"));
    painter.setPen(Qt::NoPen);
    painter.setBrush(armGrad);
    painter.drawRoundedRect(QRectF(-30, -5, 60, 10), 4, 4);
    painter.drawRoundedRect(QRectF(-5, -30, 10, 60), 4, 4);

    const QPointF motorPositions[4] = {
        QPointF(-34, -34), QPointF(34, -34), QPointF(-34, 34), QPointF(34, 34)
    };
    for (const QPointF &pos : motorPositions) {
        QRadialGradient motorGrad(pos + QPointF(-3, -3), 10);
        motorGrad.setColorAt(0, QColor("#5aa9ff"));
        motorGrad.setColorAt(1, QColor("#1a4f8f"));
        painter.setBrush(motorGrad);
        painter.drawEllipse(pos, 8, 8);
    }

    QLinearGradient hubGrad(-11, -11, 11, 11);
    hubGrad.setColorAt(0, QColor("#dfe8ef"));
    hubGrad.setColorAt(1, QColor("#8fa3b0"));
    painter.setBrush(hubGrad);
    painter.drawRoundedRect(QRectF(-11, -11, 22, 22), 6, 6);

    painter.restore();
}
