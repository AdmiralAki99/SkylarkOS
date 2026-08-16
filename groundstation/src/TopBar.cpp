#include "TopBar.hpp"
#include <QStyle>
#include <QMouseEvent>
#include <QWindow>

TopBar::TopBar(QWidget* parent): QWidget(parent){
    setObjectName("topBar");
    setAttribute(Qt::WA_StyledBackground, true);

    linkLabel_ = new QLabel("92% LINK", this);
    linkLabel_->setObjectName("monoReadout");
    satLabel_ = new QLabel("SAT 14", this);
    satLabel_->setObjectName("monoReadout");
    tempLabel_ = new QLabel("58°C", this);
    tempLabel_->setObjectName("monoReadout");
    batteryLabel_ = new QLabel("87%", this);
    batteryLabel_->setObjectName("batteryReadout");
    titleLabel_ = new QLabel("Jetson UAV · Ground Station", this);
    titleLabel_->setObjectName("appTitle");
    vehicleLabel_ = new QLabel("VEHICLE 1", this);
    vehicleLabel_->setObjectName("monoReadout");
    armButton_ = new QPushButton("DISARMED", this);
    armButton_->setObjectName("armButton");
    armButton_->setProperty("armed", false);
    layout_ = new QHBoxLayout(this);

    layout_->addWidget(titleLabel_);
    layout_->addWidget(vehicleLabel_);

    layout_->addStretch();
    for(const QString& mode: {"MANUAL","GUIDED","AUTO","LOITER","RTL"}){
        QPushButton *modeButton = new QPushButton(mode,this);
        modeButton->setObjectName("modeButton");
        connect(modeButton, &QPushButton::clicked, this, [this,mode]{
            emit modeSelected(mode);
        });
        layout_->addWidget(modeButton);
    }

    chartsButton_ = new QPushButton("CHARTS", this);
    chartsButton_->setObjectName("modeButton");
    chartsButton_->setToolTip("Toggle telemetry charts");
    connect(chartsButton_, &QPushButton::clicked, this, &TopBar::chartsToggled);
    layout_->addWidget(chartsButton_);

    layout_->addStretch();

    layout_->addWidget(linkLabel_);
    layout_->addWidget(satLabel_);
    layout_->addWidget(tempLabel_);
    layout_->addWidget(batteryLabel_);
    layout_->addWidget(armButton_);

    connect(armButton_, &QPushButton::clicked, this, &TopBar::armToggled);

    minimizeButton_ = new QPushButton(QChar(0x2212), this);
    minimizeButton_->setObjectName("winControlButton");
    connect(minimizeButton_, &QPushButton::clicked, this, [this]{ window()->showMinimized(); });
    layout_->addWidget(minimizeButton_);

    maximizeButton_ = new QPushButton(QChar(0x25A1), this);
    maximizeButton_->setObjectName("winControlButton");
    connect(maximizeButton_, &QPushButton::clicked, this, &TopBar::toggleMaximized);
    layout_->addWidget(maximizeButton_);

    closeButton_ = new QPushButton(QChar(0x00D7), this);
    closeButton_->setObjectName("winCloseButton");
    connect(closeButton_, &QPushButton::clicked, this, [this]{ window()->close(); });
    layout_->addWidget(closeButton_);
}

void TopBar::toggleMaximized(){
    if (window()->isMaximized()) {
        window()->showNormal();
    } else {
        window()->showMaximized();
    }
}

void TopBar::mousePressEvent(QMouseEvent *event){
    if (event->button() == Qt::LeftButton && window()->windowHandle()) {
        window()->windowHandle()->startSystemMove();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void TopBar::mouseDoubleClickEvent(QMouseEvent *event){
    if (event->button() == Qt::LeftButton) {
        toggleMaximized();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
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
    armButton_->setProperty("armed", armed);
    armButton_->style()->unpolish(armButton_);
    armButton_->style()->polish(armButton_);
}

void TopBar::setConnected(bool connected){
    linkLabel_->setText(connected ? "LINK CONNECTED" : "LINK LOST");
    linkLabel_->setProperty("connected", connected);
    linkLabel_->style()->unpolish(linkLabel_);
    linkLabel_->style()->polish(linkLabel_);
}
