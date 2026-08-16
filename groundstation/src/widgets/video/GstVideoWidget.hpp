#ifndef GST_VIDEO_WIDGET_HPP
#define GST_VIDEO_WIDGET_HPP

#include <QWidget>
#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string>

#include "TelemetryClient.hpp"


class GstVideoWidget: public QWidget{
        Q_OBJECT
    public:
        GstVideoWidget(QWidget* parent = nullptr);
        ~GstVideoWidget();

        void start(const std::string &host, int port);
        void setTracks(const QVector<Track> &tracks);
        void setEnlarged(bool enlarged);

        signals:
            void clicked();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;

    private:
        GstElement* pipeline_ = nullptr;
        QImage image_;
        QMutex frame_mutex_;
        QVector<Track> tracks_;
        bool enlarged_ = false;

        static GstFlowReturn onNewSample(GstElement* sink, gpointer user_data);
        void handleNewFrame(const QImage& frame);
};

#endif // GST_VIDEO_WIDGET_HPP