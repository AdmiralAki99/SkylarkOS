#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int32.hpp>
#include <skylark_interfaces/msg/track_array.hpp>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include <algorithm>

struct TrackPacket {
    uint32_t timestamp_ms;
    uint8_t  count;
    struct { int32_t id; float x1, y1, x2, y2; } tracks[16];
};

class StreamingNode : public rclcpp::Node{
    public:
        StreamingNode() : Node("streaming_node"){

            declare_parameter("udp_host", std::string("10.0.0.2"));
            declare_parameter("udp_metadata_port", 5601);

            std::string host = get_parameter("udp_host").as_string();
            int metadata_port = get_parameter("udp_metadata_port").as_int();
            auto qos = rclcpp::QoS(1).best_effort();

            // locked_id_subscription_ = this->create_subscription<std_msgs::msg::Int32>(
            //     "/identity/locked_tracking_id",
            //     qos,
            //     std::bind(&StreamingNode::locked_id_callback, this, std::placeholders::_1)
            // );

            track_array_subscription_ = this->create_subscription<skylark_interfaces::msg::TrackArray>(
                "/tracking/tracks",
                qos,
                std::bind(&StreamingNode::track_array_callback, this, std::placeholders::_1)
            );

            sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);

            if(sock_fd_ == -1){
                RCLCPP_ERROR(this->get_logger(), "UDP Metadata Stream Socket not started at PORT: %d",metadata_port);
                return;
            }

            memset(&dest_addr, 0, sizeof(dest_addr));
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(metadata_port);
            inet_pton(AF_INET, host.c_str(), &dest_addr.sin_addr);
            
        }

        ~StreamingNode(){
            if(sock_fd_ >= 0){
                close(sock_fd_);
            }
        }

    private:

        int sock_fd_ = -1 ;
        sockaddr_in dest_addr;

        rclcpp::Subscription<skylark_interfaces::msg::TrackArray>::SharedPtr track_array_subscription_;
        rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr locked_id_subscription_;

        void locked_id_callback(const std_msgs::msg::Int32::SharedPtr message){
            
        }

        void track_array_callback(const skylark_interfaces::msg::TrackArray::SharedPtr message){
            TrackPacket packet;
            memset(&packet,0,sizeof(TrackPacket));

            packet.timestamp_ms = message->header.stamp.sec * 1000 + message->header.stamp.nanosec / 1000000;
            packet.count = std::min((int)message->tracks.size(), 16);
            for(int index=0; index < packet.count; index++){
                int32_t id = message->tracks[index].tracking_id;
                float x1 = message->tracks[index].x1;
                float y1 = message->tracks[index].y1;
                float x2 = message->tracks[index].x2;
                float y2 = message->tracks[index].y2;
                
                packet.tracks[index] = {id,x1,y1,x2,y2};
            }

            sendto(sock_fd_, &packet, sizeof(packet), 0, (sockaddr*)&dest_addr, sizeof(dest_addr));
        }
};

int main(int argc, char* argv[]){
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<StreamingNode>());
    rclcpp::shutdown();
    return 0;
}