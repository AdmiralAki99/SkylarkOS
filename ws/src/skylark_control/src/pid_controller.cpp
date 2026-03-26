#include "pid_controller.hpp"

PIDController::PIDController():kp_(0.0f), kd_(0.0f), dt_(0.0f), prev_error_(0.0f){

}

PIDController::PIDController(float kp, float kd, float dt){
    kp_ = kp;
    kd_ = kd;
    dt_ = dt;
    prev_error_ = 0.0f;
}

float PIDController::compute(float error){
    // Calculating the PID computation
    float p_output = kp_ * error;
    float rate_of_change = (error - error) / dt_;
    float d_output = kd_ * rate_of_change;

    float velocity_command = p_output + d_output;

    // Updating the error
    prev_error_ = error;

    return velocity_command;
}

void PIDController::reset(){
    // Resetting the error calculation
    prev_error_ = 0.0f;
}