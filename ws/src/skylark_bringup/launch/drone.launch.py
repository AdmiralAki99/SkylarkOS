import os
from launch import LaunchDescription
from launch_ros.actions import Node

# Hardware launch file for Jetson + Pixhawk deployment
# Models and config are volume-mounted into /skylark at runtime:
#   docker run -v /path/to/models:/skylark/models -v /path/to/config:/skylark/config ...

MODELS = '/skylark/models'
CONFIG = '/skylark/config'

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='skylark_perception',
            executable='perception_node',
            parameters=[{
                'model_path': os.path.join(MODELS, 'yolo11n.onnx'),
                'input_width': 640,
                'input_height': 640,
                'confidence_threshold': 0.5,
                'nms_threshold': 0.4,
                'detection_skip_frames': 2,
            }]
        ),

        Node(
            package='skylark_tracking',
            executable='tracking_node',
            parameters=[{
                'max_age': 5,
                'min_hits': 3,
                'iou_threshold': 0.3,
            }]
        ),

        Node(
            package='skylark_identity',
            executable='identity_node',
            parameters=[{
                'embedding_path': os.path.join(CONFIG, 'owner_embedding.npy'),
                'reid_embedding_path': os.path.join(CONFIG, 'reid_embedding.npy'),
                'reid_model_path': os.path.join(MODELS, 'osnet_x0_25.onnx'),
                'match_threshold': 0.45,
                'reid_match_threshold': 0.3,
            }]
        ),

        Node(
            package='skylark_gesture',
            executable='gesture_node',
            parameters=[{
                'pose_model_path': os.path.join(MODELS, 'yolo11n-pose.onnx'),
                'keypoint_confidence_threshold': 0.3,
                'gesture_debounce_count': 3,
            }]
        ),

        Node(
            package='skylark_streaming',
            executable='streaming_node',
        ),

        Node(
            package='skylark_control',
            executable='control_node',
            parameters=[{
                'takeoff_altitude': 1.5,
                'target_bbox_height_ratio': 0.4,
                'max_velocity': 1.0,
                'kp_lateral': 1.2,
                'kd_lateral': 0.1,
                'kp_distance': 1.0,
                'kd_distance': 0.1,
            }]
        ),
    ])