#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/header.hpp>
#include <sensor_msgs/msg/image.hpp>

#include <thread>

class CameraNode : public rclcpp::Node
{
    public:
    CameraNode() : Node("camera_node"){

        // Declaring parameters
        declare_parameter("sensor_id", 0);
        declare_parameter("width", 640);
        declare_parameter("height", 480);
        declare_parameter("fps", 60);
        declare_parameter("udp_host_gs", std::string("10.0.0.2"));
        declare_parameter("udp_port_gs", 5600);
        declare_parameter("udp_host_watch", std::string("10.0.0.3"));
        declare_parameter("udp_port_watch", 5601);

        // Getting parameters
        sensor_id_ = get_parameter("sensor_id").as_int();
        width_ = get_parameter("width").as_int();
        height_ = get_parameter("height").as_int();
        fps_ = get_parameter("fps").as_int();

        udp_host_gs_ = get_parameter("udp_host_gs").as_string();
        udp_port_gs_ = get_parameter("udp_port_gs").as_int();
        udp_host_watch_ = get_parameter("udp_host_watch").as_string();
        udp_port_watch_ = get_parameter("udp_port_watch").as_int();

        auto qos = rclcpp::QoS(1).best_effort();

        // Creating the publishers for the raw and compressed images
        image_pub_ = this->create_publisher<sensor_msgs::msg::Image>("/camera/image_raw", qos);

        // Initializing the Gstreamer pipeline
        gst_init(nullptr, nullptr);
        std::string pipeline_description =  "nvarguscamerasrc sensor-id=" + std::to_string(sensor_id_) + " ! "
                                            "video/x-raw(memory:NVMM),width=" + std::to_string(width_) + ","
                                            "height=" + std::to_string(height_) + ",framerate=" + std::to_string(fps_) + "/1 ! "
                                            "tee name=t "
                                            "t. ! queue max-size-buffers=2 leaky=upstream max-size-time=0 max-size-bytes=0 ! "
                                            "nvvidconv ! video/x-raw,format=BGRx ! "
                                            "appsink name=raw_sink emit-signals=true max-buffers=1 drop=true "
                                            "t. ! queue max-size-buffers=2 leaky=upstream max-size-time=0 max-size-bytes=0 ! "
                                            "nvvidconv ! video/x-raw,format=NV12,width=480,height=480 ! x264enc tune=zerolatency speed-preset=ultrafast profile=baseline ! "
                                            "rtph264pay config-interval=1 pt=96 ! "
                                            "tee name=t2 "
                                            "t2. ! queue ! udpsink host=" + udp_host_gs_ + " port=" + std::to_string(udp_port_gs_) + " sync=false "
                                            "t2. ! queue ! udpsink host=" + udp_host_watch_ + " port=" + std::to_string(udp_port_watch_) + " sync=false";

                                            // "t. ! queue max-size-buffers=2 leaky=upstream max-size-time=0 max-size-bytes=0 ! "
                                            // "nvvidconv ! video/x-raw,format=I420 ! "
                                            // "videorate ! video/x-raw,framerate=25/1 ! "
                                            // "nvvidconv ! video/x-raw(memory:NVMM),width=640,height=360,format=NV12 ! "
                                            // "nvjpegenc quality=20 ! rtpjpegpay ! "
                                            // "udpsink host=" + udp_host_ + " port=" + std::to_string(udp_port_) + " sync=true buffer-size=8388608 max-bitrate=2000000";

        GError* error = nullptr;
        pipeline_ = gst_parse_launch(pipeline_description.c_str(), &error);
        if (!pipeline_) {
            RCLCPP_ERROR(get_logger(), "Failed to create pipeline: %s", error ? error->message : "unknown");
            if (error) g_error_free(error);
            return;
        }

        GstElement* raw_sink  = gst_bin_get_by_name(GST_BIN(pipeline_), "raw_sink");

        // Connecting the sinks to the pipeline
        g_signal_connect(raw_sink, "new-sample", G_CALLBACK(on_raw_sample), this);

        // Releasing the sink elements
        gst_object_unref(raw_sink);

        // Starting the pipeline
        gst_element_set_state(pipeline_, GST_STATE_PLAYING);

        // GstBus* bus = gst_element_get_bus(pipeline_);
        // gst_bus_add_watch(bus, on_bus_message, this);
        // gst_object_unref(bus);

        // gst_loop_ = g_main_loop_new(nullptr, FALSE);
        // gst_thread_ = std::thread([this]{ g_main_loop_run(gst_loop_); })
    }

    ~CameraNode(){
        if (pipeline_) {
            gst_element_set_state(pipeline_, GST_STATE_NULL);
            gst_object_unref(pipeline_);
        }
    }

    private:
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;
    GstElement* pipeline_ = nullptr;

    GMainLoop* gst_loop_;
    std::thread gst_thread_;

    int sensor_id_ = 0;
    int width_ = 640;
    int height_ = 480;
    int fps_ = 60;
    int udp_port_gs_;
    std::string udp_host_gs_;
    int udp_port_watch_;
    std::string udp_host_watch_;

    // void start_pipeline(){

    //     std::string pipeline_description =  "nvarguscamerasrc sensor-id=" + std::to_string(sensor_id_) + " ! "
    //                                         "video/x-raw(memory:NVMM),width=" + std::to_string(width_) + ","
    //                                         "height=" + std::to_string(height_) + ",framerate=" + std::to_string(fps_) + "/1 ! "
    //                                         "tee name=t "
    //                                         "t. ! queue max-size-buffers=2 leaky=downstream max-size-time=0 max-size-bytes=0 ! "
    //                                         "appsink name=raw_sink emit-signals=true max-buffers=1 drop=true "

    //                                         "t. ! queue max-size-buffers=2 leaky=downstream max-size-time=0 max-size-bytes=0 ! "
    //                                         "nvvidconv ! video/x-raw,format=I420 ! "
    //                                         "videorate ! video/x-raw,framerate=30/1 ! "
    //                                         "nvvidconv ! video/x-raw(memory:NVMM),width=640,height=360,format=NV12 ! "
    //                                         "nvjpegenc quality=20 ! rtpjpegpay ! "
    //                                         "udpsink host=" + udp_host_ + " port=" + std::to_string(udp_port_) + " sync=true buffer-size=8388608";

    //     GError* error = nullptr;
    //     pipeline_ = gst_parse_launch(pipeline_description.c_str(), &error);
    //     if (!pipeline_) {
    //         RCLCPP_ERROR(get_logger(), "Failed to start pipeline: %s", error ? error->message : "unknown");
    //         if (error) g_error_free(error);
    //         return;
    //     }

    //     GstElement* raw_sink = gst_bin_get_by_name(GST_BIN(pipeline_), "raw_sink");
    //     g_signal_connect(raw_sink, "new-sample", G_CALLBACK(on_raw_sample), this);
    //     gst_object_unref(raw_sink);

    //     GstBus* bus = gst_element_get_bus(pipeline_);
    //     gst_bus_add_watch(bus, on_bus_message, this);
    //     gst_object_unref(bus);

    //     gst_element_set_state(pipeline_, GST_STATE_PLAYING)

    // }

    // void restart_pipeline(){
    //     gst_element_set_state(pipeline_, GST_STATE_NULL);
    //     gst_element_get_state(pipeline_, nullptr, nullptr, GST_CLOCK_TIME_NONE);

    //     gst_object_unref(pipeline_);
    //     pipeline_ = nullptr;

    //     start_pipeline();
    // }

    // GStreamer callbacks

    static GstFlowReturn on_raw_sample(GstElement* sink, gpointer user_data){
        CameraNode* node = static_cast<CameraNode*>(user_data);
        // Publishing the raw image and logging the event
    

        GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
        GstBuffer* buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        gst_buffer_map(buffer, &map, GST_MAP_READ);
        
        std::vector<uint8_t> raw_data((uint8_t*)map.data, (uint8_t*)map.data + map.size);

        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);

        // Sending the raw image
        sensor_msgs::msg::Image message;
        message.header.stamp = node->now();
        message.data = std::move(raw_data);
        message.height = node->height_;
        message.width = node->width_;
        message.step = node->width_ * 4;
        message.encoding = "bgra8";
        node->image_pub_->publish(message);

        return GST_FLOW_OK;       
    }

    // static gboolean on_bus_message(GstBus*, GstMessage* msg, gpointer user_data){
    //     CameraNode* node = static_cast<CameraNode*>(user_data);

    //     switch (GST_MESSAGE_TYPE(msg)) {
    //         case GST_MESSAGE_ERROR: { 
    //             GError* err;
    //             gchar* debug_info;
    //             gst_message_parse_error(msg, &err, &debug_info);
    //             RCLCPP_ERROR(node->get_logger(), "Pipeline error: %s", err->message);
    //             g_error_free(err);
    //             g_free(debug_info);
    //             node->restart_pipeline();
    //             break; 
    //         }
    //         case GST_MESSAGE_EOS: {
    //             RCLCPP_WARN(node->get_logger(), "Pipeline EOS, restarting");
    //             node->restart_pipeline();
    //             break; 
    //         }
    //         default: break;
    //     }

    //     return TRUE;
    // }
};

int main(int argc, char* argv[]){
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CameraNode>());
    rclcpp::shutdown();
    return 0;
}