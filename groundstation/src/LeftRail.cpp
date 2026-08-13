#include "LeftRail.hpp"

#include <QPixmap>
#include <QPainter>
#include <QIcon>

QToolButton* LeftRail::makeRailButton(const QString &glyph, const QString &label, QWidget *parent) {
    const int size = 20;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QFont font = painter.font();
    font.setPointSizeF(13);
    painter.setFont(font);
    painter.setPen(QColor("#8fa3b0"));
    painter.drawText(pixmap.rect(), Qt::AlignCenter, glyph);

    QToolButton *button = new QToolButton(parent);
    button->setObjectName("railButton");
    button->setIcon(QIcon(pixmap));
    button->setIconSize(QSize(size, size));
    button->setText(label);
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    return button;
}

LeftRail::LeftRail(QWidget* parent): QWidget(parent){
    setObjectName("leftRail");
    setAttribute(Qt::WA_StyledBackground, true);

    takeoffButton_ = makeRailButton(QChar(0x25B2), "TAKEOFF", this);
    returnButton_ = makeRailButton(QChar(0x27F2), "RETURN", this);
    pauseButton_ = makeRailButton(QChar(0x23F8), "PAUSE", this);
    armButton_ = makeRailButton(QChar(0x26A1), "ARM", this);
    layout_ = new QVBoxLayout(this);

    layout_->addWidget(takeoffButton_);
    layout_->addWidget(returnButton_);
    layout_->addWidget(pauseButton_);
    layout_->addWidget(armButton_);

    connect(takeoffButton_, &QToolButton::clicked, this, &LeftRail::takeoffClicked);
    connect(returnButton_, &QToolButton::clicked, this, &LeftRail::returnClicked);
    connect(pauseButton_, &QToolButton::clicked, this, &LeftRail::pauseClicked);
    connect(armButton_, &QToolButton::clicked, this, &LeftRail::armClicked);
}
