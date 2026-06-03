import threading
import numpy
import rclpy
from  rclpy.node import Node
from sensor_msgs.msg import CompressedImage
from std_msgs.msg import Int32, Float32MultiArray
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy
from skylark_interfaces.msg import TrackArray
from cv_bridge import CvBridge
import cv2 as cv
from http.server import BaseHTTPRequestHandler, HTTPServer

class StreamingNode(Node):
    def __init__(self):
        super().__init__('streaming_node')
        
        self._lock = threading.Lock()
        self._bridge = CvBridge()
        self._tracks = []
        self._locked_id = -1
        self._keypoints = None
        
        self._raw_frame = None
        self._raw_event = threading.Event()
        self._jpeg_frame = None
        self._jpeg_event = threading.Event()
        
        qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            history=HistoryPolicy.KEEP_LAST,
            depth=1
        )
        
        self.subscriber = self.create_subscription(
            CompressedImage,
            '/camera/image_compressed',
            self.image_callback,
            qos
        )
        
        self.tracker_subscriber = self.create_subscription(
            TrackArray,
            '/tracking/tracks',
            self.tracks_callback,
            qos
        )
        
        self.identity_subscriber = self.create_subscription(
            Int32,
            '/identity/locked_track_id',
            self.identity_callback,
            qos
        )
        
        # self.skeleton_subscriber = self.create_subscription(
        #     Float32MultiArray,
        #     '/gesture/keypoints',
        #     self.keypoints_callback,
        #     qos
        # )
        
        encode_thread = threading.Thread(target=self._encode_loop)
        encode_thread.daemon = True
        encode_thread.start()
        
        self.get_logger().info('Created server started on port 8080')
        
        server = HTTPServer(('0.0.0.0',8080),MJPEGHandler)
        thread = threading.Thread(target= server.serve_forever)
        thread.daemon = True
        
        server.node = self
        
        thread.start()
        
        self.get_logger().info('Streaming started on port 8080')
    
    def image_callback(self, message):
        image = cv.imdecode(numpy.frombuffer(message.data, numpy.uint8), cv.IMREAD_COLOR)
        with self._lock:
            self._raw_frame = image
        self._raw_event.set()
            
    def tracks_callback(self, message):
        with self._lock:
            self._tracks = message.tracks
            
    def identity_callback(self, message):
        with self._lock:
            self._locked_id = message.data
            
    def keypoints_callback(self, message):
        with self._lock:
            self._keypoints = message.data
        
    def _encode_loop(self):
        while True:
            self._raw_event.wait()
            with self._lock:
                frame = self._raw_frame
                tracks = self._tracks
                locked_id = self._locked_id
            # keypoints = self._keypoints
            self._raw_event.clear()
        
            if frame is None:
                continue
        
            image = frame.copy()
            H,W = image.shape[:2]
            if tracks is not None:
                # Need to iterate over the tracks and get the coordinates
                for track in tracks:                
                    # Creating the rectangle
                    x1 = int(track.x1 * W) 
                    y1 = int(track.y1 * H)
                    x2 = int(track.x2 * W)
                    y2 = int(track.y2 * H)
                    
                    if track.tracking_id == locked_id:
                        color = (255,100,0)
                        label = f"OWNER [{track.tracking_id}]"
                    else:
                        color = (0,255,0)
                        label = f"ID: {track.tracking_id}"
                
                    cv.rectangle(image, (x1,y1), (x2,y2), color, 2)
                    cv.putText(image, label,(x1, y1 - 5), cv.FONT_HERSHEY_SIMPLEX, 0.5, color, 1)
        
            _, buffer = cv.imencode('.jpg', image, [cv.IMWRITE_JPEG_QUALITY,60])
            with self._lock:
                self._jpeg_frame = buffer.tobytes()
            self._jpeg_event.set()
        

class MJPEGHandler(BaseHTTPRequestHandler):
    def __init__(self, request, client_address, server):
        super().__init__(request, client_address, server)
        
    def log_message(self, format, *args):
        pass
        
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-Type','multipart/x-mixed-replace; boundary=frame')
        self.end_headers()
        
        # Acquire the loop
        # if no frame skip
        # write boundary values to file
        
        self.server.node.get_logger().info(f'Client connected: {self.client_address}')
        
        while True:
            self.server.node._jpeg_event.wait()
            with self.server.node._lock:
                _jpeg_frame = self.server.node._jpeg_frame
            self.server.node._jpeg_event.clear()
            
            if _jpeg_frame is None:
                continue
            
            try:
                # Writing the boundary value
                self.wfile.write(b'--frame\r\n')
                self.wfile.write(b'Content-Type: image/jpeg\r\n\r\n')
                self.wfile.write(_jpeg_frame)
                self.wfile.write(b'\r\n')
                self.wfile.flush()
            except BrokenPipeError:
                self.server.node.get_logger().info(f'Client disconnected: {self.client_address}')
                break
        

def main(args= None):
    rclpy.init(args=args)
    node = StreamingNode()
    rclpy.spin(node)
    rclpy.shutdown()
    
    