#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/compressed_image.hpp> 
#include <std_msgs/msg/int32.hpp>
#include <skylark_interfaces/msg/track_array.hpp>
#include <thread>

// GStreamer Libraries
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

struct MsgHolder { sensor_msgs::msg::CompressedImage::SharedPtr msg; };
static void destroy_msg(gpointer data) { delete static_cast<MsgHolder*>(data); }

class StreamingNode : public rclcpp::Node{
    public:
        StreamingNode() : Node("streaming_node"){

            declare_parameter("udp_host", std::string("10.0.0.2"));
            declare_parameter("udp_port", 5600);

            std::string host = get_parameter("udp_host").as_string();
            int port = get_parameter("udp_port").as_int();
            auto qos = rclcpp::QoS(1).best_effort();

            compressed_image_subscription_ = this->create_subscription<sensor_msgs::msg::CompressedImage>(
                "/camera/image_compressed",
                qos,
                std::bind(&StreamingNode::image_callback,
                this,
                std::placeholders::_1)
            );

            // locked_id_subscription_ = this->create_subscription<std_msgs::msg::Int32>(
            //     "/identity/locked_tracking_id",
            //     qos,
            //     std::bind(&StreamingNode::locked_id_callback, this, std::placeholders::_1)
            // );

            // track_array_subscription_ = this->create_subscription<skylark_interfaces::msg::TrackArray>(
            //     "/tracking/tracks",
            //     qos,
            //     std::bind(&StreamingNode::track_array_callback, this, std::placeholders::_1)
            // );

            // Initializing the Gstreamer pipeline
            gst_init(nullptr, nullptr);

            std::string pipeline= "appsrc name=src is-live=true format=time do-timestamp=true ! image/jpeg,width=640,height=360,framerate=25/1 ! rtpjpegpay ! "
                                  "udpsink host=" + host + " port=" + std::to_string(port) + " sync=true buffer-size=8388608";

            GError* error = nullptr;
            pipeline_ = gst_parse_launch(pipeline.c_str(), &error);
            if(!pipeline_){
                RCLCPP_ERROR(get_logger(), "Failed to create pipeline: %s", error ? error->message : "unknown");
                if (error) g_error_free(error);
                return;
            }
            
            appsrc_ = GST_APP_SRC(gst_bin_get_by_name(GST_BIN(pipeline_), "src"));

            gst_element_set_state(pipeline_, GST_STATE_PLAYING);
            RCLCPP_INFO(get_logger(), "Streaming to udp://%s:%d", host.c_str(), port);
        }

        ~StreamingNode(){
            if(pipeline_){
                gst_element_send_event(pipeline_, gst_event_new_eos());
                gst_element_set_state(pipeline_, GST_STATE_NULL);
                gst_object_unref(GST_ELEMENT(appsrc_));
                gst_object_unref(pipeline_);
            }
        }

    private:
    
        rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr compressed_image_subscription_;
        rclcpp::Subscription<skylark_interfaces::msg::TrackArray>::SharedPtr track_array_subscription_;
        rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr locked_id_subscription_;

        GstAppSrc* appsrc_ = nullptr;
        GstElement* pipeline_ = nullptr;

        void image_callback(const sensor_msgs::msg::CompressedImage::SharedPtr message){
            if(appsrc_ == nullptr) return;

            gsize size = message->data.size();
            auto* holder = new MsgHolder{message};
            GstBuffer* buf = gst_buffer_new_wrapped_full(
            GST_MEMORY_FLAG_READONLY,
            const_cast<uint8_t*>(message->data.data()),size, 0, size,holder, destroy_msg);
            gst_app_src_push_buffer(appsrc_, buf);
        }

        void locked_id_callback(const std_msgs::msg::Int32::SharedPtr message){
            
        }

        void track_array_callback(const skylark_interfaces::msg::TrackArray::SharedPtr message){

        }
};

int main(int argc, char* argv[]){
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<StreamingNode>());
    rclcpp::shutdown();
    return 0;
}