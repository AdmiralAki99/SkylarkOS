import threading
import rclpy
from  rclpy.node import Node
from sensor_msgs.msg import Image
from skylark_interfaces.msg import TrackArray
from cv_bridge import CvBridge
import cv2 as cv
from http.server import BaseHTTPRequestHandler, HTTPServer

class StreamingNode(Node):
    def __init__(self):
        super().__init__('streaming_node')
        
        self._last_frame = None
        self._latest_tracks = None
        self._synchronization_lock = threading.Lock()
        self._bridge = CvBridge()
        self.subscriber = self.create_subscription(
            Image,
            '/camera/image_raw',
            self.image_callback,
            10
        )
        
        self.tracker_subscriber = self.create_subscription(
            TrackArray,
            '/tracking/tracks',
            self.tracks_callback,
            10
        )
        
        self.get_logger().info('Created server started on port 8080')
        
        server = HTTPServer(('0.0.0.0',8080),MJPEGHandler)
        thread = threading.Thread(target= server.serve_forever)
        thread.daemon = True
        
        server.node = self
        
        thread.start()
        
        self.get_logger().info('Streaming started on port 8080')
        
        self._frame = 0
        
    @property
    def synchronization_lock(self):
        return self._synchronization_lock
    
    def image_callback(self, message):
        # Make an image out of the message
        image = self._bridge.imgmsg_to_cv2(message, 'bgr8')
        
        H,W = image.shape[:2]
        # Read the latest tracks
        with self._synchronization_lock:
            latest_tracks = self._latest_tracks
            
        if latest_tracks is not None and latest_tracks.tracks:
            # Need to iterate over the tracks and get the coordinates
            for track in latest_tracks.tracks:                
                # Creating the rectangle
                x1 = int(track.x1 * W) 
                y1 = int(track.y1 * H)
                x2 = int(track.x2 * W)
                y2 = int(track.y2 * H)
                
                cv.rectangle(image, (x1,y1), (x2,y2), (0,255,0), 2)
                cv.putText(image, f"ID:{track.tracking_id} {track.class_id}",(x1, y1 - 5), cv.FONT_HERSHEY_SIMPLEX, 0.5, (0,255,0), 1)
                
     
        # Encode to JPEG
        is_successful, img_encoded = cv.imencode('.jpg', image)
        
        self.get_logger().info(f'JPEG encoding status: {"SUCCESS" if is_successful else "FAILED"}')
        
        # Writing the encoded jpeg string to bytes
        jpeg_bytes = img_encoded.tobytes()
                  
        # Need to save the last frame without causing race conditions
        self._frame = self._frame + 1
        # if self._frame % 30 == 0:
        #     self.get_logger().info(f'Streaming — frames received: {self._frame}')
        with self._synchronization_lock:
            self._last_frame = jpeg_bytes
            
    def tracks_callback(self, message):
        with self._synchronization_lock:
            self._latest_tracks = message
            
        

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
                self.server.node.get_logger().info(f'Client disconnected: {self.client_address}')
                break

def main(args= None):
    rclpy.init(args=args)
    node = StreamingNode()
    rclpy.spin(node)
    rclpy.shutdown()
    
    