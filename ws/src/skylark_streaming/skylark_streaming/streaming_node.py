import threading
import rclpy
from  rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
import cv2 as cv
from http.server import BaseHTTPRequestHandler, HTTPServer

class StreamingNode(Node):
    def __init__(self):
        super().__init__('streaming_node')
        
        self._last_frame = None
        self._synchronization_lock = threading.Lock()
        self._bridge = CvBridge()
        self.subscriber = self.create_subscription(
            Image,
            '/detected_frames',
            self.image_callback,
            10
        )
        
        server = HTTPServer(('0.0.0.0',8080),MJPEGHandler)
        thread = threading.Thread(target= server.serve_forever)
        thread.daemon = True
        
        server.node = self
        
        thread.start()
        
    @property
    def synchronization_lock(self):
        return self._synchronization_lock
    
    def image_callback(self, message):
        # Make an image out of the message
        image = self._bridge.imgmsg_to_cv2(message, 'bgr8')
        
        # Encode to JPEG
        is_successful, img_encoded = cv.imencode('.jpg', image)
        
        # Writing the encoded jpeg string to bytes
        jpeg_bytes = img_encoded.tobytes()
        
        # Need to save the last frame without causing race conditions
        with self._synchronization_lock:
            self._last_frame = jpeg_bytes
            
        

class MJPEGHandler(BaseHTTPRequestHandler):
    def __init__(self, request, client_address, server):
        super().__init__(request, client_address, server)
        
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-Type','multipart/x-mixed-replace; boundary=frame')
        self.end_headers()
        
        # Acquire the loop
        # if no frame skip
        # write boundary values to file
        
        while True:
            with self.server.node.synchronization_lock:
                frame = self.server.node._last_frame
                
            if frame is None:
                continue
                
            try:
                # Writing the boundary value
                self.wfile.write(b'--frame\r\n')
                self.wfile.write(b'Content-Type: image/jpeg\r\n\r\n')
                self.wfile.write(frame)
                self.wfile.write(b'\r\n')
            except BrokenPipeError:
                break

def main(args= None):
    rclpy.init(args=args)
    node = StreamingNode()
    rclpy.spin(node)
    rclpy.shutdown()
    
    