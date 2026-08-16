#ifndef METRIC_HISTORY_HPP
#define METRIC_HISTORY_HPP

#include <QVector>
#include <QPointF>
#include <QElapsedTimer>

class MetricHistory {
public:
    explicit MetricHistory(double windowSeconds = 60.0);

    void push(double value);
    const QVector<QPointF>& samples() const { return samples_; }
    double windowSeconds() const { return windowSeconds_; }
    double elapsedSeconds() const;

private:
    QVector<QPointF> samples_;
    QElapsedTimer clock_;
    double windowSeconds_;
};

#endif // METRIC_HISTORY_HPP
