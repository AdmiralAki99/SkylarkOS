#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

class Listener: public rclcpp::Node{
    public:
    //Constructor
    Listener(): Node("listener"){
        this->subscriber_ = this->create_subscription<std_msgs::msg::String>(
            "chatter", 10, std::bind(&Listener::subsrciption_callback, this, std::placeholders::_1)
        );
    }

    private:
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscriber_;
    void subsrciption_callback(const std_msgs::msg::String::SharedPtr message){
        RCLCPP_INFO(this->get_logger(), "I heard: %s", message->data.c_str());
    }
};


int main(int argc, char *argv[]){
    // Create the ROS 2 system
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Listener>());
    rclcpp::shutdown();
    return 0;
}