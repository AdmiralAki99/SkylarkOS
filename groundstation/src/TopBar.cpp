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

void TopBar::setLinkQuality(int quality){
    linkLabel_->setText(QString("%1% LINK").arg(quality));
}

void TopBar::setSatellites(int satellites){
    satLabel_->setText(QString("SAT %1").arg(satellites));
}

void TopBar::setJetsonTemp(int tempC){
    tempLabel_->setText(QString("%1°C").arg(tempC));
}

void TopBar::setBattery(double percent){
    batteryLabel_->setText(QString("%1%").arg(percent, 0, 'f', 0));
}

void TopBar::setArmed(bool armed){
    armButton_->setText(armed ? "ARMED" : "DISARMED");
    armButton_->setStyleSheet(armed
        ? "background: #1c3a2c; color: #46c88c;"
        : "background: #2a1c1c; color: #e2685a;");
}

void TopBar::setConnected(bool connected){
    linkLabel_->setText(connected ? "LINK CONNECTED" : "LINK LOST");
    linkLabel_->setStyleSheet(connected ? "color: #46c88c;" : "color: #e2685a;");
}