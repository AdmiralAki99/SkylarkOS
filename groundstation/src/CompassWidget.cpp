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

    // Dial rotates opposite heading so "N" stays pointing at true north;
    // labels are positioned via trig at the rotated angle but drawn
    // unrotated so the letters stay upright.
    const double radius = side / 2.0 - 18;
    const QPointF center = dialRect.center();
    auto drawDirLabel = [&](const QString &text, double angleDeg, const QColor &color, bool bold) {
        const double effectiveAngle = angleDeg - heading_;
        const double rad = qDegreesToRadians(effectiveAngle - 90.0);
        const QPointF pos = center + QPointF(std::cos(rad) * radius, std::sin(rad) * radius);

        QFont font = painter.font();
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
    painter.translate(dialRect.center().x(), dialRect.top() + 8);
    QPolygonF arrow;
    arrow << QPointF(-5, 8) << QPointF(5, 8) << QPointF(0, 0);
    painter.setBrush(QColor("#e2685a"));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(arrow);
    painter.restore();

    QFont headingFont = painter.font();
    headingFont.setBold(true);
    headingFont.setPointSizeF(16);
    painter.setFont(headingFont);
    painter.setPen(QColor("#e7edf2"));
    const QString headingText = QString::number(int(std::round(heading_)) % 360) + QChar(0xB0);
    painter.drawText(dialRect, Qt::AlignCenter, headingText);
}
