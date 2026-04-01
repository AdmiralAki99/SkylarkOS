# SkylarkOS

> ROS2-based autonomous UAV operating system with real-time perception, multi-object tracking, owner identification, gesture control, and live video streaming — built for NVIDIA Jetson edge deployment.

---

## Overview

SkylarkOS is a modular onboard software stack for autonomous UAVs. It runs on an NVIDIA Jetson Orin companion computer paired with a Radiolink Pixhawk Advanced flight controller running PX4 firmware. The system provides a full perception-to-control pipeline: camera frames are processed through a MobileNetV2-SSD detection model, tracked across frames using SORT, and used to drive offboard velocity setpoints to PX4 via uXRCE-DDS. An identity layer uses ArcFace face recognition to lock onto the owner and ReID-based appearance matching to follow them at distance. A gesture layer uses YOLO11 pose estimation to interpret body gestures as flight commands. All of this is streamed live to a ground station browser.

---

## Hardware

| Component | Details |
|---|---|
| Companion Computer | NVIDIA Jetson Orin / Xavier |
| Flight Controller | Radiolink Pixhawk Advanced (PX4) |
| PX4 Bridge | uXRCE-DDS — native ROS2, no MAVROS |
| Inference Runtime | ONNX Runtime (dev) / TensorRT EP (Jetson) |
| Detection Model | MobileNetV2-SSD, 416x416 |

---

## Software Stack

| Layer | Technology |
|---|---|
| OS | Ubuntu 22.04 (Jetson) / WSL2 Ubuntu 22.04 (dev) |
| Middleware | ROS2 Humble LTS |
| DDS | CycloneDDS (`rmw_cyclonedds_cpp`) |
| Build System | colcon / ament_cmake / ament_python |
| Computer Vision | OpenCV 4 |
| Inference | ONNX Runtime / TensorRT |
| Face Recognition | InsightFace buffalo_sc (ArcFace MBF, 512-D embeddings) |
| Re-Identification | OSNet x0.25 (ImageNet normalized, 512-D embeddings) |
| Pose Estimation | YOLO11n-pose (COCO 17 keypoints) |
| Streaming | MJPEG over HTTP |

---

## System Architecture

```
[Camera / skylark_video_sim]
        |
        | /camera/image_raw
        v
[skylark_perception]  ---- MobileNetV2-SSD + NMS
        |
        |-- /detected_frames  ----------------------------------> [skylark_streaming]
        |                                                         MJPEG :8080
        | /detection_results
        v
[skylark_tracking]  ---- SORT (Kalman + Hungarian)
        |
        | /tracking/tracks
        |------------------------------> [skylark_identity]
        |                                       |
        |                                       | /identity/locked_track_id
        |                               (face lock + ReID enrollment)
        |                                       |
        |------------------------------> [skylark_gesture]
        |                                       |
        |                                       | /gesture/command
        |                               (YOLO11 pose + gesture logic)
        |                                       |
        v                                       v
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
```

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
- Runs MobileNetV2-SSD inference (416x416, NHWC) via ONNX Runtime
- Applies NMS in ROS2 layer
- Configurable frame skip (`detection_skip_frames`) to reduce CPU load
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

- Subscribes to `/detected_frames`, `/tracking/tracks`, and `/identity/locked_track_id`
- Overlays track bounding boxes and IDs on frames
- Owner track rendered in blue, all other tracks in green
- Serves MJPEG stream over HTTP on port 8080
- Accessible from any browser at `http://<device-ip>:8080`

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
- Averages crops into a mean embedding and saves to disk
- Transitions to LOCKED state once enrollment is complete

**Stage 3 — ReID Verification (LOCKED)**
- Verifies identity every frame using OSNet appearance matching
- Falls back to SEARCHING if track is lost and cannot be recovered

Enrollment server (`enroll_server.py`) is a FastAPI HTTPS service that accepts face photos from a phone browser to create the permanent face embedding. This runs once and persists across flights. The ReID embedding is re-enrolled every flight from live crops, adapting to the owner's current appearance.

**Parameters:** `embedding_path`, `reid_embedding_path`, `reid_model_path`, `match_threshold`, `reid_match_threshold`

**State machine:**
```
SEARCHING -> ENROLLING_REID -> LOCKED -> LOST -> SEARCHING
```

---

### `skylark_gesture`
Python node for gesture-based flight control using body pose estimation.

- Subscribes to `/camera/image_raw`, `/tracking/tracks`, `/identity/locked_track_id`
- Runs YOLO11n-pose ONNX on the locked owner's bounding box only
- Extracts 17 COCO keypoints and passes to `GestureEngine`
- Publishes detected gesture strings to `/gesture/command`

**Supported gestures:**

| Gesture | Command |
|---|---|
| Both arms raised | `STOP` |
| T-pose (arms horizontal and extended) | `HOVER` |
| Right arm raised | `FOLLOW` |
| Left arm raised | `LAND` |

**Parameters:** `pose_model_path`, `keypoint_confidence_threshold`

---

### `skylark_control`
C++ lifecycle node bridging ROS2 perception to PX4 offboard control via uXRCE-DDS.

- Full flight state machine: `IDLE -> REQUESTING_OFFBOARD -> ARMING -> TAKEOFF -> FLYING -> LANDING -> DISARMED`
- Subscribes to `/fmu/out/vehicle_status` and `/fmu/out/vehicle_odometry` for feedback-driven state transitions
- Subscribes to `/tracking/tracks` and `/identity/locked_track_id` for owner-specific visual servoing
- Subscribes to `/gesture/command` for gesture-driven flight commands
- PID velocity control: computes lateral and distance error from normalized bounding box, outputs NED velocity setpoints
- Publishes `OffboardControlMode`, `TrajectorySetpoint`, `VehicleCommand` to PX4
- Exposes `/land` and `/takeoff` services (`std_srvs/Trigger`) for external control

**Parameters:** `takeoff_altitude`, `position_threshold`, `lateral_gain`, `distance_gain`, `target_bbox_height_ratio`

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

# Hardware (production) -- coming soon
ros2 launch skylark_bringup drone.launch.py
```

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

Open `http://localhost:8080` to view the live annotated stream. The owner's bounding box renders in blue once identity is locked.

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
| `model.onnx` | Person detection (MobileNetV2-SSD) | 416x416 NHWC | Bounding boxes + confidence |
| `w600k_mbf.onnx` | Face recognition (ArcFace MBF) | 112x112 face crop | 512-D embedding |
| `scrfd_500m.onnx` | Face detection (InsightFace) | Variable | Face bounding boxes |
| `osnet_x0_25.onnx` | Person re-identification (OSNet) | 256x128 body crop | 512-D embedding |
| `yolo11n-pose.onnx` | Pose estimation (YOLO11n) | 640x640 | 17 COCO keypoints |

All models run via ONNX Runtime on CPU during development. On Jetson, perception and tracking use the TensorRT execution provider.

---

## Status

| Package | Status |
|---|---|
| `skylark_interfaces` | Complete |
| `skylark_perception` | Complete |
| `skylark_tracking` | Complete |
| `skylark_streaming` | Complete |
| `skylark_video_sim` | Complete |
| `skylark_control` | Complete — PID velocity control, owner following, SITL validated |
| `skylark_identity` | Complete — face lock + ReID enrollment, SITL validated |
| `skylark_gesture` | Complete — YOLO11 pose, 4 gestures, pending hardware validation |
| `skylark_bringup` | Complete — SITL launch, drone launch in progress |
| `skylark_navigation` | Planned — Nav2 waypoint missions |
