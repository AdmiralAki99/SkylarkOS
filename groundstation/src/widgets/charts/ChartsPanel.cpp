#include "ChartsPanel.hpp"

#include <QPainter>
#include <QLinearGradient>
#include <QFont>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QValueAxis>

namespace {

void styleChart(QChart *chart, const QString &title) {
    chart->setTitle(title);
    chart->setTitleFont(QFont("IBM Plex Mono", 8, QFont::DemiBold));
    chart->setTitleBrush(QBrush(QColor("#8fa3b0")));
    chart->legend()->hide();
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(4, 2, 4, 2));
}

void styleAxisX(QValueAxis *axisX) {
    axisX->setLabelsVisible(false);
    axisX->setGridLineVisible(false);
    axisX->setLineVisible(false);
}

void styleAxisY(QValueAxis *axisY) {
    QFont axisFont("IBM Plex Mono", 7);
    axisY->setLabelsFont(axisFont);
    axisY->setLabelsColor(QColor("#5f707c"));
    axisY->setGridLineColor(QColor("#1c242a"));
    axisY->setLineVisible(false);
    axisY->setTickCount(3);
}

QAreaSeries* addGlowFill(QChart *chart, QLineSeries *line, const QColor &color) {
    QAreaSeries *area = new QAreaSeries(line);
    QLinearGradient gradient(0, 0, 0, 1);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    QColor top = color; top.setAlpha(90);
    QColor bottom = color; bottom.setAlpha(0);
    gradient.setColorAt(0.0, top);
    gradient.setColorAt(1.0, bottom);
    area->setBrush(gradient);

    QPen linePen(color);
    linePen.setWidthF(2.0);
    area->setPen(linePen);

    chart->addSeries(area);

    return area;
}

}

ChartsPanel::ChartsPanel(QWidget *parent) : QWidget(parent) {
    setObjectName("chartsPanel");
    setAttribute(Qt::WA_StyledBackground, true);

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(14, 14, 14, 14);
    layout_->setSpacing(10);

    QLabel *headerLabel = new QLabel("TELEMETRY CHARTS", this);
    headerLabel->setObjectName("panelHeader");
    layout_->addWidget(headerLabel);

    QChartView *batteryView = buildChart("BATTERY", QColor("#46c88c"), 0, 100, "%",
                                          &batteryChart_, &batterySeries_, &batteryAxisX_);
    layout_->addWidget(batteryView);

    QChartView *gpuView = buildChart("GPU LOAD", QColor("#5aa9ff"), 0, 100, "%",
                                      &gpuChart_, &gpuSeries_, &gpuAxisX_);
    layout_->addWidget(gpuView);

    thermalChart_ = new QChart();
    styleChart(thermalChart_, "CORE TEMPS");

    const QColor coreColors[kCoreCount] = {
        QColor("#e2685a"), QColor("#ffb020"), QColor("#5aa9ff"), QColor("#46c88c")
    };
    for (int i = 0; i < kCoreCount; ++i) {
        thermalSeries_[i] = new QLineSeries();
        thermalSeries_[i]->setColor(coreColors[i]);
        thermalChart_->addSeries(thermalSeries_[i]);
    }

    thermalAxisX_ = new QValueAxis();
    thermalAxisX_->setRange(0, kWindowSeconds);
    styleAxisX(thermalAxisX_);
    thermalChart_->addAxis(thermalAxisX_, Qt::AlignBottom);

    QValueAxis *thermalAxisY = new QValueAxis();
    thermalAxisY->setRange(30, 90);
    thermalAxisY->setLabelFormat("%d°C");
    styleAxisY(thermalAxisY);
    thermalChart_->addAxis(thermalAxisY, Qt::AlignLeft);

    for (int i = 0; i < kCoreCount; ++i) {
        thermalSeries_[i]->attachAxis(thermalAxisX_);
        thermalSeries_[i]->attachAxis(thermalAxisY);
        QPen linePen(coreColors[i]);
        linePen.setWidthF(1.5);
        thermalSeries_[i]->setPen(linePen);
    }

    QChartView *thermalView = new QChartView(thermalChart_, this);
    thermalView->setObjectName("chartCard");
    thermalView->setAttribute(Qt::WA_StyledBackground, true);
    thermalView->setRenderHint(QPainter::Antialiasing);
    thermalView->setFixedHeight(190);
    layout_->addWidget(thermalView);
}

QChartView* ChartsPanel::buildChart(const QString &title, const QColor &color,
                                     double yMin, double yMax, const QString &yFormat,
                                     QChart **outChart, QLineSeries **outSeries, QValueAxis **outAxisX) {
    QChart *chart = new QChart();
    styleChart(chart, title);

    QLineSeries *series = new QLineSeries();
    QAreaSeries *area = addGlowFill(chart, series, color);

    QValueAxis *axisX = new QValueAxis();
    axisX->setRange(0, kWindowSeconds);
    styleAxisX(axisX);
    chart->addAxis(axisX, Qt::AlignBottom);
    area->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(yMin, yMax);
    axisY->setLabelFormat("%d" + yFormat);
    styleAxisY(axisY);
    chart->addAxis(axisY, Qt::AlignLeft);
    area->attachAxis(axisY);

    *outChart = chart;
    *outSeries = series;
    *outAxisX = axisX;

    QChartView *view = new QChartView(chart, this);
    view->setObjectName("chartCard");
    view->setAttribute(Qt::WA_StyledBackground, true);
    view->setRenderHint(QPainter::Antialiasing);
    view->setFixedHeight(170);
    return view;
}

void ChartsPanel::scrollAxis(QValueAxis *axisX, double elapsedSeconds) {
    if (elapsedSeconds > kWindowSeconds) {
        axisX->setRange(elapsedSeconds - kWindowSeconds, elapsedSeconds);
    }
}

void ChartsPanel::pushBattery(double percent) {
    batteryHistory_.push(percent);
    batterySeries_->replace(batteryHistory_.samples());
    scrollAxis(batteryAxisX_, batteryHistory_.elapsedSeconds());
}

void ChartsPanel::pushGpu(double percent) {
    gpuHistory_.push(percent);
    gpuSeries_->replace(gpuHistory_.samples());
    scrollAxis(gpuAxisX_, gpuHistory_.elapsedSeconds());
}

void ChartsPanel::pushThermalCore(int coreIndex, double tempC) {
    if (coreIndex < 0 || coreIndex >= kCoreCount) return;
    thermalHistory_[coreIndex].push(tempC);
    thermalSeries_[coreIndex]->replace(thermalHistory_[coreIndex].samples());
    scrollAxis(thermalAxisX_, thermalHistory_[coreIndex].elapsedSeconds());
}
