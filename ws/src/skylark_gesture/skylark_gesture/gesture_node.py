import rclpy
from  rclpy.node import Node
from sensor_msgs.msg import Image
from std_msgs.msg import Int32, String, Float32MultiArray
from skylark_interfaces.msg import TrackArray
from skylark_gesture.gesture_engine import GestureEngine
from skylark_gesture.pose_engine import PoseEngine
from cv_bridge import CvBridge

class GestureNode(Node):
    def __init__(self):
        super().__init__('gesture_node')
        
        self.declare_parameter('pose_model_path','/app/models/yolo11n-pose.onnx')
        self.declare_parameter('keypoint_confidence_threshold', 0.5)
        self.declare_parameter('gesture_debounce_count', 3)
        
        self.pose_model_path = self.get_parameter('pose_model_path').get_parameter_value().string_value
        self.keypoint_confidence_threshold = self.get_parameter('keypoint_confidence_threshold').get_parameter_value().double_value
        self.gesture_debounce_count = self.get_parameter('gesture_debounce_count').get_parameter_value().integer_value
        
        self.pose_engine = PoseEngine(model_path= self.pose_model_path)
        self.gesture_engine = GestureEngine(keypoint_confidence_threshold= self.keypoint_confidence_threshold)
        
        self.latest_frame = None
        self.locked_id = -1
        self.latest_tracks = None
        self.frame_counter = 0
        
        self.last_gesture = None
        self.gesture_streak = 0
        
        self.bridge = CvBridge()
        
        self.image_subscriber = self.create_subscription(
            Image,
            "/camera/image_raw",
            self.image_callback,
            10
        )
        
        self.get_logger().info('Created Subscriber for raw camera footage...')
        
        self.tracks_subscriber = self.create_subscription(
            TrackArray,
            '/tracking/tracks',
            self.tracks_callback,
            10
        )
        
        self.get_logger().info('Created Subscriber for SORT tracks...')
        
        self.locked_tracking_id_subscriber = self.create_subscription(
            Int32,
            '/identity/locked_track_id',
            self.identity_callback,
            10
        )
        
        self.get_logger().info('Created Subscriber for Locked ID...')
        
        self.gesture_publisher = self.create_publisher(String, '/gesture/command', 10)
        self.skeleton_publisher = self.create_publisher(Float32MultiArray, '/gesture/keypoints', 10)
        
    def image_callback(self, message):
        image = self.bridge.imgmsg_to_cv2(message, 'bgr8')
            
        self.latest_frame = image
    
    def tracks_callback(self,tracks):
        if self.latest_frame is None:
            return
        
        if self.locked_id == -1:
            return
        
        self.frame_counter = self.frame_counter + 1
        if self.frame_counter % 3 != 0:
            return
        
        for track in tracks.tracks:
            if self.locked_id == track.tracking_id:
                # Calling the pose engine 
                keypoints = self.pose_engine.extract_keypoints(self.latest_frame, (track.x1, track.y1, track.x2, track.y2))
                
                if keypoints is not None:
                    keypoint_message = Float32MultiArray()
                    keypoint_message.data = keypoints.flatten().tolist()
                    self.skeleton_publisher.publish(keypoint_message)
                
                gesture = self.gesture_engine.detect(keypoints= keypoints)
                
                if gesture == self.last_gesture:
                    self.gesture_streak = self.gesture_streak + 1
                else:
                    self.gesture_streak = 1
                    self.last_gesture = gesture
                
                # self.get_logger().info(f'Detected {gesture} gesture...')
        
                if self.gesture_streak == self.gesture_debounce_count and gesture is not None:
                    msg = String()
                    msg.data = gesture
                    self.gesture_publisher.publish(msg)
    
    def identity_callback(self, message):
        self.locked_id = message.data
        
def main(args= None):
    rclpy.init(args=args)
    node = GestureNode()
    rclpy.spin(node)
    rclpy.shutdown()