#include "NudgePad.hpp"

QToolButton* NudgePad::makeNudgeButton(const QString &glyph, QWidget *parent) {
    QToolButton *button = new QToolButton(parent);
    button->setObjectName("nudgeButton");
    button->setText(glyph);
    return button;
}

NudgePad::NudgePad(QWidget* parent): QWidget(parent){
    setObjectName("nudgePad");
    setAttribute(Qt::WA_StyledBackground, true);

    forwardButton_ = makeNudgeButton(QChar(0x25B2), this);
    backwardButton_ = makeNudgeButton(QChar(0x25BC), this);
    leftButton_ = makeNudgeButton(QChar(0x25C0), this);
    rightButton_ = makeNudgeButton(QChar(0x25B6), this);
    stopButton_ = makeNudgeButton(QChar(0x25A0), this);
    stopButton_->setObjectName("nudgeStopButton");

    layout_ = new QGridLayout(this);
    layout_->setSpacing(6);
    layout_->addWidget(forwardButton_, 0, 1);
    layout_->addWidget(leftButton_, 1, 0);
    layout_->addWidget(stopButton_, 1, 1);
    layout_->addWidget(rightButton_, 1, 2);
    layout_->addWidget(backwardButton_, 2, 1);

    connect(forwardButton_, &QToolButton::clicked, this, &NudgePad::forwardClicked);
    connect(backwardButton_, &QToolButton::clicked, this, &NudgePad::backwardClicked);
    connect(leftButton_, &QToolButton::clicked, this, &NudgePad::leftClicked);
    connect(rightButton_, &QToolButton::clicked, this, &NudgePad::rightClicked);
    connect(stopButton_, &QToolButton::clicked, this, &NudgePad::stopClicked);
}
