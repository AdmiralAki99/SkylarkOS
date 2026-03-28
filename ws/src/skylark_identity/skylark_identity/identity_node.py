import rclpy
from  rclpy.node import Node
from sensor_msgs.msg import Image
from std_msgs.msg import Int32
from skylark_interfaces.msg import TrackArray
from skylark_identity.face_engine import FaceEngine
from cv_bridge import CvBridge
import cv2 as cv

import os
import subprocess
import sys

STATE_SEARCHING = 'SEARCHING'
STATE_LOCKED = 'LOCKED'
STATE_LOST = 'LOST'

class IdentityNode(Node):
    def __init__(self):
        super().__init__('identity_node')
        
        self.state = STATE_SEARCHING
        self.locked_id = -1
        self.frame_counter = 0
        self.consecutive_hits = 0
        self.consecutive_misses = 0
        
        self.latest_frame = None
        
        self.declare_parameter('embedding_path','/app/data/owner_embedding.npy')
        
        self.embedding_path = self.get_parameter('embedding_path').get_parameter_value().string_value
        
        self.get_logger().info('Loaded Enrollment Info- Need to check if user is enrolled...')
        
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
        
        self.engine = FaceEngine(model_pack='buffalo_sc', embedding_path= self.embedding_path)
        
        self.get_logger().info('Initialized Face Engine - Need to check enrollment now...')
        
        if not self.engine.is_enrolled():
            # There is no user enrollment so the server needs to be deployed
            self.get_logger().info('Not enrolled — starting enrollment server...')
            server_path = os.path.join(os.path.dirname(__file__), 'enroll_server.py')
            self.enroll_process = subprocess.Popen([sys.executable, server_path])
            self.enroll_check_timer = self.create_timer(2.0, self.check_enrollment)
            
        self.get_logger().info('Initialized Face Engine Complete- User is enrolled with embeddings...')
        
        self.identity_publisher = self.create_publisher(Int32, '/identity/locked_track_id', 10)
        
        self.bridge = CvBridge()
        
    def image_callback(self, message):
        # Make an image out of the message
        image = self.bridge.imgmsg_to_cv2(message, 'bgr8')
            
        self.latest_frame = image
    
    def tracks_callback(self, tracks):
        # Guarding for latest frame not available
        if self.latest_frame is None:
            return
        
        # Guarding that the user is not enrolled
        if not self.engine.is_enrolled():
            return
        
        # Increment the frame counter so it doesnt run every frame causing computation overhead
        self.frame_counter = self.frame_counter + 1
        if self.frame_counter % 5 != 0:
            return
        
        if self.state == STATE_SEARCHING:
            for track in tracks.tracks:
                score = self.engine.detect_and_match(self.latest_frame, (track.x1, track.y1, track.x2, track.y2))
                if score > 0.45:
                    self.consecutive_hits = self.consecutive_hits + 1
                else:
                    self.consecutive_hits = 0
                
                if self.consecutive_hits >= 3:
                    self.locked_id = track.tracking_id
                    self.state = STATE_LOCKED
                    self.consecutive_hits = 0
                    break
                
        elif self.state == STATE_LOCKED:
            found_track = False
            for track in tracks.tracks:
                if track.tracking_id == self.locked_id:
                    found_track = True                
                    # There is a track that is locked
                    score = self.engine.detect_and_match(self.latest_frame, (track.x1, track.y1, track.x2, track.y2))
                    if score < 0.45:
                        self.consecutive_misses = self.consecutive_misses + 1
                    
                    if self.consecutive_misses >= 3:
                        # The state is lost
                        self.state = STATE_LOST
                        self.consecutive_misses = 0
                        break
            
            if not found_track:
                self.state = STATE_LOST
        else:
            # The state is lost
            for track in tracks.tracks:
                if self.locked_id == track.tracking_id:
                    self.state = STATE_LOCKED
                    break
            else:    
                self.state = STATE_SEARCHING
                self.locked_id = -1
                
        # Publishing the locked id for the face model
        msg = Int32()
        msg.data = self.locked_id
        self.identity_publisher.publish(msg)
        
    def check_enrollment(self):
        # Need to check if the user embeddings exist
        self.engine._load_embedding(self.embedding_path)
        if self.engine.is_enrolled():
            self.get_logger().info('Enrollment complete — shutting down enrollment server')
            self.enroll_process.terminate()
            self.enroll_process = None
            self.enroll_check_timer.cancel()
            self.enroll_check_timer = None
                
        
        

        
    
    
def main(args= None):
    rclpy.init(args=args)
    node = IdentityNode()
    rclpy.spin(node)
    rclpy.shutdown()