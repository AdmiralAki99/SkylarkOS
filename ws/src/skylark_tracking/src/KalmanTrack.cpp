#include "KalmanTrack.hpp"

int KalmanTrack::next_id_ = 0;

KalmanTrack::KalmanTrack(cv::Rect2f detection){
            
    this->age_ = 1;
    this->time_since_last_update_ = 0;
    this->hits_ = 1;
    this->tracking_id_ = next_id_++;

    kalman_filter_ = cv::KalmanFilter(8,4,0);

    // Initializing the filters
    initialize_transition_matrix();
    initialize_measurement_matrix();

    // Initializing the covariance matrices
    cv::setIdentity(kalman_filter_.processNoiseCov, cv::Scalar(1e-2));
    cv::setIdentity(kalman_filter_.measurementNoiseCov, cv::Scalar(1e-1));
    cv::setIdentity(kalman_filter_.errorCovPost, cv::Scalar(1.0));

    // Converting detection to center format
    float x1 = detection.x;
    float y1 = detection.y;
    float x2 = detection.x + detection.width;
    float y2 = detection.y + detection.height;

    // Calculating the center format
    float cx = (x1 + x2) / 2;
    float cy = (y1 + y2) / 2;
    float w = x2 - x1;
    float h = y2 - y1;

    // Initializing the state post
    kalman_filter_.statePost.at<float>(0) = cx;
    kalman_filter_.statePost.at<float>(1) = cy;
    kalman_filter_.statePost.at<float>(2) = w;
    kalman_filter_.statePost.at<float>(3) = h;
}

cv::Rect2f KalmanTrack::predict(){
    cv::Mat predictedState = kalman_filter_.predict();

    // Now the values for the time status need to be updated
    age_++;
    time_since_last_update_++;

    // Reading the predicted position
    float cx = predictedState.at<float>(0);
    float cy = predictedState.at<float>(1);
    float w = predictedState.at<float>(2);
    float h = predictedState.at<float>(3);

    // Convert the coordinates into xy-coordinate system
    float x1 = cx - w/2;
    float y1 = cy - h/2;

    return cv::Rect2f(x1,y1,w,h);
}

void KalmanTrack::update(cv::Rect2f bounding_box, int class_id, const std::string& label, float confidence){
    // The bounding box will be in the form of x1,y1,x2,y2

    float x1 = bounding_box.x;
    float y1 = bounding_box.y;
    float x2 = bounding_box.x + bounding_box.width;
    float y2 = bounding_box.y + bounding_box.height;

    // Converting it back to center format for the filter
    float cx = (x1 + x2) / 2.0f;
    float cy = (y1 + y2) / 2.0f;
    float w = x2 - x1;
    float h = y2 - y1;

    // Creating the correction matrix
    cv::Mat measurementMatrix = cv::Mat::zeros(4,1,CV_32F);

    measurementMatrix.at<float>(0) = cx;
    measurementMatrix.at<float>(1) = cy;
    measurementMatrix.at<float>(2) = w;
    measurementMatrix.at<float>(3) = h;

    // Calling the correct function from the filter
    kalman_filter_.correct(measurementMatrix);

    // Updating the time status values
    hits_ ++;
    time_since_last_update_ = 0;

    // Storing the class info
    class_id_ = class_id;
    label_ = label;
    confidence_ = confidence;
}

void KalmanTrack::initialize_transition_matrix(){
    cv::setIdentity(kalman_filter_.transitionMatrix);

    // Setting the values for the matrix
    kalman_filter_.transitionMatrix.at<float>(0,4) = 1.0f;
    kalman_filter_.transitionMatrix.at<float>(1,5) = 1.0f;
    kalman_filter_.transitionMatrix.at<float>(2,6) = 1.0f;
    kalman_filter_.transitionMatrix.at<float>(3,7) = 1.0f;
}

void KalmanTrack::initialize_measurement_matrix(){
    cv::setIdentity(kalman_filter_.measurementMatrix);
}

cv::Rect2f KalmanTrack::get_current_state_bounding_box(){

    float cx = kalman_filter_.statePost.at<float>(0);
    float cy = kalman_filter_.statePost.at<float>(1);
    float w = kalman_filter_.statePost.at<float>(2);
    float h = kalman_filter_.statePost.at<float>(3);

    float x1 = cx - w/2.0f;
    float y1 = cy - h/2.0f;

    return cv::Rect2f(x1,y1,w,h);
}