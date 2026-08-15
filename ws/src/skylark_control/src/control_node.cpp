#include <chrono>
#include <memory>
#include <string>
#include <algorithm>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp_lifecycle/lifecycle_publisher.hpp"

#include "px4_msgs/msg/vehicle_odometry.hpp"
#include "px4_msgs/msg/vehicle_status.hpp"
#include "px4_msgs/msg/offboard_control_mode.hpp"
#include "px4_msgs/msg/trajectory_setpoint.hpp"
#include "px4_msgs/msg/vehicle_command.hpp"

#include "skylark_interfaces/msg/track_array.hpp"

#include "std_srvs/srv/trigger.hpp"
#include "std_msgs/msg/int32.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/bool.hpp"

#include "pid_controller.hpp"

using namespace std::chrono_literals;

// Definitions for some commands
#define PX4_CUSTOM_MODE_OFFBOARD 6.0f
#define PX4_BASE_MODE_CUSTOM 1.0f

#define PERSON_CLASS 15

class ControlNode: public rclcpp_lifecycle::LifecycleNode{
    public:

    ControlNode(const rclcpp::NodeOptions& options): rclcpp_lifecycle::LifecycleNode("control_node", options){
        RCLCPP_INFO(get_logger(), "Created Control Node.");
    }

    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_configure(const rclcpp_lifecycle::State& ) override{

        // Declaring the parameters for the node
        declare_parameter("offboard_click_threshold",10);
        declare_parameter("takeoff_altitude", 10.0);
        declare_parameter("position_threshold", 0.3f);
        declare_parameter("lateral_gain", 0.5f);
        declare_parameter("distance_gain", 1.0f);
        declare_parameter("target_bbox_height_ratio", 0.38f);
        declare_parameter("kp_lateral", 1.0f);
        declare_parameter("kd_lateral", 0.1f);
        declare_parameter("kp_distance", 1.0f);
        declare_parameter("kd_distance", 0.1f);
        declare_parameter("max_velocity", 2.0f);
        declare_parameter("api_nudge_velocity", 0.5f);
        declare_parameter("api_nudge_duration", 0.3f);

        // Getting the parameters for the node
        offboard_setpoint_threshold_ = get_parameter("offboard_click_threshold").as_int();
        takeoff_altitude = get_parameter("takeoff_altitude").as_double();
        position_threshold = get_parameter("position_threshold").as_double();

        lateral_gain_ = get_parameter("lateral_gain").as_double();
        distance_gain_ = get_parameter("distance_gain").as_double();
        target_bbox_height_ratio_ = get_parameter("target_bbox_height_ratio").as_double();

        kp_lateral_ = get_parameter("kp_lateral").as_double();
        kd_lateral_ = get_parameter("kd_lateral").as_double();
        kp_distance_ = get_parameter("kp_distance").as_double();
        kd_distance_ = get_parameter("kd_distance").as_double();
        max_velocity_ = get_parameter("max_velocity").as_double();
        api_nudge_velocity_ = get_parameter("api_nudge_velocity").as_double();
        api_nudge_duration_ = get_parameter("api_nudge_duration").as_double();

        RCLCPP_INFO(get_logger(), "Set the offboard setpoint counter.");
        RCLCPP_INFO(get_logger(), "Initialized the takeoff altitude for the drone.");

        // Creating the publishers for the node
        offboard_control_mode_publisher_ = this->create_publisher<px4_msgs::msg::OffboardControlMode>("/fmu/in/offboard_control_mode",rclcpp::QoS(10).best_effort());
        
        RCLCPP_INFO(get_logger(), "Created the OffboardControlMode publisher.");

        trajectory_setpoint_publisher_ = this->create_publisher<px4_msgs::msg::TrajectorySetpoint>("/fmu/in/trajectory_setpoint", rclcpp::QoS(10).best_effort());

        RCLCPP_INFO(get_logger(), "Created the TrajectorySetpoint publisher.");

        vehicle_command_publisher_ = this->create_publisher<px4_msgs::msg::VehicleCommand>("/fmu/in/vehicle_command", rclcpp::QoS(10).best_effort());

        RCLCPP_INFO(get_logger(), "Created the VehicleCommand publisher.");

        // Creating the timer

        timer_ = this->create_wall_timer(50ms, std::bind(&ControlNode::timer_callback,this));

        RCLCPP_INFO(get_logger(), "Created the Timer.");

        // Creating the subscribers

        odometry_subscription_ = this->create_subscription<px4_msgs::msg::VehicleOdometry>(
            "/fmu/out/vehicle_odometry",
            rclcpp::SensorDataQoS(),
            std::bind(&ControlNode::vehicle_odometry_callback,this, std::placeholders::_1)
        );

        RCLCPP_INFO(get_logger(), "Created the VehicleOdometry subscriber.");

        vehicle_status_subscription_ = this->create_subscription<px4_msgs::msg::VehicleStatus>(
            "/fmu/out/vehicle_status",
            rclcpp::SensorDataQoS(),
            std::bind(&ControlNode::vehicle_status_callback, this, std::placeholders::_1)
        );

        RCLCPP_INFO(get_logger(), "Created the VehicleStatus subscriber.");

        tracked_array_subscription_ = this->create_subscription<skylark_interfaces::msg::TrackArray>(
            "/tracking/tracks",
            rclcpp::SensorDataQoS(),
            std::bind(&ControlNode::tracking_callback, this, std::placeholders::_1)
        );

        RCLCPP_INFO(get_logger(), "Creating the TrackArray subscriber.");

        identity_subscription_ = this->create_subscription<std_msgs::msg::Int32>(
            "/identity/locked_track_id",
            10,
            std::bind(&ControlNode::identity_callback, this, std::placeholders::_1)
        );

        RCLCPP_INFO(get_logger(), "Creating the Identity Track Lock subscriber.");

        gesture_subscription_ = this->create_subscription<std_msgs::msg::String>(
            "/gesture/command",
            10,
            std::bind(&ControlNode::gesture_callback, this, std::placeholders::_1)
        );

        RCLCPP_INFO(get_logger(), "Creating the Gesture command subscriber.");

        api_command_subscription_ = this->create_subscription<std_msgs::msg::String>(
            "/api/command",
            10,
            std::bind(&ControlNode::api_command_callback, this, std::placeholders::_1)
        );

        RCLCPP_INFO(get_logger(), "Creating the API command subscriber.");

        link_status_subscription_ = this->create_subscription<std_msgs::msg::Bool>(
            "/system/link_status",
            rclcpp::QoS(1).reliable(),
            std::bind(&ControlNode::link_status_callback, this, std::placeholders::_1)
        );

        RCLCPP_INFO(get_logger(), "Creating the Link Status subscriber.");

        // Creating the landing service
        landing_service_ = this->create_service<std_srvs::srv::Trigger>(
            "/land",
            std::bind(&ControlNode::landing_trigger, this, std::placeholders::_1, std::placeholders::_2)
        );

        RCLCPP_INFO(get_logger(), "Created the Landing service.");

        takeoff_service_ = this->create_service<std_srvs::srv::Trigger>(
            "/takeoff",
            std::bind(&ControlNode::takeoff_trigger, this, std::placeholders::_1, std::placeholders::_2)
        );

        RCLCPP_INFO(get_logger(), "Created the takeoff service.");       

        // Creating the PID Controllers
        pid_lateral_ = PIDController(kp_lateral_,kd_lateral_, 0.05f);
        pid_distance_ = PIDController(kp_distance_,kd_distance_,0.05f);

        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_activate(const rclcpp_lifecycle::State&) override{
        // Activate the publisher
        offboard_control_mode_publisher_->on_activate();

        RCLCPP_INFO(get_logger(), "Activated the OffboardControlMode publisher.");

        trajectory_setpoint_publisher_->on_activate();

        RCLCPP_INFO(get_logger(), "Activated the TrajectorySetpoint publisher.");

        vehicle_command_publisher_->on_activate();

        RCLCPP_INFO(get_logger(), "Activated the VehicleCommand publisher.");

        // Setting the flight state
        flight_state_ = FlightState::REQUESTING_OFFBOARD;
        RCLCPP_INFO(get_logger(), "Transitioned the drone to REQUESTING_OFFBOARD state.");

        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_deactivate(const rclcpp_lifecycle::State&) override{

        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_cleanup(const rclcpp_lifecycle::State&) override{

        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    private:
    float takeoff_altitude;
    float position_threshold;

    enum class FlightState {IDLE, REQUESTING_OFFBOARD, ARMING, ARMED_STANDBY, TAKEOFF, FLYING, LANDING, DISARMED};
    FlightState flight_state_ = FlightState::IDLE;

    skylark_interfaces::msg::TrackArray latest_tracks_;

    uint8_t nav_state_;
    uint8_t arming_state_;
    float current_x_;
    float current_y_;
    float current_z_;

    float target_x_ = 0.0f;
    float target_y_ = 0.0f;
    float target_z_ = 0.0f; // This is important since the PX4 used the North East Down (NED) mode negative takeoff takes it up.

    float lateral_gain_;
    float distance_gain_;
    float target_bbox_height_ratio_;

    float command_vx_ = 0.0f;
    float command_vy_ = 0.0f;


    int offboard_setpoint_counter_ = 0;
    int offboard_setpoint_threshold_;

    bool disarm_logged_ = false;

    int locked_tracking_id_ = -1;

    std::string current_gesture_ = "";
    std::string current_api_command_ = "";

    bool api_nudge_active_ = false;
    bool link_ok_ = true;
    rclcpp::Time api_nudge_start_;
    float api_nudge_duration_ = 0.3f;   // seconds a MOVE_* command displaces the drone before falling back to tracking
    float api_nudge_velocity_ = 0.5f;   // fixed velocity magnitude applied during a nudge

    rclcpp::Subscription<px4_msgs::msg::VehicleOdometry>::SharedPtr odometry_subscription_;
    rclcpp::Subscription<px4_msgs::msg::VehicleStatus>::SharedPtr vehicle_status_subscription_;
    rclcpp::Subscription<skylark_interfaces::msg::TrackArray>::SharedPtr tracked_array_subscription_;
    rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr identity_subscription_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr gesture_subscription_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr api_command_subscription_;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr link_status_subscription_;
    
    /*
    * OffboardControlMode is a message that tells what is the nature of the information being sent to the PX4
    * This explains what loops the PX4 needs to run on its behalf.
    */
    rclcpp_lifecycle::LifecyclePublisher<px4_msgs::msg::OffboardControlMode>::SharedPtr offboard_control_mode_publisher_;

    rclcpp_lifecycle::LifecyclePublisher<px4_msgs::msg::TrajectorySetpoint>::SharedPtr trajectory_setpoint_publisher_;
    rclcpp_lifecycle::LifecyclePublisher<px4_msgs::msg::VehicleCommand>::SharedPtr vehicle_command_publisher_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr landing_service_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr takeoff_service_;
    rclcpp::TimerBase::SharedPtr timer_;

    /*
    * PID Controller variables for the control node
    */
   PIDController pid_lateral_;
   PIDController pid_distance_;
   float max_velocity_;
   float kp_lateral_;
   float kp_distance_;
   float kd_lateral_;
   float kd_distance_;

    /**
     * Idea is this, every tick the drone does three steps:
     *  1. Tell the FC that it is controlled using program (autonomous)
     *  2. Publishes a setpoint (x,y,z) -> (vx,vy,0.0f, altitude) 
     *  3. Handle the state machine
     */
    void timer_callback(){
        // PX4 has a deadman's switch built in so the offboard control needs to be published every now and then to stop it from dying.
        publish_offboard_control_mode();
        publish_trajectory_setpoint(command_vx_,command_vy_, 0.0f, target_z_);
        manage_state_machine();
    }

    void gesture_callback(const std_msgs::msg::String::SharedPtr message){
        current_gesture_ = message->data;
    }

    void api_command_callback(const std_msgs::msg::String::SharedPtr message){
        current_api_command_ = message->data;
    }

    void link_status_callback(const std_msgs::msg::Bool::SharedPtr message){
        link_ok_ = message->data;
    }

    void vehicle_status_callback(const px4_msgs::msg::VehicleStatus::SharedPtr message){
        arming_state_ = message->arming_state;
        nav_state_ = message->nav_state;
    }

    void vehicle_odometry_callback(const px4_msgs::msg::VehicleOdometry::SharedPtr message){
        // Reading the vehicle odometry which is a vector of x,y,z values
        current_x_ = message->position[0];
        current_y_ = message->position[1];
        current_z_ = message->position[2];
    }

    void publish_vehicle_command(uint32_t command_code, float param1= 0.0f, float param2= 0.0f){
        // Helper to send the command to the PX4
        auto message = px4_msgs::msg::VehicleCommand();
        message.timestamp = this->get_clock()->now().nanoseconds() / 1000;
        message.command = command_code;
        message.param1 = param1;
        message.param2 = param2;
        message.target_system = 1;
        message.target_component = 1;
        message.source_system = 1;
        message.source_component = 1;
        message.from_external = true;
        vehicle_command_publisher_->publish(message);
    }

    void publish_trajectory_setpoint(float velocity_x, float velocity_y, float velocity_z, float z_hold){
       // Helper to send the setpoint information to the PX4 controller
       auto message = px4_msgs::msg::TrajectorySetpoint();
       message.position = {NAN, NAN, z_hold};
       message.velocity = {velocity_x, velocity_y, velocity_z};
       message.yaw = 0.0f;
       message.timestamp = this->get_clock()->now().nanoseconds() / 1000;
       trajectory_setpoint_publisher_->publish(message);
    }

    void publish_offboard_control_mode(){
        auto message = px4_msgs::msg::OffboardControlMode();
        message.position = true;
        message.velocity = true;
        message.timestamp = this->get_clock()->now().nanoseconds() / 1000;
        // Now publishing the message
        offboard_control_mode_publisher_->publish(message);
    }

    void manage_state_machine(){
        switch(flight_state_){
            case FlightState::IDLE:
                // Idle state requires nothing
            break;
            case FlightState::REQUESTING_OFFBOARD:
                // This state requires the counter to be incremented
                offboard_setpoint_counter_++;
                // Now checking if the status needs to check if its been a number of clicks
                if(offboard_setpoint_counter_ == offboard_setpoint_threshold_){
                    // Now that means that the state machine needs to send the command
                    publish_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_DO_SET_MODE,PX4_BASE_MODE_CUSTOM,PX4_CUSTOM_MODE_OFFBOARD);
                    RCLCPP_INFO(get_logger(), "Sent SET_MODE to offboard, waiting for confirmation.");
                    flight_state_ = FlightState::ARMING;

                }
            break;
            case FlightState::ARMING:
                if(nav_state_ == px4_msgs::msg::VehicleStatus::NAVIGATION_STATE_OFFBOARD && arming_state_ == px4_msgs::msg::VehicleStatus::ARMING_STATE_ARMED){
                    RCLCPP_INFO(get_logger(), "Offboard confirmed and drone is ARMED. Transitioning to TAKEOFF.");
                    target_z_ = -takeoff_altitude; // The FC NED (North East Down)
                    flight_state_ = FlightState::TAKEOFF;
                } else if(nav_state_ == px4_msgs::msg::VehicleStatus::NAVIGATION_STATE_OFFBOARD && arming_state_ != px4_msgs::msg::VehicleStatus::ARMING_STATE_ARMED){
                    RCLCPP_INFO(get_logger(), "Offboard confirmed. Sending ARM command.");
                    publish_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_COMPONENT_ARM_DISARM, 1.0f, 0.0f);
                } else {
                    RCLCPP_INFO(get_logger(), "Waiting for offboard mode. Resending SET_MODE.");
                    publish_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_DO_SET_MODE, PX4_BASE_MODE_CUSTOM, PX4_CUSTOM_MODE_OFFBOARD);
                }
            break;
            case FlightState::TAKEOFF:
                // This concerns with the drone while its taking off
                // The only concern is if it has reached the target altitude yet
                if(std::abs(current_z_) >= takeoff_altitude - position_threshold){
                    RCLCPP_INFO(get_logger(), "Drone reached the target takeoff altitude");
                    // Set the mode to flying
                    flight_state_ = FlightState::FLYING;
                    // Need to reset the PID's for when triggering landings and takeoffs
                    pid_distance_.reset();
                    pid_lateral_.reset();
                    command_vx_ = 0.0f;
                    command_vy_ = 0.0f;
                }
            break;
            case FlightState::FLYING:
                // This is the most important state and takes care of all the flying operations

                if(!link_ok_){
                    command_vx_ = 0.0f;
                    command_vy_ = 0.0f;
                    api_nudge_active_ = false;
                    pid_distance_.reset();
                    pid_lateral_.reset();
                    RCLCPP_INFO(get_logger(), "Drone lost link connection.");
                    break;
                }

                // API commands take precedence over gesture commands.
                // A nudge in progress holds its velocity for api_nudge_duration_ before
                // falling back to normal tracking/gesture behavior on a later tick.
                if(api_nudge_active_){
                    double elapsed = (this->get_clock()->now() - api_nudge_start_).seconds();
                    if(elapsed >= api_nudge_duration_){
                        api_nudge_active_ = false;
                        command_vx_ = 0.0f;
                        command_vy_ = 0.0f;
                        pid_distance_.reset();
                        pid_lateral_.reset();
                    } else {
                        break;
                    }
                }

                if(current_api_command_ == "LAND"){
                    current_api_command_ = "";
                    flight_state_ = FlightState::LANDING;
                    target_x_ = current_x_;
                    target_y_ = current_y_;
                    publish_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_LAND);
                    break;
                }

                if(current_api_command_ == "STOP" || current_api_command_ == "HOVER"){
                    current_api_command_ = "";
                    command_vx_ = 0.0f;
                    command_vy_ = 0.0f;
                    pid_distance_.reset();
                    pid_lateral_.reset();
                    break;
                }

                if(current_api_command_ == "MOVE_LEFT" || current_api_command_ == "MOVE_RIGHT" ||
                   current_api_command_ == "MOVE_FORWARD" || current_api_command_ == "MOVE_BACKWARD"){
                    if(current_api_command_ == "MOVE_LEFT")          { command_vy_ = -api_nudge_velocity_; command_vx_ = 0.0f; }
                    else if(current_api_command_ == "MOVE_RIGHT")    { command_vy_ =  api_nudge_velocity_; command_vx_ = 0.0f; }
                    else if(current_api_command_ == "MOVE_FORWARD")  { command_vx_ =  api_nudge_velocity_; command_vy_ = 0.0f; }
                    else if(current_api_command_ == "MOVE_BACKWARD") { command_vx_ = -api_nudge_velocity_; command_vy_ = 0.0f; }

                    current_api_command_ = "";
                    api_nudge_active_ = true;
                    api_nudge_start_ = this->get_clock()->now();
                    pid_distance_.reset();
                    pid_lateral_.reset();
                    break;
                }

                // TAKEOFF (no-op while already flying) or any other unrecognized value —
                // clear it so it doesn't linger and get rechecked forever.
                current_api_command_ = "";

                if(current_gesture_ == "LAND"){
                    current_gesture_ = "";
                    flight_state_ = FlightState::LANDING;
                    target_x_ = current_x_;
                    target_y_ = current_y_;
                    publish_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_LAND);
                    break;
                }

                if(current_gesture_ == "STOP" || current_gesture_ == "HOVER"){
                    command_vx_ = 0.0f;
                    command_vy_ = 0.0f;
                    pid_distance_.reset();
                    pid_lateral_.reset();
                    break;
                }

                // Handling the tracking boxes
                if(latest_tracks_.tracks.empty()){
                    // There is no person to follow
                    command_vx_ = 0.0;
                    command_vy_ = 0.0;
                    pid_distance_.reset();
                    pid_lateral_.reset();
                }else{
                    // There are tracks to handle
                    bool found = false;
                    skylark_interfaces::msg::Track best_track;
                    
                    if(locked_tracking_id_ != -1){
                        for(auto track: latest_tracks_.tracks){
                            if(track.tracking_id == locked_tracking_id_){
                                // No need for person class since the tracking id is isolated
                                best_track = track;
                                found = true;
                                break;
                            }
                        }
                    }
                   

                    // Now need to check if there is a best track found or not
                    if(!found){
                        command_vx_ = 0.0f;
                        command_vy_ = 0.0f;
                        pid_distance_.reset();
                        pid_lateral_.reset();
                    }else{
                        // There is a box found so I need to calculate the coordinates
                        std::vector<float> coordinate = compute_center_coordinates(best_track);

                        // Calculating the error of the coordinates
                        float error_x = coordinate[0] - 0.5f;
                        float error_height = coordinate[3] - target_bbox_height_ratio_;
                    
                        // Need to send this info to the PID
                        float velocity_y = pid_lateral_.compute(error_x);
                        float velocity_x = pid_distance_.compute(error_height);

                        // Adding clamping to stop extreme changes
                        velocity_x = std::clamp(velocity_x, -max_velocity_, max_velocity_);
                        velocity_y = std::clamp(velocity_y, -max_velocity_, max_velocity_);

                        RCLCPP_INFO(get_logger(), 
                        "FLYING: cx=%.3f bbox_h=%.3f err_x=%.3f err_dist=%.3f tx=%.3f ty=%.3f",
                            coordinate[0],      // center x in frame
                            coordinate[3],      // bbox height
                            error_x,            // lateral error
                            error_height,       // distance error
                            velocity_x,          // resulting velocity x
                            velocity_y           // resulting velocity y
                        );

                        // Assigning it
                        command_vx_ = velocity_x;
                        command_vy_ = velocity_y;
                        log_direction(command_vx_, command_vy_);
                    }
                    
                }
            break;
            case FlightState::LANDING:
                RCLCPP_INFO(get_logger(), "LANDING: current_z=%.2f threshold=%.2f", current_z_, -position_threshold);
                // NOTE: This is for SITL only since Gazebo has an issue with landing detection
                if(current_z_ >= -position_threshold){
                    publish_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_COMPONENT_ARM_DISARM, 0.0f, 21196.0f);
                }

                // This state only looks at the landing of the drone (opposite of takeoff)
                if(arming_state_ == px4_msgs::msg::VehicleStatus::ARMING_STATE_STANDBY){
                    // The drone disarms once it touches down by default because of PX4
                    flight_state_ = FlightState::DISARMED;
                }
            break;
            case FlightState::DISARMED:
                if(!disarm_logged_){
                    RCLCPP_INFO(get_logger(), "The drone is DISARMED.");
                    disarm_logged_ = true;
                }
            break;
            default:
                RCLCPP_INFO(get_logger(), "Wrong Flight State.");
        }
    }

    /* 
    * These are the services that are used by the user to trigger services on the control model.
    */
    void landing_trigger(const std::shared_ptr<std_srvs::srv::Trigger::Request>, std::shared_ptr<std_srvs::srv::Trigger::Response> response){
        // This is handling the service that will be sent by the user or me to have it land.
        // First checking if the drone is even in the correct state to land in
        if(flight_state_ == FlightState::FLYING){
            // If it is flying then only it can land
            RCLCPP_INFO(get_logger(), "Initiating Drone landing service.");
            // Setting the state to be landing
            flight_state_ = FlightState::LANDING; // The state machine will handle the landing action
            response->success = true;
            response->message = "Landing initiated.";

            target_x_ = current_x_;
            target_y_ = current_y_;
            publish_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_LAND);
        }else{
            RCLCPP_INFO(get_logger(), "Invalid land request");
            response->success = false;
            response->message = "Drone not in FLYING state.";
        }
    }

    void takeoff_trigger(const std::shared_ptr<std_srvs::srv::Trigger::Request>, std::shared_ptr<std_srvs::srv::Trigger::Response> response){
        // This handles the take off service that will be triggered by the user or me to have it land
        // Checking if the drone is disarmed
        if(flight_state_ == FlightState::DISARMED){
            RCLCPP_INFO(get_logger(), "Initiated Takeoff after landing.");
            offboard_setpoint_counter_ = 0;
            flight_state_ = FlightState::REQUESTING_OFFBOARD;
            response->success = true;
            response->message = "Initiated takeoff";
            disarm_logged_ = false;
        }else{
            RCLCPP_INFO(get_logger(), "Invalid takeoff request");
            response->success = false;
            response->message = "Drone not in DISARMED state.";
        }
    }

    void tracking_callback(const skylark_interfaces::msg::TrackArray::SharedPtr message){
        latest_tracks_ = *message;
    }

    void identity_callback(const std_msgs::msg::Int32::SharedPtr message){
        locked_tracking_id_ = message->data;
    }

    std::vector<float> compute_center_coordinates(const skylark_interfaces::msg::Track& track){
        float x1 = track.x1;
        float y1 = std::max(0.0f, track.y1);
        float x2 = track.x2;
        float y2 = std::min(1.0f, track.y2);

        // Calculating the center coordinates
        float cx = (x1 + x2) / 2.0f;
        float cy = (y1 + y2) / 2.0f;
        float w = x2 - x1;
        float h = y2 - y1;

        std::vector<float> coordinates(4);
        coordinates[0] = cx;
        coordinates[1] = cy;
        coordinates[2] = w;
        coordinates[3] = h;

        return coordinates;
    }

    void log_direction(float vx, float vy){
        std::string x_dir = "";
        std::string y_dir = "";

        if(vx > 0.05f) {
            x_dir = "FORWARD";
        }
        else if(vx < -0.05f){
            x_dir = "BACKWARD";
        }else{
            x_dir = "HOLDING";
        }                  

        if(vy > 0.05f){
            y_dir = "RIGHT";
        }
        else if(vy < -0.05f){
            y_dir = "LEFT";
        }else {
            y_dir = "HOLDING";
        }

        RCLCPP_INFO(get_logger(), "DIRECTION: x=%s(%.2f) y=%s(%.2f)", x_dir.c_str(), vx, y_dir.c_str(), vy);
    }
};

int main(int argc, char* argv[]){
    rclcpp::init(argc,argv);
    rclcpp::executors::MultiThreadedExecutor executor;
    auto node = std::make_shared<ControlNode>(rclcpp::NodeOptions());
    executor.add_node(node->get_node_base_interface());
    executor.spin();
    rclcpp::shutdown();
    return 0;
}