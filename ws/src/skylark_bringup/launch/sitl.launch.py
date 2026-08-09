from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess, TimerAction

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='skylark_video_sim',
            executable= 'video_publisher_node',
            parameters=[{'filename': '/mnt/d/dev/SkylarkOS/ryan_gesture_test.mp4'}]
            # parameters=[{'filename': '0'}]
        ),

        Node(
            package='skylark_perception',
            executable='perception_node',
            parameters=[{'model_path':'/mnt/d/dev/SkylarkOS/models/model.onnx'}]
        ),
        
        Node(
            package='skylark_tracking',
            executable='tracking_node'
        ),
        
        Node(
            package='skylark_streaming',
            executable='streaming_node'
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
        
        Node(
            package='skylark_identity',
            executable='identity_node',
            parameters=[
                {
                    'embedding_path':'/mnt/d/dev/SkylarkOS/data/owner_embedding.npy',
                    'reid_embedding_path': '/mnt/d/dev/SkylarkOS/data/owner_reid_embedding.npy',
                    'match_threshold': 0.45,
                    'reid_match_threshold': 0.3,
                    'reid_model_path': '/mnt/d/dev/SkylarkOS/models/osnet_x0_25.onnx'
                }]
        ),
        
        Node(
            package='skylark_gesture',
            executable='gesture_node',
            parameters=[{
                'pose_model_path': '/mnt/d/dev/SkylarkOS/models/yolo11n-pose.onnx',
                'keypoint_confidence_threshold': 0.3
            }]
        ),
        Node(
            package='skylark_telemetry',
            executable='telemetry_node',
        ),
        
        TimerAction(
            period=75.0,
            actions=[
                ExecuteProcess(
                    cmd=['ros2', 'lifecycle', 'set', '/identity_node', 'configure'],
                    output='screen'
                )
            ]
        ),

        TimerAction(
            period=85.0,
            actions=[
                ExecuteProcess(
                    cmd=['ros2', 'lifecycle', 'set', '/identity_node', 'activate'],
                    output='screen'
                )
            ]
        ),
        TimerAction(
            period=40.0,
            actions=[
                ExecuteProcess(
                    cmd=['ros2', 'lifecycle', 'set', '/perception_node', 'configure'],
                    output='screen'
                )
            ]
        ),
        TimerAction(
            period=50.0,
            actions=[
                ExecuteProcess(
                    cmd=['ros2', 'lifecycle', 'set', '/perception_node', 'activate'],
                    output='screen'
                )
            ]
        ),
        TimerAction(
            period=55.0,
            actions=[
                ExecuteProcess(
                    cmd=['ros2', 'lifecycle', 'set', '/tracking_node', 'configure'],
                    output='screen'
                )
            ]
        ),

        TimerAction(
            period=60.0,
            actions=[
                ExecuteProcess(
                    cmd=['ros2', 'lifecycle', 'set', '/tracking_node', 'activate'],
                    output='screen'
                )
            ]
        ),
        TimerAction(
            period=65.0,
            actions=[
                ExecuteProcess(
                    cmd=['ros2', 'lifecycle', 'set', '/control_node', 'configure'],
                    output='screen'
                )
            ]
        ),
        TimerAction(
            period=70.0,
            actions=[
                ExecuteProcess(
                    cmd=['ros2', 'lifecycle', 'set', '/control_node', 'activate'],
                    output='screen'
                )
            ]
        ),
    ])