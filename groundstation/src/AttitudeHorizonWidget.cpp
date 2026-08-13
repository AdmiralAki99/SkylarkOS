#include "AttitudeHorizonWidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <algorithm>

AttitudeHorizonWidget::AttitudeHorizonWidget(QWidget *parent) : QWidget(parent) {
}

QSize AttitudeHorizonWidget::sizeHint() const {
    return QSize(150, 150);
}

void AttitudeHorizonWidget::setPitch(double degrees) {
    pitch_ = degrees;
    update();
}

void AttitudeHorizonWidget::setRoll(double degrees) {
    roll_ = degrees;
    update();
}

void AttitudeHorizonWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int side = std::min(width(), height());
    const QRectF gaugeRect((width() - side) / 2.0, (height() - side) / 2.0, side, side);

    QPainterPath clipPath;
    clipPath.addEllipse(gaugeRect);
    painter.setClipPath(clipPath);

    painter.save();
    painter.translate(gaugeRect.center());
    painter.rotate(-roll_);

    const double big = side * 2.0;
    const double pitchOffset = pitch_ * 2.2;
    QRectF skyRect(-big, -big + pitchOffset, big * 2, big);
    QRectF groundRect(-big, pitchOffset, big * 2, big);

    QLinearGradient skyGrad(0, skyRect.top(), 0, skyRect.bottom());
    skyGrad.setColorAt(0, QColor("#2a6bb5"));
    skyGrad.setColorAt(1, QColor("#5aa9ff"));
    painter.setPen(Qt::NoPen);
    painter.setBrush(skyGrad);
    painter.drawRect(skyRect);

    QLinearGradient groundGrad(0, groundRect.top(), 0, groundRect.bottom());
    groundGrad.setColorAt(0, QColor("#3e8f5a"));
    groundGrad.setColorAt(1, QColor("#1f5c39"));
    painter.setBrush(groundGrad);
    painter.drawRect(groundRect);

    painter.setPen(QPen(QColor("#e7edf2"), 2));
    painter.drawLine(QPointF(-big, pitchOffset), QPointF(big, pitchOffset));

    painter.restore();

    painter.setClipping(false);
    painter.save();
    painter.translate(gaugeRect.center());
    painter.setPen(QPen(QColor("#ffb020"), 2.5));
    painter.drawLine(QPointF(-20, 0), QPointF(-6, 0));
    painter.drawLine(QPointF(6, 0), QPointF(20, 0));
    painter.setBrush(QColor("#ffb020"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(0, 0), 2.5, 2.5);
    painter.restore();

    painter.setPen(QPen(QColor("#1c242a"), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(gaugeRect);
}
