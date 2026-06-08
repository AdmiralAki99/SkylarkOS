#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/header.hpp>
#include <sensor_msgs/msg/image.hpp>

class CameraNode : public rclcpp::Node
{
    public:
    CameraNode() : Node("camera_node"){

        // Declaring parameters
        declare_parameter("sensor_id", 0);
        declare_parameter("width", 640);
        declare_parameter("height", 480);
        declare_parameter("fps", 60);
        declare_parameter("udp_host", std::string("10.0.0.2"));
        declare_parameter("udp_port", 5600);

        // Getting parameters
        sensor_id_ = get_parameter("sensor_id").as_int();
        width_ = get_parameter("width").as_int();
        height_ = get_parameter("height").as_int();
        fps_ = get_parameter("fps").as_int();

        std::string udp_host = get_parameter("udp_host").as_string();
        int udp_port = get_parameter("udp_port").as_int();

        auto qos = rclcpp::QoS(1).best_effort();

        // Creating the publishers for the raw and compressed images
        image_pub_ = this->create_publisher<sensor_msgs::msg::Image>("/camera/image_raw", qos);

        // Initializing the Gstreamer pipeline
        gst_init(nullptr, nullptr);
        std::string pipeline_description = "nvarguscamerasrc sensor-id="+ std::to_string(sensor_id_) + " ! video/x-raw(memory:NVMM),width="+std::to_string(width_) + ","
                                            "height="+std::to_string(height_) + ",framerate="+std::to_string(fps_) + "/1 ! "
                                            "tee name=t "
                                            // Raw Sink
                                            // To reduce overhead, using the dedicated hardware conversion in the GStreamer Pipeline nvvidconv
                                            "t. ! queue ! nvvidconv ! video/x-raw,format=BGRx ! appsink name=raw_sink emit-signals=true max-buffers=1 drop=true "
                                            // Stream Sink
                                            // To reduce overhead, using the dedicated hardware encoding in the GStreamer Pipeline nvjpegenc
                                            "t. ! queue ! nvvidconv ! video/x-raw,width=640,height=360,format=I420 ! "
                                            "videorate ! video/x-raw,framerate=25/1 ! "
                                            "nvjpegenc quality=35 ! rtpjpegpay ! "
                                            "udpsink host=" + udp_host + " port=" + std::to_string(udp_port) + " sync=true buffer-size=8388608"

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
    }

    private:
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;
    GstElement* pipeline_;

    int sensor_id_ = 0;
    int width_ = 640;
    int height_ = 480;
    int fps_ = 30;

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
};

int main(int argc, char* argv[]){
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CameraNode>());
    rclcpp::shutdown();
    return 0;
}