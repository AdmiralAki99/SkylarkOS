#include <chrono>
#include <memory>
#include <string>
#include <set>
#include <algorithm>

#include "skylark_interfaces/msg/detection_array.hpp"
#include "skylark_interfaces/msg/track_array.hpp"

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp_lifecycle/lifecycle_publisher.hpp"

#include "KalmanTrack.hpp"
#include "Hungarian.h"


class TrackingNode: public rclcpp_lifecycle::LifecycleNode{
    public:
        explicit TrackingNode(const rclcpp::NodeOptions& options): rclcpp_lifecycle::LifecycleNode("tracking_node", options){
            RCLCPP_INFO(get_logger(), "TrackingNode has been created.");
        }

        // Creating the lifecycle functions for the node
        rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_configure(const rclcpp_lifecycle::State &) override{
            
            // Declaring the parameters for the tracking node
            declare_parameter("max_age",3);
            declare_parameter("min_hits",2);
            declare_parameter("iou_threshold",0.5);

            // Getting the parameters to be used in the TrackingNode
            max_age_ = get_parameter("max_age").as_int();
            min_hits_ = get_parameter("min_hits").as_int();
            iou_threshold_ = get_parameter("iou_threshold").as_double();

            RCLCPP_INFO(get_logger(), "Initialized tracker node variables");
            
            // Creating the subscriptions needed with the right callback
            detection_subscription_ = this->create_subscription<skylark_interfaces::msg::DetectionArray>(
                "/detection_results",
                10,
                std::bind(&TrackingNode::tracking_callback,
                            this,
                            std::placeholders::_1)
            );

            RCLCPP_INFO(get_logger(), "Subscribed to the detection results topic.");

            // Creating the tracker publisher
            tracking_publisher_= this->create_publisher<skylark_interfaces::msg::TrackArray>("/tracking/tracks",10);

            RCLCPP_INFO(get_logger(), "Created the tracking publisher for the tracking.");

            return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
        }

        rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_activate(const rclcpp_lifecycle::State &) override{
            // Need to only activate the publishers for the node
            tracking_publisher_->on_activate();

            RCLCPP_INFO(get_logger(), "Activated the tracking publisher.");

            return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
        }

        rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_deactivate(const rclcpp_lifecycle::State &) override{
            // Need to deactivate the publisher
            tracking_publisher_->on_deactivate();
            RCLCPP_INFO(get_logger(), "Deactivated the tracking publisher.");

            return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
        }

        rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override{
            // Need to reset all the publishers
            tracking_publisher_.reset();
            RCLCPP_INFO(get_logger(), "Reset complete of the tracking publisher.");

            // Clearing the tracks
            tracks_.clear();
            RCLCPP_INFO(get_logger(), "Reset complete of the Kalman tracks");

            return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
        }
    private:

        // Need a subscriber to take the detection info from the messages and used them
        rclcpp::Subscription<skylark_interfaces::msg::DetectionArray>::SharedPtr detection_subscription_;

        // Creating a publisher to publish the tracked objects
        rclcpp_lifecycle::LifecyclePublisher<skylark_interfaces::msg::TrackArray>::SharedPtr tracking_publisher_;
        
        // Variables for the Tracker
        int max_age_;
        int min_hits_;
        float iou_threshold_;
        std::vector<KalmanTrack> tracks_;

        // Hungarian solver
        HungarianAlgorithm solver_;
        void tracking_callback(const skylark_interfaces::msg::DetectionArray& detections){

            std::vector<cv::Rect2f> detection_boxes; 
            std::vector<skylark_interfaces::msg::Detection> detectionList;
            // Need to iterate over the detection array
            for(const auto& detection: detections.detections){
                float x1 = detection.x1;
                float y1 = detection.y1;
                float x2 = detection.x2;
                float y2 = detection.y2;

                cv::Rect2f rect = cv::Rect2f(x1,y1,x2-x1,y2-y1);

                // Add it to the local boxes to push it to the tracking
                detection_boxes.push_back(rect);
                detectionList.push_back(detection);
            }

            RCLCPP_INFO(get_logger(), "Initialized the detection boxes.");

            std::vector<cv::Rect2f> predicted_boxes; 
            // Now need to create a IoU matrix
            for(auto& track: tracks_){
                cv::Rect2f predicted_track = track.predict();
                predicted_boxes.push_back(predicted_track);
            }

            RCLCPP_INFO(get_logger(), "Initialized the tracks for the Hungarian solver.");

            if(predicted_boxes.empty() || detection_boxes.empty()){
                RCLCPP_INFO(get_logger(), "Predicted boxes or detection boxes are empty.");
                for(size_t i = 0; i < detection_boxes.size(); i++){
                    tracks_.push_back(KalmanTrack(detection_boxes[i]));
                }

                return;
            }

            
            std::vector<std::vector<double>> costMatrix(predicted_boxes.size(), std::vector<double>(detection_boxes.size(),0.0));
            for(size_t outerIndex=0 ;outerIndex < predicted_boxes.size(); outerIndex++){
                // Now iterating over the tracks
                for(size_t innerIndex=0; innerIndex < detection_boxes.size(); innerIndex++){
                    // Calculating the IoU cost matrix
                    cv::Rect2f intersection = predicted_boxes[outerIndex] & detection_boxes[innerIndex];
                    float intersectionArea = intersection.area();

                    float unionArea = predicted_boxes[outerIndex].area() + detection_boxes[innerIndex].area() - intersectionArea;
                    float iou = (unionArea > 0.0f)? intersectionArea / unionArea : 0.0f;
                    double cost = 1.0 - iou;
                    costMatrix[outerIndex][innerIndex] = cost;
                }
            }

            RCLCPP_INFO(get_logger(), "Computed cost matrix.");
            std::vector<int> assignments;
            solver_.Solve(costMatrix, assignments);

            // Getting the tracks for the different objects present
            for(size_t index= 0; index < tracks_.size(); index++){

                if(assignments[index] != -1){
                    // There is an assignment for a particular track
                    // Checking if the cost is correct
                    if (costMatrix[index][assignments[index]] < (1.0 - iou_threshold_)){
                        // The IoU is high enough
                        tracks_[index].update(detection_boxes[assignments[index]],detectionList[assignments[index]].class_id,detectionList[assignments[index]].label,detectionList[assignments[index]].confidence);
                    }

                }
            }

            RCLCPP_INFO(get_logger(), "Assigned detections to the tracks.");

             // There is no match for the track
            std::set<int> assignedDetections;
            for(size_t innerIndex= 0; innerIndex < assignments.size(); innerIndex++){
                // Checking which of the assignments was not -1
                if(assignments[innerIndex] != -1){
                    assignedDetections.insert(assignments[innerIndex]);
                }
            }

            // Looping over the detection boxes
            for(size_t detectionIndex= 0; detectionIndex < detection_boxes.size(); detectionIndex++){
                if(assignedDetections.find(detectionIndex) == assignedDetections.end()){
                    tracks_.push_back(KalmanTrack(detection_boxes[detectionIndex]));
                }
            }

            // Remove dead tracks
            tracks_.erase(
                std::remove_if(tracks_.begin(), tracks_.end(), [this](const KalmanTrack& track){
                     return track.time_since_last_update_ > max_age_;
                }),
                tracks_.end()
            );

            RCLCPP_INFO(get_logger(), "Erased the tracks and detections.");

            auto trackArrayMessage = skylark_interfaces::msg::TrackArray();
            trackArrayMessage.header = detections.header;

            // Now storing the tracks into the array
            for(auto& track : tracks_){
                if(track.hits_ >= min_hits_){
                    auto trackMessage = skylark_interfaces::msg::Track();
                    trackMessage.tracking_id = track.tracking_id_;

                    cv::Rect2f boundingBox = track.get_current_state_bounding_box();
                    float x1 = boundingBox.x;
                    float y1 = boundingBox.y;
                    float x2 = boundingBox.x + boundingBox.width;
                    float y2 = boundingBox.y + boundingBox.height; 

                    trackMessage.x1 = x1;
                    trackMessage.y1 = y1;
                    trackMessage.x2 = x2;
                    trackMessage.y2 = y2;

                    trackMessage.confidence = track.confidence_;
                    trackMessage.label = track.label_;
                    trackMessage.class_id = track.class_id_;

                    trackArrayMessage.tracks.push_back(trackMessage);
                }
            }

            // Publishing the tracks
            tracking_publisher_->publish(trackArrayMessage);
        }

};

int main(int argc, char* argv[]){
    rclcpp::init(argc, argv);
    rclcpp::executors::MultiThreadedExecutor executor;
    auto node = std::make_shared<TrackingNode>(rclcpp::NodeOptions());
    executor.add_node(node->get_node_base_interface());
    executor.spin();
    rclcpp::shutdown();
    return 0;
}