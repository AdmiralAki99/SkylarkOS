#include "LeftRail.hpp"

LeftRail::LeftRail(QWidget* parent): QWidget(parent){
    takeoffButton_ = new QPushButton("TAKEOFF", this);
    returnButton_ = new QPushButton("RETURN", this);
    pauseButton_ = new QPushButton("PAUSE", this);
    armButton_ = new QPushButton("ARM", this);
    layout_ = new QVBoxLayout(this);

    layout_->addWidget(takeoffButton_);
    layout_->addWidget(returnButton_);
    layout_->addWidget(pauseButton_);
    layout_->addWidget(armButton_);

    connect(takeoffButton_, &QPushButton::clicked, this, &LeftRail::takeoffClicked);
    connect(returnButton_, &QPushButton::clicked, this, &LeftRail::returnClicked);
    connect(pauseButton_, &QPushButton::clicked, this, &LeftRail::pauseClicked);
    connect(armButton_, &QPushButton::clicked, this, &LeftRail::armClicked);
}

