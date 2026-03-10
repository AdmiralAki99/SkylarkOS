#include <chrono>
#include <memory>
#include <string>
#include <algorithm>

#include "skylark_interfaces/msg/detection_array.hpp"
#include "sensor_msgs/msg/image.hpp"

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp_lifecycle/lifecycle_publisher.hpp"

// Including the ONNX model
#include "onnxruntime_cxx_api.h"

// Including the cv_bridge
#include "cv_bridge/cv_bridge.h"
#include <opencv2/opencv.hpp>

#define DETECTION_TIMER_MS 500

std::vector<skylark_interfaces::msg::Detection> non_max_suppression(std::vector<skylark_interfaces::msg::Detection>& detections, float iou_threshold);
float compute_iou(skylark_interfaces::msg::Detection& a, skylark_interfaces::msg::Detection& b);
class PerceptionNode : public rclcpp_lifecycle::LifecycleNode{
    public:
        explicit PerceptionNode(const rclcpp::NodeOptions & options): rclcpp_lifecycle::LifecycleNode("perception_node", options), env_(ORT_LOGGING_LEVEL_WARNING, "perception_node"){
            RCLCPP_INFO(get_logger(), "PerceptionNode has been created.");
        }

        // Callbacks for the lifecycle states
        rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_configure(const rclcpp_lifecycle::State &) override{

            declare_parameter("model_path", "");
            declare_parameter("input_width", 300);
            declare_parameter("input_height", 300);
            declare_parameter("confidence_threshold", 0.5f);
            declare_parameter("nms_threshold", 0.5f);

            // Get the model path from the parameters
            std::string model_path = get_parameter("model_path").as_string();

            // Get the input dimensions from the parameters
            input_width_ = get_parameter("input_width").as_int();
            input_height_ = get_parameter("input_height").as_int();

            // Get the confidence threshold from the parameters
            confidence_threshold_ = get_parameter("confidence_threshold").as_double();

            // Get the NMS threshold from the parameters
            nms_threshold_ = get_parameter("nms_threshold").as_double();

            // Validating the model path
            if(model_path.empty()){
                RCLCPP_ERROR(get_logger(), "Model path is empty. Please provide a valid model path.");
                return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::FAILURE;
            }

            // Creating the subscription to the input frames
            image_subscription_ = this->create_subscription<sensor_msgs::msg::Image>(
                "/camera/image_raw",
                10,
                std::bind(&PerceptionNode::image_callback,
                this,
                std::placeholders::_1)
            );

            RCLCPP_INFO(get_logger(), "Subscribed to input frames topic.");
            
            // Creating a publisher for the detected frames
            image_publisher_ = this->create_publisher<sensor_msgs::msg::Image>("detected_frames",10);

            RCLCPP_INFO(get_logger(), "Attached detection frames publisher to the lifecycle node.");

            // Creating a publisher for the detection results
            detection_publisher_ = this->create_publisher<skylark_interfaces::msg::DetectionArray>("detection_results",10);

            RCLCPP_INFO(get_logger(), "Attached detection results publisher to the lifecycle node.");

            // Loading the ONNX model
            try{
                session_ = std::make_unique<Ort::Session>(env_, model_path.c_str(), session_options_);
                RCLCPP_INFO(get_logger(), "Loaded ONNX model from path: %s", model_path.c_str());
            }catch (const Ort::Exception & e){
                RCLCPP_ERROR(get_logger(), "Failed to load ONNX model: %s", e.what());
                RCLCPP_ERROR(get_logger(), "Please check the model path and ensure the model is compatible with ONNX Runtime.");
                return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::FAILURE;
            }
            
            return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
        }

        rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_activate(const rclcpp_lifecycle::State &) override{
            
            // Activate the publishers to start publishing messages
            image_publisher_->on_activate();

            RCLCPP_INFO(get_logger(), "Activated detection frames publisher.");

            detection_publisher_->on_activate();

            RCLCPP_INFO(get_logger(), "Activated detection results publisher.");

            return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
        }

        rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_deactivate(const rclcpp_lifecycle::State &) override{
            
            // Deactivate the publishers to stop publishing messages
            image_publisher_->on_deactivate();

            RCLCPP_INFO(get_logger(), "Deactivated detection frames publisher.");

            detection_publisher_->on_deactivate();

            RCLCPP_INFO(get_logger(), "Deactivated detection results publisher.");

            return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
        }

        rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override{
            
            // Reset the publishers to free resources
            image_publisher_.reset();
            detection_publisher_.reset();

            // Reset the ONNX session to free resources
            session_.reset();
            RCLCPP_INFO(get_logger(), "Cleaned up ONNX session and freed resources.");

            return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
        }

    private:
    // Creating the publisher to trigger the lifecycle callbacks and the detected frames
    rclcpp_lifecycle::LifecyclePublisher<sensor_msgs::msg::Image>::SharedPtr image_publisher_; // Publisher for detected frames
    rclcpp_lifecycle::LifecyclePublisher<skylark_interfaces::msg::DetectionArray>::SharedPtr detection_publisher_; // Publisher for detection results
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_subscription_; // Subscription for input frames

    // ONNX Runtime environment variables
    Ort::Env env_;
    Ort::SessionOptions session_options_;
    std::unique_ptr<Ort::Session> session_; // Pointer to the ONNX Session for the model

    // Image variables
    int input_width_;
    int input_height_;
    float confidence_threshold_;
    float nms_threshold_;

    void image_callback(const sensor_msgs::msg::Image::SharedPtr message){
        //TODO: Add NMS at the end 
        // Creating a cv_bridge to convert the ROS image message into a matrix
        cv_bridge::CvImagePtr cv_image_ptr;
        cv::Mat input_image;
        cv::Mat rgb;
        try{
            cv_image_ptr = cv_bridge::toCvCopy(message, "bgr8");
            input_image = cv_image_ptr->image;
        } catch (cv_bridge::Exception & e){
            RCLCPP_ERROR(get_logger(), "cv_bridge exception: %s", e.what());
            return;
        }

        cv::Mat resized_image;
        cv::resize(input_image, resized_image, cv::Size(input_width_, input_height_));

        // Image is converted and now can be used.
        RCLCPP_INFO(get_logger(), "Received frame: %dx%d", input_image.cols, input_image.rows);
        cv::cvtColor(resized_image, rgb, cv::COLOR_BGR2RGB);

        // Convert to float
        cv::Mat float_image;
        rgb.convertTo(float_image, CV_32F, 1.0 / 255.0);

        // Building the ONNX Tensor
        std::vector<int64_t> input_dims = {1, input_height_, input_width_, 3}; // Batch size, height, width, channels

        size_t input_size = 1 * input_height_ * input_width_ * 3;

        auto allocation = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(allocation, reinterpret_cast<float*>(float_image.data), input_size, input_dims.data(), input_dims.size());
        const char* input_names[] = {"input_image"};
        const char* output_names[] = {"boxes", "scores"};

        auto output_tensors = session_->Run(
            Ort::RunOptions{nullptr},
            input_names,
            &input_tensor,
            1,
            output_names,
            2
        );

        // Get the raw info from the output
        float* boxes = output_tensors[0].GetTensorMutableData<float>();
        float* scores = output_tensors[1].GetTensorMutableData<float>();

        // Setting an empty detection array message
        auto detection_array_message = skylark_interfaces::msg::DetectionArray();
        detection_array_message.header = message->header;

        // Need to loop over all the detections and filter them
        for(int index = 0; index <= 13501; index++){
            // Getting the scores for the box
            float* row_start = scores + index * 21 + 1;
            float* row_end = scores + index * 21 + 21;

            auto row_max = std::max_element(row_start, row_end);
            float best_score = *row_max;
            int best_label = (row_max - row_start) + 1;

            // Filtering the detection based on the confidence threshold
            if(best_score >= confidence_threshold_){
                RCLCPP_INFO(get_logger(), "Detection %d: Label=%d, Score=%.2f", index, best_label, best_score);

                // Getting the coordinates
                float x1 = boxes[4 * index + 0];
                float y1 = boxes[4 * index + 1];
                float x2 = boxes[4 * index + 2];
                float y2 = boxes[4 * index + 3];

                auto detection = skylark_interfaces::msg::Detection();
                detection.confidence = best_score;
                detection.class_id = best_label;
                detection.x1 = x1;
                detection.y1 = y1;
                detection.x2 = x2;
                detection.y2 = y2;
                detection.label = "Class " + std::to_string(best_label);

                detection_array_message.detections.push_back(detection);
            }

        }

        detection_array_message.detections = non_max_suppression(detection_array_message.detections, nms_threshold_);

        detection_publisher_->publish(detection_array_message);

        // Publishing the original frame with the detections (for visualization purposes)
        cv::Mat annotated_image = resized_image.clone();

        for(const auto& detection: detection_array_message.detections){
            // Getting the box from the detections
            int x1 = static_cast<int>(detection.x1 * input_width_);
            int y1 = static_cast<int>(detection.y1 * input_height_);
            int x2 = static_cast<int>(detection.x2 * input_width_);
            int y2 = static_cast<int>(detection.y2 * input_height_);

            // Adding the detection on the image
            cv::rectangle(annotated_image, cv::Point(x1,y1), cv::Point(x2, y2), cv::Scalar(0, 255, 0), 2);

            cv::putText(annotated_image, detection.label, cv::Point(x1, y1 - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,255,0), 1);
        }

        // Publishing the image
        auto annotated_image_message = cv_bridge::CvImage(message->header, "bgr8", annotated_image).toImageMsg();
        image_publisher_->publish(*annotated_image_message);
        
    }


};

std::vector<skylark_interfaces::msg::Detection> non_max_suppression(std::vector<skylark_interfaces::msg::Detection>& detections, float iou_threshold){
    // Need to first sort the best boxes based on the scores that are there
    std::sort(detections.begin(), detections.end(), [](const skylark_interfaces::msg::Detection& a, const skylark_interfaces::msg::Detection& b){
        return a.confidence > b.confidence;
    });

    // Creating a output vector
    std::vector<skylark_interfaces::msg::Detection> kept;
    std::vector<bool> suppressed(detections.size(), false);

    // Iterate over every detection
    for(size_t  outer_index=0 ; outer_index < detections.size(); outer_index++){
        // Checking if the outer box is suppressed
        if(suppressed[outer_index]){
            continue;
        }

        // This box is still in contention, so add it to the kept
        kept.push_back(detections[outer_index]);

        // Checking every other box after
        for(size_t  inner_index = outer_index + 1; inner_index < detections.size(); inner_index++){
            // Checking if the box is suppressed
            if(suppressed[inner_index]){
                continue;
            }

            float iou = compute_iou(detections[outer_index], detections[inner_index]);

            // Now checking if the IoU is over the threshold
            if(iou > iou_threshold){
                suppressed[inner_index] = true;
            }
        }
    }

    return kept;
}

float compute_iou(skylark_interfaces::msg::Detection& a, skylark_interfaces::msg::Detection& b){
    // Calculating a rectangle
    cv::Rect2f rect_a(a.x1, a.y1, a.x2 - a.x1, a.y2 - a.y1);
    cv::Rect2f rect_b(b.x1, b.y1, b.x2 - b.x1, b.y2 - b.y1);

    // Calculating the intersection
    cv::Rect2f intersection = rect_a & rect_b;
    float intersection_area = intersection.area();

    if(intersection_area == 0.0){
        return 0.0;
    }

    float union_area = rect_a.area() + rect_b.area() - intersection_area;

    if(union_area > 0.0){
        return intersection_area / union_area; // Calculating IoU
    }else{
        return 0.0;
    }
}

int main(int argc, char* argv[]){
    rclcpp::init(argc, argv);
    rclcpp::executors::MultiThreadedExecutor executor;
    auto node = std::make_shared<PerceptionNode>(rclcpp::NodeOptions());
    executor.add_node(node->get_node_base_interface());
    executor.spin();
    rclcpp::shutdown();
    return 0;
}