#include <chrono>
#include <memory>
#include <string>
#include <algorithm>

#include <opencv2/opencv.hpp>
#include <opencv2/video/tracking.hpp>

class KalmanTrack{
    public:
        int time_since_last_update_;
        int hits_;
        int tracking_id_;
        int class_id_ = 0;
        std::string label_ = "";
        float confidence_ = 0.0;

        KalmanTrack(cv::Rect2f detection);
        
        cv::Rect2f predict();

        void update(cv::Rect2f bounding_box, int class_id, const std::string& label, float confidence);
        void initialize_transition_matrix();
        void initialize_measurement_matrix();

        cv::Rect2f get_current_state_bounding_box();

    private:
        cv::KalmanFilter kalman_filter_;
        int age_;
        static int next_id_;

};

/**
 *  [1 0 0 0 1 0 0 0]   x  = x  + vx
    [0 1 0 0 0 1 0 0]   y  = y  + vy
    [0 0 1 0 0 0 1 0]   w  = w  + vw
    [0 0 0 1 0 0 0 1]   h  = h  + vh
    [0 0 0 0 1 0 0 0]   vx = vx
    [0 0 0 0 0 1 0 0]   vy = vy
    [0 0 0 0 0 0 1 0]   vw = vw
    [0 0 0 0 0 0 0 1]   vh = vh
 
 * 
 * 
 */
