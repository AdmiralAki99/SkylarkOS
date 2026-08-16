#include "AttitudeHorizonWidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <algorithm>
#include <cmath>

AttitudeHorizonWidget::AttitudeHorizonWidget(QWidget *parent) : QWidget(parent) {
}

QSize AttitudeHorizonWidget::sizeHint() const {
    return QSize(260, 150);
}

void AttitudeHorizonWidget::setPitch(double degrees) {
    pitch_ = degrees;
    update();
}

void AttitudeHorizonWidget::setRoll(double degrees) {
    roll_ = degrees;
    update();
}

void AttitudeHorizonWidget::setAltitude(double meters) {
    altitude_ = meters;
    update();
}

void AttitudeHorizonWidget::drawAltitudeTape(QPainter &painter, const QRectF &tapeRect) {
    const double centerY = tapeRect.center().y();
    const double pxPerMeter = (tapeRect.height() / 2.0 - 10.0) / 100.0;

    QFont valueFont = painter.font();
    valueFont.setFamily("IBM Plex Mono");

    for (int offset = -100; offset <= 100; offset += 25) {
        const double y = centerY - offset * pxPerMeter;
        const bool major = (offset % 100 == 0);
        const double tickX = tapeRect.right() - 6;
        const double tickLen = major ? 12 : 6;

        painter.setPen(QPen(QColor("#5f707c"), 1));
        painter.drawLine(QPointF(tickX - tickLen, y), QPointF(tickX, y));

        if (major) {
            valueFont.setPointSizeF(offset == 0 ? 15 : 11);
            valueFont.setBold(offset == 0);
            painter.setFont(valueFont);
            painter.setPen(QColor(offset == 0 ? "#e7edf2" : "#8fa3b0"));
            const QString label = QString::number(std::round(altitude_ + offset)) + " m";
            painter.drawText(QRectF(tapeRect.left(), y - 10, tapeRect.width() - tickLen - 12, 20),
                              Qt::AlignVCenter | Qt::AlignRight, label);
        }
    }

    painter.setPen(QPen(QColor("#e2685a"), 2));
    painter.drawLine(QPointF(tapeRect.left(), centerY), QPointF(tapeRect.right() - 6, centerY));
}

void AttitudeHorizonWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const double side = height();
    const double radius = side / 2.0;
    const QRectF gaugeRect(width() - side, 0, side, side);
    const QPointF gaugeCenter = gaugeRect.center();

    QPainterPath capsulePath;
    QPainterPath leftPath;
    leftPath.addRoundedRect(QRectF(0, 0, width() - radius, height()), 12, 12);
    QPainterPath circlePath;
    circlePath.addEllipse(gaugeRect);
    capsulePath = leftPath.united(circlePath);

    painter.setPen(QPen(QColor("#1c242a"), 1));
    painter.setBrush(QColor(10, 14, 17, 235));
    painter.drawPath(capsulePath);

    drawAltitudeTape(painter, QRectF(14, 0, width() - side - 14, height()));

    QPainterPath clipPath;
    clipPath.addEllipse(gaugeRect.adjusted(2, 2, -2, -2));
    painter.setClipPath(clipPath);

    painter.save();
    painter.translate(gaugeCenter);
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

    painter.setPen(QPen(QColor("#e2685a"), 2.5));
    painter.drawLine(QPointF(-big, pitchOffset), QPointF(big, pitchOffset));

    QFont ladderFont = painter.font();
    ladderFont.setFamily("IBM Plex Mono");
    ladderFont.setPointSizeF(9);
    painter.setFont(ladderFont);
    for (int rung : {10, -10}) {
        const double rungY = pitchOffset - rung * 2.2;
        painter.setPen(QPen(QColor("#e7edf2"), 1.5));
        painter.drawLine(QPointF(-26, rungY), QPointF(-10, rungY));
        painter.drawLine(QPointF(10, rungY), QPointF(26, rungY));
        painter.drawText(QRectF(-44, rungY - 7, 14, 14), Qt::AlignCenter, QString::number(rung));
        painter.drawText(QRectF(30, rungY - 7, 14, 14), Qt::AlignCenter, QString::number(rung));
    }

    painter.restore();

    painter.setClipping(false);
    painter.save();
    painter.translate(gaugeCenter);
    painter.setPen(QPen(QColor("#ffb020"), 2.5));
    painter.drawLine(QPointF(-20, 0), QPointF(-6, 0));
    painter.drawLine(QPointF(6, 0), QPointF(20, 0));
    painter.setBrush(QColor("#ffb020"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(0, 0), 2.5, 2.5);
    painter.restore();

    painter.save();
    painter.translate(gaugeCenter.x(), gaugeRect.top() + 8);
    QPolygonF bankMarker;
    bankMarker << QPointF(-5, 0) << QPointF(5, 0) << QPointF(0, 8);
    painter.setBrush(QColor("#e2685a"));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(bankMarker);
    painter.restore();

    painter.setPen(QPen(QColor("#1c242a"), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(gaugeRect.adjusted(1, 1, -1, -1));
}
