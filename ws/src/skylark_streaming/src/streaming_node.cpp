#include <rclcpp/rclcpp.hpp>
#include <opencv2/opencv.hpp>
#include <sensor_msgs/msg/compressed_image.hpp> 
#include <std_msgs/msg/int32.hpp>
#include <skylark_interfaces/msg/track_array.hpp>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>

// TCP libraries
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

class StreamingNode : public rclcpp::Node{
    public:
        StreamingNode() : Node("streaming_node"){

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

            // Creating a TCP server socket
            server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
            if(server_fd_ == -1){
                RCLCPP_ERROR(get_logger(), "Failed to create TCP socket.");
                return;
            }

            int option = 1;
            setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

            server_address_.sin_family = AF_INET;
            server_address_.sin_addr.s_addr = INADDR_ANY;
            server_address_.sin_port = htons(8080);

            if(bind(server_fd_, (struct sockaddr*)&server_address_, sizeof(server_address_)) < 0){
                RCLCPP_ERROR(get_logger(), "Failed to bind TCP socket.");
                close(server_fd_);
                return;
            }

            if (listen(server_fd_, 5) < 0){
                RCLCPP_ERROR(get_logger(), "Failed to listen on TCP socket.");
                close(server_fd_);
                return;
            }

            RCLCPP_INFO(get_logger(), "TCP server is listening on port 8080.");

            // Launching the encoding and streaming threads
            streaming_thread_ = std::thread(&StreamingNode::streaming_thread, this);
            encoding_thread_ = std::thread(&StreamingNode::encoding_thread, this);
            
        }

    private:
    
        rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr compressed_image_subscription_;
        rclcpp::Subscription<skylark_interfaces::msg::TrackArray>::SharedPtr track_array_subscription_;
        rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr locked_id_subscription_;

        std::vector<uint8_t> incoming_jpeg_;
        std::vector<uint8_t> streamed_jpeg_;
        std::vector<skylark_interfaces::msg::Track> tracks_;
        int32_t locked_id_{-1};
        std::mutex frame_mutex_;
        std::mutex streaming_mutex_;
        std::condition_variable streaming_condition_;
        std::condition_variable frame_condition_;
        uint64_t frame_counter_{0};

        std::thread encoding_thread_;
        std::thread streaming_thread_;

        std::atomic<bool> new_frame_available_{false};
        std::atomic<bool> new_stream_available_{false};

        int server_fd_ = -1;
        int client_socket_ = -1;
        sockaddr_in server_address_;
        std::chrono::steady_clock::time_point last_frame_time_ = std::chrono::steady_clock::now();

        void image_callback(const sensor_msgs::msg::CompressedImage::SharedPtr message){
            {
                std::lock_guard<std::mutex> lock(frame_mutex_);
                incoming_jpeg_ = message->data;
                new_frame_available_ = true;
            }
            frame_condition_.notify_one();
        }

        void locked_id_callback(const std_msgs::msg::Int32::SharedPtr message){
            std::lock_guard<std::mutex> lock(streaming_mutex_);
            locked_id_ = message->data;
        }

        void track_array_callback(const skylark_interfaces::msg::TrackArray::SharedPtr message){
            std::lock_guard<std::mutex> lock(streaming_mutex_);
            tracks_ = message->tracks;
        }

        bool send_all(int socket, const uint8_t* data, size_t size){
            const char* data_ptr = reinterpret_cast<const char*>(data);
            size_t total_sent = 0;
            while(total_sent < size){
                int remaining = size - total_sent;
                ssize_t sent = send(socket, data_ptr, remaining, MSG_NOSIGNAL);
                if(sent <= 0){
                    return false;
                }

                data_ptr = data_ptr + sent;
                total_sent = total_sent + sent;
            }

            return true;
        }

        void client_handler(int client_socket){
            int flag = 1;
            setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

            // Writing the MJPEG stream header
            std::string stream_header = "HTTP/1.0 200 OK\r\n"
                                        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";

            send(client_socket, stream_header.c_str(), stream_header.size(), 0);
            uint64_t last_frame = 0;
            while(true){
                std::vector<uint8_t> jpeg_data;
                {
                    std::unique_lock<std::mutex> lock(streaming_mutex_);
                    streaming_condition_.wait(lock, [this, &last_frame]{ return frame_counter_ > last_frame; });
                    jpeg_data = streamed_jpeg_;
                    last_frame = frame_counter_;
                }
                std::string boundary = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: " + std::to_string(jpeg_data.size()) + "\r\n\r\n";
                std::vector<uint8_t> packet;
                packet.insert(packet.end(), boundary.begin(), boundary.end());
                packet.insert(packet.end(), jpeg_data.begin(), jpeg_data.end());
                if(!send_all(client_socket, packet.data(), packet.size())){
                    break;
                }
            }
            
            close(client_socket);
        }

        void streaming_thread(){
            while(true){
                sockaddr_in client_addr{};
                socklen_t addr_len = sizeof(client_addr);
                int client_socket = accept(server_fd_, (struct sockaddr*)&client_addr, &addr_len);
                if (client_socket < 0) {
                    continue;
                }
                std::thread(&StreamingNode::client_handler, this, client_socket).detach();
            }
        }

        void encoding_thread(){
            while(true){
                std::unique_lock<std::mutex> lock(frame_mutex_);
                frame_condition_.wait(lock, [this]{ return new_frame_available_.load();});
                // Getting the frame from the buffer
                std::vector<uint8_t> jpeg_data = incoming_jpeg_;
                new_frame_available_ = false;
                lock.unlock();

                // cv::Mat frame = cv::imdecode(jpeg_data, cv::IMREAD_COLOR);
                // if(frame.empty()){
                //     RCLCPP_ERROR(get_logger(), "Failed to decode incoming JPEG image.");
                //     continue;
                // }

                auto now = std::chrono::steady_clock::now();
                double fps = 1.0 / std::chrono::duration<double>(now - last_frame_time_).count();
                last_frame_time_ = now;
                // cv::putText(frame, "FPS: " + std::to_string(static_cast<int>(fps)),
                //     cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                //     cv::Scalar(0, 255, 0), 2);

                // std::vector<uint8_t> reencoded_jpeg;
                // cv::imencode(".jpg", frame, reencoded_jpeg, {cv::IMWRITE_JPEG_QUALITY, 60});
                {
                    std::lock_guard<std::mutex> streaming_lock(streaming_mutex_);
                    streamed_jpeg_ = std::move(jpeg_data);
                    frame_counter_++;
                }
                streaming_condition_.notify_all();
            }
        }


};

int main(int argc, char* argv[]){
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<StreamingNode>());
    rclcpp::shutdown();
    return 0;
}