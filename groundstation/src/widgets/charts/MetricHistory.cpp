#include "MetricHistory.hpp"

MetricHistory::MetricHistory(double windowSeconds) : windowSeconds_(windowSeconds) {
    clock_.start();
}

double MetricHistory::elapsedSeconds() const {
    return clock_.elapsed() / 1000.0;
}

void MetricHistory::push(double value) {
    const double now = elapsedSeconds();
    samples_.append(QPointF(now, value));

    const double cutoff = now - windowSeconds_;
    int firstKept = 0;
    while (firstKept < samples_.size() && samples_.at(firstKept).x() < cutoff) {
        firstKept++;
    }
    if (firstKept > 0) {
        samples_.remove(0, firstKept);
    }
}
