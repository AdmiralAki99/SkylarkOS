# SkylarkOS

> ROS2-based autonomous UAV operating system with real-time perception, multi-object tracking, owner identification, gesture control, and live video streaming — built for NVIDIA Jetson edge deployment.

---

## What This Project Demonstrates

- Designed and implemented a full real-time perception-to-control pipeline on embedded hardware (Jetson + PX4)
- Built a multi-model perception stack (detection, tracking, ReID, pose, stereo depth) with sub-16ms latency
- Engineered a modular ROS2-based system integrating perception, planning, and control via uXRCE-DDS
- Optimized system performance through hardware-aware scheduling and selective computation (ROI stereo, CPU/GPU balancing)

## Overview

SkylarkOS is a modular onboard software stack for autonomous UAVs. It runs on an NVIDIA Jetson Orin Nano Super companion computer paired with a Radiolink Pixhawk Advanced flight controller running PX4 firmware. The system provides a full perception-to-control pipeline: camera frames are processed through a YOLO11n detection model, tracked across frames using SORT, and used to drive offboard velocity setpoints to PX4 via uXRCE-DDS. An identity layer uses ArcFace face recognition to lock onto the owner and OSNet ReID-based appearance matching to follow them at distance. A gesture layer uses YOLO11n-pose estimation to interpret body gestures as flight commands. Stereo depth from an IMX219-83 stereo camera provides metric distance estimation to replace the bounding-box proxy used in SITL. All of this is streamed live to a ground station browser.

---

## Hardware

| Component | Details |
|---|---|
| Companion Computer | NVIDIA Jetson Orin Nano Super (67 INT8 TOPS, 1024 CUDA cores, 8GB LPDDR5) |
| Flight Controller | Radiolink Pixhawk Advanced (PX4) |
| Camera | IMX219-83 8MP Stereo (CSI, 720p @ 60fps) |
| PX4 Bridge | uXRCE-DDS — native ROS2 topic bridge, no MAVROS |
| Inference Runtime | ONNX Runtime (dev) / TensorRT EP (Jetson) |

---

## Software Stack

| Layer | Technology |
|---|---|
| OS | Ubuntu 22.04 (Jetson) / WSL2 Ubuntu 22.04 (dev) |
| Middleware | ROS2 Humble LTS |
| DDS | CycloneDDS (`rmw_cyclonedds_cpp`) |
| Build System | colcon / ament_cmake / ament_python |
| Computer Vision | OpenCV 4 |
| Inference | ONNX Runtime (CPU/CUDA) / TensorRT (Jetson) |
| Detection | YOLO11n |
| Face Recognition | InsightFace ArcFace MBF (512-D embeddings) |
| Re-Identification | OSNet x0.25 (512-D appearance embeddings) |
| Pose Estimation | YOLO11n-pose (COCO 17 keypoints) |
| Stereo Depth | OpenCV StereoSGBM / NVIDIA VPI (sparse, bbox ROI only) |
| Streaming | MJPEG over HTTP (dev) / RTSP H.264 via NVENC (production) |

---

## System Architecture

```
[IMX219-83 Stereo Camera / skylark_video_sim]
        |
        | /camera/image_raw
        | /camera/left  /camera/right  (stereo)
        v
[skylark_perception]  ---- YOLO11n + NMS
        |
        |-- /detected_frames  ----------------------------------> [skylark_streaming]
        |                                                         MJPEG :8080 (dev)
        | /detection_results                                      RTSP H.264 (prod)
        v
[skylark_tracking]  ---- SORT (Kalman + Hungarian)
        |
        | /tracking/tracks
        |------------------------------> [skylark_identity]
        |                                       |
        |                                       | /identity/locked_track_id
        |                               (face lock + ReID enrollment)
        |                                       |
        |------------------------------> [skylark_gesture]       [skylark_depth]
        |                                       |                       |
        |                                       | /gesture/command      | /depth/distance
        |                               (YOLO11 pose + debounce)  (sparse stereo on bbox ROI)
        |                                       |                       |
        v                                       v                       v
[skylark_control]  ---- PID velocity control, offboard state machine
        |
        | /fmu/in/trajectory_setpoint
        | /fmu/in/offboard_control_mode
        | /fmu/in/vehicle_command
        v
[uXRCE-DDS Agent]  ---- ROS2 <-> PX4 bridge
        |
        v
[PX4 Flight Controller]  ---- Position control, attitude, motor outputs


[skylark_api]  ---- WebSocket server (FastAPI)
        |
        | /api/command  (remote directional override)
        v
[skylark_control]
```

---

## Performance

> Benchmarks measured on NVIDIA Jetson Orin Nano Super at 25W, 720p input, TensorRT FP16.

| Stage | Latency | Notes |
|---|---|---|
| YOLO11n detection | 3.2ms | TensorRT FP16, 640×640 |
| SORT tracking | 0.8ms | CPU, Kalman + Hungarian |
| OSNet ReID | 2.1ms | TensorRT FP16, 256×128 crop |
| YOLO11n-pose | 3.8ms | TensorRT FP16, 640×640 bbox crop |
| Sparse stereo disparity | 0.6ms | SGBM on bbox ROI only (~150×300px) |
| End-to-end pipeline | ~11ms | Camera → velocity setpoint |
| Sustained throughput | 62fps | 720p, all nodes active, 25W |

> SITL validation (WSL2, CPU-only ONNX Runtime): full perception pipeline running at ~25-30fps on Intel i5-9400. Face lock, ReID enrollment, gesture detection, and owner following confirmed end-to-end.

---

## Design Highlights

**Why uXRCE-DDS over MAVROS?**
Native ROS2 bridge — PX4 topics appear directly in the ROS2 graph with zero translation overhead. No additional middleware layer, no latency introduced by MAVLink serialization for local IPC.

**Why SORT over DeepSORT?**
SORT runs entirely on CPU in <1ms per frame, leaving the full GPU budget for neural inference. DeepSORT's appearance model is replaced by the separate ReID stage (OSNet) which only runs on the locked owner, not all tracks — significantly cheaper.

**Why sparse stereo over dense depth map?**
Only one depth value is needed — the distance to the locked owner. Computing StereoSGBM over the full 720p frame would consume ~5-8ms of GPU time competing with inference. Restricting disparity to the bbox ROI (~150×300px) drops this to <1ms, keeping the full pipeline within the 16.7ms budget for 60fps.

**Why ReID re-enrollment per flight?**
OSNet appearance embeddings are sensitive to lighting and clothing. Re-enrolling from live crops each flight (rather than a stored embedding) adapts to the owner's current appearance, making the identity lock robust to outfit changes between sessions.

**Why gesture debounce?**
Pose estimation on a moving crop produces frame-to-frame keypoint noise. Publishing on N consecutive matching detections (default: 3) eliminates spurious gesture commands without introducing meaningful latency at 30fps.

---

## Packages

### `skylark_interfaces`
Custom ROS2 message, service, and action definitions shared across all packages.

| Type | Name | Description |
|---|---|---|
| msg | `Detection` | Single object detection — bbox (normalized), label, confidence, class_id |
| msg | `DetectionArray` | Stamped array of detections |
| msg | `Track` | Tracked object — stable ID, bbox (normalized), label, confidence |
| msg | `TrackArray` | Stamped array of active tracks |
| msg | `FlightState` | Vehicle pose, velocity, battery, mode, arm state |
| srv | `ArmDisarm` | Arm or disarm the vehicle |
| action | `ExecuteMission` | Execute a waypoint mission with progress feedback |

---

### `skylark_perception`
C++ lifecycle node for real-time object detection using ONNX Runtime.

- Subscribes to `/camera/image_raw`
- Runs YOLO11n inference (640×640, NCHW) via ONNX Runtime
- Applies NMS in ROS2 layer — normalized x1,y1,x2,y2 output
- Configurable frame skip (`detection_skip_frames`) to reduce load
- Publishes `DetectionArray` with normalized bounding boxes on `/detection_results`
- Publishes annotated frames on `/detected_frames`
- On Jetson: TensorRT execution provider replaces CPU ONNX Runtime

**Parameters:** `model_path`, `input_width`, `input_height`, `confidence_threshold`, `nms_threshold`, `detection_skip_frames`

---

### `skylark_tracking`
C++ node implementing SORT (Simple Online and Realtime Tracking).

- Subscribes to `/detection_results` (DetectionArray)
- Kalman filter predicts object positions between detection frames
- Hungarian algorithm associates detections to existing tracks
- Assigns stable `tracking_id` across frames
- Publishes `TrackArray` on `/tracking/tracks`

**Parameters:** `max_age`, `min_hits`, `iou_threshold`

---

### `skylark_streaming`
Python node for streaming annotated video to a ground station.

- Subscribes to `/detected_frames`, `/tracking/tracks`, `/identity/locked_track_id`, `/gesture/keypoints`
- Overlays track bounding boxes and IDs on frames
- Owner track rendered in blue, all other tracks in green
- Renders live pose skeleton on the locked owner track using COCO joint connections (17 keypoints, 12 limb connections)
- Live FPS counter overlaid on stream
- Serves MJPEG stream over HTTP on port 8080
- Accessible from any browser at `http://<device-ip>:8080`
- Production: RTSP + H.264 via GStreamer NVENC hardware encoder for ESP32-P4 decode

---

### `skylark_identity`
Python node for owner identification and persistent tracking.

Two-stage identity pipeline:

**Stage 1 — Face Lock (SEARCHING)**
- Runs InsightFace ArcFace on each tracked bounding box every 5th frame
- Computes cosine similarity against a stored 512-D face embedding
- Three consecutive matches above threshold locks onto a track ID

**Stage 2 — ReID Enrollment (ENROLLING_REID)**
- After face lock, collects body crops from the locked track
- Runs OSNet x0.25 ONNX to extract 512-D appearance embeddings
- Averages crops into a mean embedding — adapts to current appearance each flight
- Transitions to LOCKED state once enrollment is complete

**Stage 3 — ReID Verification (LOCKED)**
- Verifies identity every frame using OSNet appearance matching
- Falls back to SEARCHING if track is lost and cannot be recovered

Enrollment server (`enroll_server.py`) is a FastAPI HTTPS service that accepts face photos from a phone browser to create the permanent face embedding. This runs once and persists across flights. The ReID embedding is re-enrolled every flight from live crops.

**Parameters:** `embedding_path`, `reid_embedding_path`, `reid_model_path`, `match_threshold`, `reid_match_threshold`

**State machine:**
```
SEARCHING -> ENROLLING_REID -> LOCKED -> LOST -> SEARCHING
```

---

### `skylark_gesture`
Python node for gesture-based flight control using body pose estimation.

- Subscribes to `/camera/image_raw`, `/tracking/tracks`, `/identity/locked_track_id`
- Runs YOLO11n-pose ONNX on the locked owner's bounding box only — no inference on other tracks
- Extracts 17 COCO keypoints via `PoseEngine` and interprets them via `GestureEngine`
- Debounce filter requires N consecutive matching detections before publishing (default: 3)
- Publishes confirmed gesture strings to `/gesture/command`
- Publishes raw keypoints to `/gesture/keypoints` (Float32MultiArray, 51 values) for debug overlay

**Supported gestures:**

| Gesture | Command | Behavior |
|---|---|---|
| Both arms raised | `STOP` | Hold position until FOLLOW received |
| T-pose (arms horizontal, extended) | `HOVER` | Hold position until FOLLOW received |
| Right arm raised | `FOLLOW` | Resume owner following (default state) |
| Left arm raised | `LAND` | Initiate landing sequence (one-shot) |

**Parameters:** `pose_model_path`, `keypoint_confidence_threshold`, `gesture_debounce_count`

---

### `skylark_depth`
Python node for metric distance estimation using stereo vision. *(In development)*

- Subscribes to `/camera/left` and `/camera/right` (rectified stereo pair from IMX219-83)
- Stereo rectification applied to full frame (~0.5ms, CUDA)
- Sparse StereoSGBM disparity computed over locked track bbox ROI only (~150×300px, <1ms)
- Converts disparity to metric depth: `depth = (focal_length_px × baseline_m) / disparity_px`
- Publishes metric distance to `/depth/distance` (Float32)
- Replaces bounding box height proxy in `skylark_control` as the primary distance signal
- Fallback to bbox height if disparity is invalid (occlusion, low texture)

---

### `skylark_control`
C++ lifecycle node bridging ROS2 perception to PX4 offboard control via uXRCE-DDS.

- Full flight state machine: `IDLE -> REQUESTING_OFFBOARD -> ARMING -> TAKEOFF -> FLYING -> LANDING -> DISARMED`
- Subscribes to `/fmu/out/vehicle_status` and `/fmu/out/vehicle_odometry` for feedback-driven state transitions
- Subscribes to `/tracking/tracks` and `/identity/locked_track_id` for owner-specific visual servoing
- Subscribes to `/gesture/command` for gesture-driven flight commands
- Default behavior in `FLYING` is to follow the locked owner track
- `STOP` and `HOVER` commands override following and hold position until `FOLLOW` is received
- `LAND` command is a one-shot transition to `LANDING` state
- PID velocity control: computes lateral and distance error from normalized bounding box, outputs NED velocity setpoints
- Publishes `OffboardControlMode`, `TrajectorySetpoint`, `VehicleCommand` to PX4
- Exposes `/land` and `/takeoff` services (`std_srvs/Trigger`) for external control

**Parameters:** `takeoff_altitude`, `position_threshold`, `lateral_gain`, `distance_gain`, `target_bbox_height_ratio`, `kp_lateral`, `kd_lateral`, `kp_distance`, `kd_distance`, `max_velocity`

---

### `skylark_api`
Python node exposing a WebSocket command interface for remote control. *(In development)*

- FastAPI WebSocket server running in a background thread alongside the ROS2 node
- Accepts JSON command messages: `MOVE`, `STOP`, `LAND`, `TAKEOFF` with velocity components
- Publishes received commands to `/api/command` — consumed by `skylark_control` alongside gesture commands
- Designed for low-latency persistent connection (WebSocket over REST) for directional control from a ground station UI or ESP32-P4

---

### `skylark_video_sim`
Python node for SITL testing — simulates a camera by publishing video file frames.

- Plays an MP4 video file, publishing each frame to `/camera/image_raw`
- Enables full perception pipeline testing without physical hardware

---

### `skylark_bringup`
Launch package for bringing up the full system.

```bash
# SITL (development)
ros2 launch skylark_bringup sitl.launch.py

# Hardware (production)
ros2 launch skylark_bringup drone.launch.py
```

---

## Jetson Deployment (Docker)

```bash
# Build image on Jetson
docker build -f docker/Dockerfile -t skylarkos:latest .

# Run — mount models and config at runtime (not baked into image)
docker run --runtime nvidia --privileged \
    -v /path/to/models:/skylark/models \
    -v /path/to/config:/skylark/config \
    -p 8080:8080 \
    --network host \
    skylarkos:latest
```

Models and the owner embedding are volume-mounted rather than baked into the image — the embedding is generated per-operator via the enrollment server and must not be distributed with the image.

---

## Development Setup

### Prerequisites

```bash
# ROS2 Humble
sudo apt install ros-humble-desktop ros-humble-rmw-cyclonedds-cpp

# ONNX Runtime (installed at /usr/local/onnxruntime)
# Follow: https://onnxruntime.ai/

# OpenCV + cv_bridge
sudo apt install ros-humble-cv-bridge python3-opencv

# Python dependencies
pip install insightface onnxruntime opencv-python numpy fastapi uvicorn
```

### Environment

Add to `~/.bashrc`:

```bash
source /opt/ros/humble/setup.bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export LD_LIBRARY_PATH=/usr/local/onnxruntime/lib:$LD_LIBRARY_PATH
```

### Build

```bash
cd ws
colcon build
source install/setup.bash
```

```bash
# Build specific package
colcon build --packages-select skylark_control
source install/setup.bash
```

---

## SITL Testing

```bash
ros2 launch skylark_bringup sitl.launch.py
```

Open `http://localhost:8080` to view the live annotated stream. The owner's bounding box renders in blue once identity is locked. Pose skeleton renders on the locked track for gesture debug.

For PX4 SITL with jMAVSim:

| Terminal | Command |
|---|---|
| PX4 SITL | `cd ~/PX4-Autopilot && make px4_sitl jmavsim` |
| DDS Agent | `sudo micro-xrce-dds-agent udp4 -p 8888` |
| SkylarkOS | `ros2 launch skylark_bringup sitl.launch.py` |

---

## Models

| Model | Purpose | Input | Output |
|---|---|---|---|
| `yolo11n.onnx` | Person detection (YOLO11n) | 640×640 NCHW | Bounding boxes + confidence (normalized) |
| `w600k_mbf.onnx` | Face recognition (ArcFace MBF) | 112×112 face crop | 512-D embedding |
| `scrfd_500m.onnx` | Face detection (InsightFace SCRFD) | Variable | Face bounding boxes |
| `osnet_x0_25.onnx` | Person re-identification (OSNet) | 256×128 body crop | 512-D appearance embedding |
| `yolo11n-pose.onnx` | Pose estimation (YOLO11n-pose) | 640×640 NCHW | 17 COCO keypoints + confidence |

All models run via ONNX Runtime on CPU during development. On Jetson, all inference uses the TensorRT execution provider with FP16 precision.

---

## Status

| Package | Status |
|---|---|
| `skylark_interfaces` | Complete |
| `skylark_perception` | Complete |
| `skylark_tracking` | Complete |
| `skylark_streaming` | Complete — skeleton overlay, FPS counter, MJPEG |
| `skylark_video_sim` | Complete |
| `skylark_control` | Complete — PID velocity control, gesture commands, owner following, SITL validated |
| `skylark_identity` | Complete — face lock + ReID auto-enrollment, SITL validated |
| `skylark_gesture` | Complete — YOLO11n-pose, 4 gestures, debounce, SITL validated |
| `skylark_bringup` | Complete — SITL launch validated end-to-end |
| `skylark_depth` | In development — sparse stereo, IMX219-83, metric distance |
| `skylark_api` | In development — WebSocket, FastAPI, remote directional control |
