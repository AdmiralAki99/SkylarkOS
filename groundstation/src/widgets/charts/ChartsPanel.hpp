#ifndef CHARTS_PANEL_HPP
#define CHARTS_PANEL_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>

#include "MetricHistory.hpp"

QT_BEGIN_NAMESPACE
class QChart;
class QChartView;
class QLineSeries;
class QValueAxis;
QT_END_NAMESPACE

class ChartsPanel : public QWidget {
    Q_OBJECT
public:
    explicit ChartsPanel(QWidget *parent = nullptr);

    void pushBattery(double percent);
    void pushGpu(double percent);
    void pushThermalCore(int coreIndex, double tempC);

private:
    static constexpr int kCoreCount = 4;
    static constexpr double kWindowSeconds = 60.0;

    QVBoxLayout *layout_ = nullptr;

    MetricHistory batteryHistory_{kWindowSeconds};
    QChart *batteryChart_ = nullptr;
    QLineSeries *batterySeries_ = nullptr;
    QValueAxis *batteryAxisX_ = nullptr;

    MetricHistory gpuHistory_{kWindowSeconds};
    QChart *gpuChart_ = nullptr;
    QLineSeries *gpuSeries_ = nullptr;
    QValueAxis *gpuAxisX_ = nullptr;

    MetricHistory thermalHistory_[kCoreCount] = {
        MetricHistory(kWindowSeconds), MetricHistory(kWindowSeconds),
        MetricHistory(kWindowSeconds), MetricHistory(kWindowSeconds)
    };
    QChart *thermalChart_ = nullptr;
    QLineSeries *thermalSeries_[kCoreCount] = {nullptr, nullptr, nullptr, nullptr};
    QValueAxis *thermalAxisX_ = nullptr;

    QChartView* buildChart(const QString &title, const QColor &color,
                            double yMin, double yMax, const QString &yFormat,
                            QChart **outChart, QLineSeries **outSeries, QValueAxis **outAxisX);
    void scrollAxis(QValueAxis *axisX, double elapsedSeconds);
};

#endif // CHARTS_PANEL_HPP
