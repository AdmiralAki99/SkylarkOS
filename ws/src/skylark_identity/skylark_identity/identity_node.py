import rclpy
from rclpy.lifecycle import State, TransitionCallbackReturn, Node
from sensor_msgs.msg import Image
from std_msgs.msg import Int32
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy
from skylark_interfaces.msg import TrackArray
from skylark_identity.face_engine import FaceEngine
from skylark_identity.reid_engine import ReIDEngine
from cv_bridge import CvBridge

STATE_SEARCHING = 'SEARCHING'
STATE_LOCKED = 'LOCKED'
STATE_LOST = 'LOST'
STATE_ENROLLING_REID = 'ENROLLING_REID'

class IdentityNode(Node):
    def __init__(self):
        super().__init__('identity_node')
        
    def on_configure(self, state: State):
        
        self.state = STATE_SEARCHING
        self.locked_id = -1
        self.frame_counter = 0
        self.consecutive_hits = 0
        self.consecutive_misses = 0
        
        self.reid_crops = []
        self.reid_enroll_target = 10
            
        self.latest_frame = None
        
        self.declare_parameter('embedding_path','/app/data/owner_embedding.npy')
        self.declare_parameter('reid_embedding_path','/app/data/owner_reid_embedding.npy')
        self.declare_parameter('reid_model_path','/app/models/osnet_x0_25.onnx')
        self.declare_parameter('match_threshold', 0.45)
        self.declare_parameter('reid_match_threshold', 0.3)
        
        self.embedding_path = self.get_parameter('embedding_path').get_parameter_value().string_value
        self.reid_embedding_path = self.get_parameter('reid_embedding_path').get_parameter_value().string_value
        self.match_threshold_ = self.get_parameter('match_threshold').get_parameter_value().double_value
        self.reid_match_threshold_ = self.get_parameter('reid_match_threshold').get_parameter_value().double_value
        self.reid_model_path = self.get_parameter('reid_model_path').get_parameter_value().string_value
        
        self.get_logger().info('Loaded Enrollment Info- Need to check if user is enrolled...')
             
        self.engine = FaceEngine(model_pack='buffalo_sc', embedding_path= self.embedding_path)
        
        self.reid_engine = ReIDEngine(model_path=self.reid_model_path, embedding_path=self.reid_embedding_path)
        
        self.get_logger().info('Initialized Face Engine - Need to check enrollment now...')
        
        # Checking if the person is enrolled
        if not self.engine.is_enrolled():
            # There is no user enrollment so the server needs to be deployed
            self.get_logger().info("User is not enrolled...")
            return TransitionCallbackReturn.FAILURE
            
        self.get_logger().info('Face Engine Initialized- User is enrolled with embeddings...')
        
        self.identity_publisher = self.create_publisher(Int32, '/identity/locked_track_id', 10)
        
        self.bridge = CvBridge()
        return TransitionCallbackReturn.SUCCESS
    
    def on_activate(self, state: State):
        qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            history=HistoryPolicy.KEEP_LAST,
            depth=1
        )
        
        self.image_subscriber = self.create_subscription(
            Image,
            "/camera/image_raw",
            self.image_callback,
            qos
        )
        
        self.get_logger().info('Created Subscriber for raw camera footage...')
        
        self.tracks_subscriber = self.create_subscription(
            TrackArray,
            '/tracking/tracks',
            self.tracks_callback,
            10
        )
        
        self.get_logger().info('Created Subscriber for SORT tracks...')
        
        return TransitionCallbackReturn.SUCCESS
    
    def on_deactivate(self, state: State):
        
        self.destroy_subscription(self.image_subscriber)
        self.destroy_subscription(self.tracks_subscriber)
        
        self.image_subscriber = None
        self.tracks_subscriber = None
        self.latest_frame = None
        self.get_logger().info('Destroyed the Subscribers and local variables...')
        return TransitionCallbackReturn.SUCCESS
    
    def on_cleanup(self, state: State):
        self.destroy_publisher(self.identity_publisher)
        
        self.engine = None
        self.reid_engine = None
        
        self.get_logger().info('Cleaned up initialized Face and ReID engines...')
        
        return TransitionCallbackReturn.SUCCESS
    
    def on_shutdown(self, state: State):
        return TransitionCallbackReturn.SUCCESS
        
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
        if self.frame_counter % 5 != 0 and self.state != STATE_ENROLLING_REID:
            return
        
        if self.state == STATE_SEARCHING:
            for track in tracks.tracks:
                score = self.engine.detect_and_match(self.latest_frame, (track.x1, track.y1, track.x2, track.y2))
                if score > self.match_threshold_:
                    self.consecutive_hits = self.consecutive_hits + 1
                else:
                    self.consecutive_hits = 0
                
                if self.consecutive_hits >= 3:
                    self.locked_id = track.tracking_id
                    self.state = STATE_ENROLLING_REID
                    self.consecutive_hits = 0
                    break
                
        elif self.state == STATE_ENROLLING_REID:
            found_track = False
            for track in tracks.tracks:
                if self.locked_id == track.tracking_id:
                    found_track = True
                    self.reid_crops.append((self.latest_frame, (track.x1, track.y1, track.x2, track.y2)))
                    if len(self.reid_crops) >= self.reid_enroll_target:
                        self.reid_engine.enroll(self.reid_crops)
                        self.reid_crops = []
                        self.state = STATE_LOCKED
                    break
            if not found_track:
                self.state = STATE_SEARCHING
                self.reid_crops = []
                self.locked_id = -1
                        
        elif self.state == STATE_LOCKED:
            found_track = False
            for track in tracks.tracks:
                if track.tracking_id == self.locked_id:
                    found_track = True                
                    # There is a track that is locked
                    score = self.reid_engine.match(self.latest_frame, (track.x1, track.y1, track.x2, track.y2))
                    if score < self.reid_match_threshold_:
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
                   
def main(args= None):
    rclpy.init(args=args)
    node = IdentityNode()
    executor = rclpy.executors.SingleThreadedExecutor()
    executor.add_node(node)
    try:
        executor.spin()
    finally:
        executor.shutdown()
        node.destroy_node()
        rclpy.shutdown()