#include "CompassWidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <cmath>

CompassWidget::CompassWidget(QWidget *parent) : QWidget(parent) {
}

QSize CompassWidget::sizeHint() const {
    return QSize(150, 150);
}

void CompassWidget::setHeading(double degrees) {
    heading_ = degrees;
    update();
}

void CompassWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int side = std::min(width(), height());
    const QRectF dialRect((width() - side) / 2.0, (height() - side) / 2.0, side, side);

    painter.setPen(QPen(QColor("#1c242a"), 1));
    painter.setBrush(QColor(10, 14, 17, 217));
    painter.drawEllipse(dialRect);

    const double radius = side / 2.0 - 18;
    const QPointF center = dialRect.center();

    const double tickOuterR = side / 2.0 - 6;
    for (int deg = 0; deg < 360; deg += 30) {
        const bool major = (deg % 90 == 0);
        const double rad = qDegreesToRadians(deg - 90.0);
        const double innerR = tickOuterR - (major ? 10 : 6);
        const QPointF outer = center + QPointF(std::cos(rad) * tickOuterR, std::sin(rad) * tickOuterR);
        const QPointF inner = center + QPointF(std::cos(rad) * innerR, std::sin(rad) * innerR);
        painter.setPen(QPen(QColor(major ? "#8fa3b0" : "#5f707c"), major ? 1.5 : 1));
        painter.drawLine(outer, inner);
    }

    auto drawDirLabel = [&](const QString &text, double angleDeg, const QColor &color, bool bold) {
        const double rad = qDegreesToRadians(angleDeg - 90.0);
        const QPointF pos = center + QPointF(std::cos(rad) * radius, std::sin(rad) * radius);

        QFont font = painter.font();
        font.setFamily("IBM Plex Mono");
        font.setBold(bold);
        font.setPointSizeF(bold ? 12 : 11);
        painter.setFont(font);
        painter.setPen(color);
        QFontMetricsF fm(font);
        QRectF textRect(pos.x() - fm.horizontalAdvance(text) / 2.0, pos.y() - fm.height() / 2.0,
                         fm.horizontalAdvance(text), fm.height());
        painter.drawText(textRect, Qt::AlignCenter, text);
    };

    drawDirLabel("N", 0, QColor("#e2685a"), true);
    drawDirLabel("S", 180, QColor("#8fa3b0"), false);
    drawDirLabel("W", 270, QColor("#8fa3b0"), false);
    drawDirLabel("E", 90, QColor("#8fa3b0"), false);

    painter.save();
    painter.translate(center);
    painter.rotate(heading_);
    painter.translate(0, -(tickOuterR + 2));
    QPolygonF arrow;
    arrow << QPointF(-5, 8) << QPointF(5, 8) << QPointF(0, 0);
    painter.setBrush(QColor("#e2685a"));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(arrow);
    painter.restore();

    QFont headingFont = painter.font();
    headingFont.setFamily("IBM Plex Mono");
    headingFont.setBold(true);
    headingFont.setPointSizeF(16);
    painter.setFont(headingFont);
    painter.setPen(QColor("#e7edf2"));
    const QString headingText = QString::number(int(std::round(heading_)) % 360) + QChar(0xB0);
    painter.drawText(dialRect, Qt::AlignCenter, headingText);
}
