from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='skylark_video_sim',
            executable= 'video_publisher_node',
            parameters=[{'filename': '/mnt/d/dev/SkylarkOS/charles_test.mp4'}]
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
        
        # Node(
        #     package='skylark_control',
        #     executable='control_node'
        # ),
        
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
                'pose_model_path': '/mnt/d/dev/SkylarkOS/models/yolo11n-pose.onnx'
            }]
        )

    ])