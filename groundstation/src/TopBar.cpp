#include "TopBar.hpp"

TopBar::TopBar(QWidget* parent): QWidget(parent){
    linkLabel_ = new QLabel("92% LINK", this);
    satLabel_ = new QLabel("SAT 14", this);
    tempLabel_ = new QLabel("58°C", this);
    batteryLabel_ = new QLabel("87%", this);
    titleLabel_ = new QLabel("Jetson UAV · Ground Station", this);
    vehicleLabel_ = new QLabel("VEHICLE 1", this);
    armButton_ = new QPushButton("DISARMED", this);
    layout_ = new QHBoxLayout(this);

    layout_->addWidget(titleLabel_);
    layout_->addWidget(vehicleLabel_);
    

    layout_->addStretch();
    for(const QString& mode: {"MANUAL","GUIDED","AUTO","LOITER","RTL"}){
        QPushButton *modeButton = new QPushButton(mode,this);
        connect(modeButton, &QPushButton::clicked, this, [this,mode]{
            emit modeSelected(mode);
        });
        layout_->addWidget(modeButton);
    }
    layout_->addStretch();

    layout_->addWidget(linkLabel_);
    layout_->addWidget(satLabel_);
    layout_->addWidget(tempLabel_);
    layout_->addWidget(batteryLabel_);
    layout_->addWidget(armButton_);

    connect(armButton_, &QPushButton::clicked, this, &TopBar::armToggled);
}