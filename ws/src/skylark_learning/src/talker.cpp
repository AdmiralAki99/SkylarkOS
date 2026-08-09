#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;


class Talker: public rclcpp::Node{
    public:
    // Constructor with the Node name as "talker"
    Talker(): Node("talker"){
        // Creating a publisher that will publish
        this->publisher_ = this->create_publisher<std_msgs::msg::String>("chatter",10);
        // Create a timer to call the callback
        this->timer_ = this->create_wall_timer(
            500ms, std::bind(&Talker::timer_callback, this)
        );
    }

    private:
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;

    void timer_callback(){
        // Creating a message to publish for the listener
        auto message = std_msgs::msg::String();
        message.data = "Hello World";
        // Get the current time from the logger and publish the message
        RCLCPP_INFO(this->get_logger(), "Publishing: %s", message.data.c_str());
        // Publishing the message
        this->publisher_->publish(message);
    }
};


int main(int argc, char *argv[]){
    // Initalize the ROS 2
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Talker>()); // Massive while loop to keep the node alive and processing callbacks
    rclcpp::shutdown();
    return 0;
}