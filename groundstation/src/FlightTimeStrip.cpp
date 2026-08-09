#include "FlightTimeStrip.hpp"

FlightTimeStrip::FlightTimeStrip(QWidget *parent) : QWidget(parent) {
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(22, 10, 22, 10);
    layout_->setSpacing(2);

    captionLabel_ = new QLabel("FLIGHT TIME", this);
    captionLabel_->setAlignment(Qt::AlignCenter);
    captionLabel_->setStyleSheet("color: #5f707c; font-size: 9.5px; letter-spacing: 1px;");

    timeValue_ = new QLabel("00:00:00", this);
    timeValue_->setAlignment(Qt::AlignCenter);
    timeValue_->setStyleSheet("color: #e7edf2; font-size: 20px; font-weight: 600;");

    layout_->addWidget(captionLabel_);
    layout_->addWidget(timeValue_);

    setStyleSheet("background: rgba(10,14,17,0.85); border: 1px solid #1c242a; border-radius: 10px;");
}

void FlightTimeStrip::setTime(const QString &hhmmss) {
    timeValue_->setText(hhmmss);
}
