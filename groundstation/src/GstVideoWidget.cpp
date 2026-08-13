#include "GstVideoWidget.hpp"
#include <QFontMetricsF>

GstVideoWidget::GstVideoWidget(QWidget* parent): QWidget(parent) {
    
}

GstVideoWidget::~GstVideoWidget(){
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
    }
}

void GstVideoWidget::start(const std::string &host, int port){
    gst_init(nullptr, nullptr);

    const std::string pipeline_description = "udpsrc port=" + std::to_string(port) + " ! application/x-rtp,media=video,encoding-name=H264,payload=96 ! "
                                            "rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=RGB ! "
                                            "appsink name=video_sink emit-signals=true max-buffers=1 drop=true";

    GError* error = nullptr;
    pipeline_ = gst_parse_launch(pipeline_description.c_str(), &error);
    if (!pipeline_) {
        qCritical("Failed to create GStreamer pipeline: %s", error ? error->message : "unknown");
        if (error) g_error_free(error);
        return;
    }

    GstElement* video_sink = gst_bin_get_by_name(GST_BIN(pipeline_), "video_sink");
    g_signal_connect(video_sink, "new-sample", G_CALLBACK(onNewSample), this);
    gst_object_unref(video_sink);
    gst_element_set_state(pipeline_, GST_STATE_PLAYING);
}

GstFlowReturn GstVideoWidget::onNewSample(GstElement* sink, gpointer user_data){
    GstVideoWidget* widget = static_cast<GstVideoWidget*>(user_data);
    
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);

    std::vector<uint8_t> raw_data((uint8_t*)map.data, (uint8_t*)map.data + map.size);
    gst_buffer_unmap(buffer, &map);
    QImage frame = QImage(raw_data.data(), 480, 480, QImage::Format_RGB888).copy();

    QMetaObject::invokeMethod(widget, [widget, frame]{
            widget->handleNewFrame(frame);
        }, Qt::QueuedConnection
    );

    gst_sample_unref(sample);
    return GST_FLOW_OK;

}

void GstVideoWidget::handleNewFrame(const QImage& frame){
    QMutexLocker Locker(&frame_mutex_);
    image_ = frame.copy();
    update();
}

void GstVideoWidget::paintEvent(QPaintEvent* event){
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath clipPath;
    if (enlarged_) {
        clipPath.addRoundedRect(rect(), 16, 16);
    } else {
        clipPath.addEllipse(rect());
    }
    painter.setClipPath(clipPath);

    {
        QMutexLocker locker(&frame_mutex_);
        if(!image_.isNull()){
            painter.drawImage(rect(), image_);
            painter.setPen(QPen(Qt::green, 2));
            for(const Track &track : tracks_){
                QRectF box(
                    rect().left() + track.x1 * rect().width(),
                    rect().top() + track.y1 * rect().height(),
                    (track.x2 - track.x1) * rect().width(),
                    (track.y2 - track.y1) * rect().height()
                );
                painter.drawRect(box);
            }
        }
    }

    painter.setClipping(false);
    QPen borderPen(QColor(enlarged_ ? "#2a333b" : "#1c242a"), 3);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    if (enlarged_) {
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 16, 16);
    } else {
        painter.drawEllipse(rect().adjusted(1, 1, -1, -1));
    }

    QFont liveFont = painter.font();
    liveFont.setFamily("IBM Plex Mono");
    liveFont.setPointSizeF(8);
    painter.setFont(liveFont);
    QFontMetricsF liveMetrics(liveFont);
    const QString liveText = "LIVE";
    const double liveTextWidth = liveMetrics.horizontalAdvance(liveText);
    const double liveBadgeWidth = liveTextWidth + 22;
    const double liveBadgeHeight = 18;
    QRectF liveBadgeRect((width() - liveBadgeWidth) / 2.0, 14, liveBadgeWidth, liveBadgeHeight);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(6, 10, 13, 179));
    painter.drawRoundedRect(liveBadgeRect, liveBadgeHeight / 2.0, liveBadgeHeight / 2.0);
    painter.setBrush(QColor("#e2685a"));
    painter.drawEllipse(QPointF(liveBadgeRect.left() + 12, liveBadgeRect.center().y()), 3, 3);
    painter.setPen(QColor("#e2685a"));
    painter.drawText(
        QRectF(liveBadgeRect.left() + 18, liveBadgeRect.top(), liveTextWidth, liveBadgeHeight),
        Qt::AlignVCenter | Qt::AlignLeft, liveText
    );

    QFont hintFont = painter.font();
    hintFont.setFamily("IBM Plex Mono");
    hintFont.setPointSizeF(7.5);
    painter.setFont(hintFont);
    const QString hintText = enlarged_ ? "CLICK TO SHRINK" : "CLICK TO EXPAND";
    QFontMetricsF hintMetrics(hintFont);
    const double hintTextWidth = hintMetrics.horizontalAdvance(hintText);
    const double hintPadWidth = hintTextWidth + 16;
    const double hintPadHeight = 18;
    const double hintBottom = enlarged_ ? 14 : 38;
    QRectF hintRect((width() - hintPadWidth) / 2.0, height() - hintBottom - hintPadHeight, hintPadWidth, hintPadHeight);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(6, 10, 13, 179));
    painter.drawRoundedRect(hintRect, 6, 6);
    painter.setPen(QColor("#8fa3b0"));
    painter.drawText(hintRect, Qt::AlignCenter, hintText);
}

void GstVideoWidget::mousePressEvent(QMouseEvent* event){
    Q_UNUSED(event);
    emit clicked();
}

void GstVideoWidget::setTracks(const QVector<Track> &tracks){
    QMutexLocker Locker(&frame_mutex_);
    tracks_ = tracks;
    update();
}

void GstVideoWidget::setEnlarged(bool enlarged){
    enlarged_ = enlarged;
    update();
}