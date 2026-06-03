import rclpy
from  rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
import cv2 as cv
import time
import threading

from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy

class VideoPublisherNode(Node):
    def __init__(self):
        super().__init__('video_publisher_node')
        
        qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            history=HistoryPolicy.KEEP_LAST,
            depth=1
        )
        
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
        
        self._frames = []
        while True:
            ok, frame = self._capture.read()
            if not ok:
                break
            self._frames.append(frame)
        
        fps = self._capture.get(cv.CAP_PROP_FPS) or 30.0
        self.get_logger().info(f'Video FPS: {fps}')
        self._capture.release()
        self._index = 0
        self.get_logger().info(f'Loaded {len(self._frames)} frames into memory')

        # Now bridging the OpenCV to ROS2
        self._bridge = CvBridge()
        self._publisher = self.create_publisher(Image, '/camera/image_raw', qos)
        
        self._messages = []
        for frame in self._frames:
            self._messages.append(self._bridge.cv2_to_imgmsg(frame, 'bgr8'))
        self._frames = None  # free the raw frame memory
        self.get_logger().info(f'Pre-built {len(self._messages)} messages')

        thread = threading.Thread(target=self._publish_loop)
        thread.daemon = True
        thread.start()
        
    def _publish_loop(self):
        interval = 1/60
        while True:
            t_start = time.time()
            self.timer_callback()
            elapsed = time.time() - t_start
            time.sleep(max(0, interval - elapsed))
    def timer_callback(self):
        msg = self._messages[self._index % len(self._messages)]
        self._index += 1
        self._publisher.publish(msg)
            
            
            
            
            
def main(args= None):
    rclpy.init(args=args)
    node = VideoPublisherNode()
    rclpy.spin(node)
    rclpy.shutdown()
        
        