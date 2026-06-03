#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/header.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/compressed_image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

class CameraNode : public rclcpp::Node
{
    public:
    CameraNode() : Node("camera_node"){

        // Declaring parameters
        declare_parameter("sensor_id", 0);
        declare_parameter("width", 640);
        declare_parameter("height", 480);
        declare_parameter("fps", 30);

        // Getting parameters
        sensor_id_ = get_parameter("sensor_id").as_int();
        width_ = get_parameter("width").as_int();
        height_ = get_parameter("height").as_int();
        fps_ = get_parameter("fps").as_int();

        auto qos = rclcpp::QoS(1).best_effort();

        // Creating the publishers for the raw and compressed images
        image_pub_ = this->create_publisher<sensor_msgs::msg::Image>("/camera/image_raw", qos);
        compressed_image_pub_ = this->create_publisher<sensor_msgs::msg::CompressedImage>("/camera/image_compressed", qos);

        // Initializing the Gstreamer pipeline
        gst_init(nullptr, nullptr);
        std::string pipeline_description = "nvarguscamerasrc sensor-id="+ std::to_string(sensor_id_) + " ! video/x-raw(memory:NVMM),width="+std::to_string(width_) + ","
                                            "height="+std::to_string(height_) + ",framerate="+std::to_string(fps_) + "/1 ! "
                                            "nvvidconv ! video/x-raw,format=BGRx ! videoconvert ! video/x-raw,format=BGR ! "
                                            "tee name=t "
                                            "t. ! queue ! appsink name=raw_sink emit-signals=true max-buffers=1 drop=true "
                                            "t. ! queue ! appsink name=compress_sink emit-signals=true max-buffers=1 drop=true";

        GError* error = nullptr;
        pipeline_ = gst_parse_launch(pipeline_description.c_str(), &error);
        if (!pipeline_) {
            RCLCPP_ERROR(get_logger(), "Failed to create pipeline: %s", error ? error->message : "unknown");
            if (error) g_error_free(error);
            return;
        }

        GstElement* raw_sink  = gst_bin_get_by_name(GST_BIN(pipeline_), "raw_sink");
        GstElement* compress_sink = gst_bin_get_by_name(GST_BIN(pipeline_), "compress_sink");

        // Connecting the sinks to the pipeline
        g_signal_connect(raw_sink, "new-sample", G_CALLBACK(on_raw_sample), this);
        g_signal_connect(compress_sink, "new-sample", G_CALLBACK(on_compress_sample), this);

        // Starting the pipeline
        gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    }

    private:
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;
    rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr compressed_image_pub_;
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
        cv::Mat raw_image = cv::Mat(node->height_, node->width_, CV_8UC3, (char*)map.data).clone();

        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);

        // Sending the raw image
        std_msgs::msg::Header header;
        header.stamp = node->now();

        auto message = cv_bridge::CvImage(header, "bgr8", raw_image).toImageMsg();
        node->image_pub_->publish(*message);

        return GST_FLOW_OK;       
    }

    static GstFlowReturn on_compress_sample(GstElement* sink, gpointer user_data){
        CameraNode* node = static_cast<CameraNode*>(user_data);
        // Publishing the compressed image and logging the event

        GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
        GstBuffer* buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        gst_buffer_map(buffer, &map, GST_MAP_READ);
        cv::Mat frame = cv::Mat(node->height_, node->width_, CV_8UC3, (char*)map.data).clone();

        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);

        std::vector<uint8_t> jpeg_data;
        cv::imencode(".jpg", frame, jpeg_data, {cv::IMWRITE_JPEG_QUALITY, 60});

        sensor_msgs::msg::CompressedImage message;
        message.header.stamp = node->now();
        message.format = "jpeg";
        message.data = jpeg_data;
        node->compressed_image_pub_->publish(message);
        
        return GST_FLOW_OK;
    }
};

int main(int argc, char* argv[]){
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CameraNode>());
    rclcpp::shutdown();
    return 0;
}