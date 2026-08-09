import sys
import threading
import cv2
import numpy as np


class LatestFrameReader:
    """Continuously reads from a capture source, keeping only the most recent frame.

    cv2's FFmpeg backend ignores CAP_PROP_BUFFERSIZE, so a slow consumer falls
    behind an unbounded internal queue and lag grows over time. This drains the
    source as fast as it produces frames and discards anything not yet consumed.
    """

    def __init__(self, source, backend):
        self.cap = cv2.VideoCapture(source, backend)
        self.lock = threading.Lock()
        self.frame = None
        self.ret = False
        self.running = True
        self.thread = threading.Thread(target=self._update, daemon=True)
        self.thread.start()

    def _update(self):
        while self.running:
            ret, frame = self.cap.read()
            with self.lock:
                self.ret, self.frame = ret, frame
            if not ret:
                break

    def read(self):
        with self.lock:
            return self.ret, self.frame

    def release(self):
        self.running = False
        self.thread.join()
        self.cap.release()

sys.path.insert(0, 'ws/src/skylark_gesture')
from skylark_gesture.pose_engine import PoseEngine
from skylark_gesture.gesture_engine import GestureEngine

COCO_SKELETON = [
    (0, 1), (0, 2), (1, 3), (2, 4),       # face
    (5, 6),                                 # shoulders
    (5, 7), (7, 9), (6, 8), (8, 10),       # arms
    (5, 11), (6, 12),                       # torso
    (11, 12), (11, 13), (13, 15),           # left leg
    (12, 14), (14, 16),                     # right leg
]

MODEL_PATH = sys.argv[1] if len(sys.argv) > 1 else 'models/yolo11n-pose.onnx'

engine = PoseEngine(MODEL_PATH)
gesture = GestureEngine(keypoint_confidence_threshold=0.3)
cap = LatestFrameReader("udp://0.0.0.0:5000", cv2.CAP_FFMPEG)

print("Press Q to quit")

keypoints = None
frame_count = 0
INFER_EVERY = 5

while True:
    ret, frame = cap.read()
    if frame is None:
        continue
    if not ret:
        break

    if frame_count % INFER_EVERY == 0:
        full_frame_box = [0.0, 0.0, 1.0, 1.0]
        keypoints = engine.extract_keypoints(frame, full_frame_box)
    frame_count += 1

    if keypoints is not None:
        for x, y, conf in keypoints:
            if conf > 0.3:
                cv2.circle(frame, (int(x), int(y)), 4, (0, 255, 0), -1)

        for a, b in COCO_SKELETON:
            xa, ya, ca = keypoints[a]
            xb, yb, cb = keypoints[b]
            if ca > 0.3 and cb > 0.3:
                cv2.line(frame, (int(xa), int(ya)), (int(xb), int(yb)), (0, 200, 255), 2)

        command = gesture.detect(keypoints)
        label = command if command else 'NONE'
        cv2.putText(frame, label, (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 255, 255), 2)

    cv2.imshow('Pose Test', frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
