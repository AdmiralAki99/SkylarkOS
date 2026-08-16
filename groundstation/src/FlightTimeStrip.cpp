#include "FlightTimeStrip.hpp"

FlightTimeStrip::FlightTimeStrip(QWidget *parent) : QWidget(parent) {
    setObjectName("flightTimeStrip");
    setAttribute(Qt::WA_StyledBackground, true);

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(22, 10, 22, 10);
    layout_->setSpacing(2);

    captionLabel_ = new QLabel("FLIGHT TIME", this);
    captionLabel_->setObjectName("flightTimeCaption");
    captionLabel_->setAlignment(Qt::AlignCenter);

    timeValue_ = new QLabel("00:00:00", this);
    timeValue_->setObjectName("flightTimeValue");
    timeValue_->setAlignment(Qt::AlignCenter);

    layout_->addWidget(captionLabel_);
    layout_->addWidget(timeValue_);
}

void FlightTimeStrip::setTime(const QString &hhmmss) {
    timeValue_->setText(hhmmss);
}
