#include "GstVideoWidget.hpp"

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
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath clipPath;
    clipPath.addEllipse(rect());
    painter.setClipPath(clipPath);

    QMutexLocker Locker(&frame_mutex_);
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

void GstVideoWidget::mousePressEvent(QMouseEvent* event){
    emit clicked();
}

void GstVideoWidget::setTracks(const QVector<Track> &tracks){
    QMutexLocker Locker(&frame_mutex_);
    tracks_ = tracks;
    update();
}