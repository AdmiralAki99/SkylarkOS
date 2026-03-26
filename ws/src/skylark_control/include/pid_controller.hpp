#pragma once

class PIDController{

    public:

        PIDController();

        PIDController(float kp, float kd, float dt);

        float compute(float error);

        void reset();
    
    private:
        float kp_;
        float kd_;
        float prev_error_;
        float dt_;

};