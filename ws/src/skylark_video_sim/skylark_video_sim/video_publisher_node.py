import rclpy
from  rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
import cv2 as cv

class VideoPublisherNode(Node):
    def __init__(self):
        super().__init__('video_publisher_node')
        
        self._desired_frame_rate = 1/30
        
        self.declare_parameter('filename', '')
        filename = self.get_parameter('filename').get_parameter_value().string_value
        
        self.get_logger().info(f'Opening video: {filename}')
        
        if not filename:
            self.get_logger().error('No filename provided')
            return
        
        self._capture = cv.VideoCapture(filename)
        if not self._capture.isOpened():
            self.get_logger().error(f'Failed to open video: {filename}')
            return
        
        # Now bridging the OpenCV to ROS2
        self._bridge = CvBridge()
        self._publisher = self.create_publisher(Image, '/camera/image_raw', 10)
        
        self.timer = self.create_timer(self._desired_frame_rate, self.timer_callback)
        
        
    def timer_callback(self):
        
        is_successful, frame = self._capture.read()
            
        if not is_successful:
            self.get_logger().info(f'Video capture status: {"SUCCESS" if is_successful else "FAILED"}')
            self._capture.set(cv.CAP_PROP_POS_FRAMES,0)
            return
            
        msg = self._bridge.cv2_to_imgmsg(frame,'bgr8')
            
        self._publisher.publish(msg)
            
            
            
            
            
def main(args= None):
    rclpy.init(args=args)
    node = VideoPublisherNode()
    rclpy.spin(node)
    rclpy.shutdown()
        
        