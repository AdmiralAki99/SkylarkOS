#include "TelemetryPanel.hpp"

TelemetryPanel::TelemetryPanel(QWidget *parent): QWidget(parent){
    setObjectName("telemetryPanel");
    setAttribute(Qt::WA_StyledBackground, true);

    layout_ = new QGridLayout(this);
    layout_->setContentsMargins(14, 14, 14, 14);

    QLabel *headerLabel = new QLabel("TELEMETRY", this);
    headerLabel->setObjectName("panelHeader");
    layout_->addWidget(headerLabel, 0, 0, 1, 2);

    altitudeValue_ = addField("ALTITUDE", 1, 0);
    groundSpeedValue_ = addField("GND SPEED", 1, 1);
    headingValue_ = addField("HEADING", 2, 0);
    flightTimeValue_ = addField("FLIGHT TIME", 2, 1);
    pitchValue_ = addField("PITCH", 3, 0);
    rollValue_ = addField("ROLL", 3, 1);

    QFrame *divider = new QFrame(this);
    divider->setObjectName("telemetryDivider");
    divider->setFrameShape(QFrame::HLine);
    layout_->addWidget(divider, 4, 0, 1, 2);

    QWidget *jetsonRow = new QWidget(this);
    QHBoxLayout *jetsonRowLayout = new QHBoxLayout(jetsonRow);
    jetsonRowLayout->setContentsMargins(0,0,0,0);
    QLabel *jetsonCaption = new QLabel("JETSON", jetsonRow);
    jetsonCaption->setObjectName("jetsonCaption");
    jetsonStatsValue_ = new QLabel("--", jetsonRow);
    jetsonStatsValue_->setObjectName("jetsonValue");
    jetsonRowLayout->addWidget(jetsonCaption);
    jetsonRowLayout->addStretch();
    jetsonRowLayout->addWidget(jetsonStatsValue_);
    layout_->addWidget(jetsonRow, 5, 0, 1, 2);
}

void TelemetryPanel::setAltitude(double meters){
    altitudeValue_->setText(QString::number(meters, 'f', 1) + " m");
}

void TelemetryPanel::setGroundSpeed(double mps){
    groundSpeedValue_->setText(QString::number(mps, 'f', 1) + " m/s");
}

void TelemetryPanel::setHeading(double degrees){
    headingValue_->setText(QString::number(degrees, 'f', 1) + "°");
}

void TelemetryPanel::setFlightTime(const QString &hhmmss){
    flightTimeValue_->setText(hhmmss);
}

void TelemetryPanel::setPitch(double degrees){
    pitchValue_->setText(QString::number(degrees, 'f', 1) + "°");
}

void TelemetryPanel::setRoll(double degrees){
    rollValue_->setText(QString::number(degrees, 'f', 1) + "°");
}

void TelemetryPanel::setJetsonStats(double tempC, int gpuLoadPct){
    jetsonStatsValue_->setText(QString::number(tempC, 'f', 0) + "°C · GPU " + QString::number(gpuLoadPct) + "%");
}

QLabel* TelemetryPanel::addField(const QString &caption, int row, int col){
    QWidget *cell = new QWidget(this);
    QVBoxLayout *cellLayout = new QVBoxLayout(cell);
    cellLayout->setContentsMargins(0,0,0,0);
    QLabel *captionLabel = new QLabel(caption, cell);
    captionLabel->setObjectName("fieldCaption");
    QLabel *valueLabel = new QLabel("--", cell);
    valueLabel->setObjectName("fieldValue");
    cellLayout->addWidget(captionLabel);
    cellLayout->addWidget(valueLabel);
    layout_->addWidget(cell, row, col);
    return valueLabel;
}
