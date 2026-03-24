# SkylarkOS

> ROS2-based autonomous UAV operating system with real-time object detection, multi-object tracking, visual servoing, and live video streaming — built for NVIDIA Jetson edge deployment.

---

## Overview

SkylarkOS is a modular onboard software stack for autonomous UAVs. It runs on an NVIDIA Jetson Orin companion computer paired with a Radiolink Pixhawk Advanced flight controller running PX4 firmware. The system provides a full perception-to-control pipeline: camera frames are processed through a MobileNetV2-SSD detection model, tracked across frames using SORT, and used to drive offboard position setpoints to PX4 via uXRCE-DDS — with live annotated video streamed to a ground station browser.

---

## Hardware

| Component | Details |
|---|---|
| Companion Computer | NVIDIA Jetson Orin / Xavier |
| Flight Controller | Radiolink Pixhawk Advanced (PX4) |
| PX4 Bridge | uXRCE-DDS — native ROS2, no MAVROS |
| Inference Runtime | ONNX Runtime (dev) → TensorRT EP (Jetson) |
| Detection Model | MobileNetV2-SSD, 416×416, mAP 76.6 |

---

## Software Stack

| Layer | Technology |
|---|---|
| OS | Ubuntu 22.04 (Jetson) / WSL2 Ubuntu 22.04 (dev) |
| Middleware | ROS2 Humble LTS |
| DDS | CycloneDDS (`rmw_cyclonedds_cpp`) |
| Build System | colcon / ament_cmake / ament_python |
| Computer Vision | OpenCV 4 |
| Inference | ONNX Runtime → TensorRT |
| Streaming | MJPEG over HTTP → WebRTC (planned) |
| Navigation | Nav2 (planned) |

---

## System Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                          SkylarkOS                               │
│                                                                  │
│  [Camera / skylark_video_sim]                                    │
│          │                                                       │
│          │ /camera/image_raw                                     │
│          ▼                                                       │
│  [skylark_perception]  ──── MobileNetV2-SSD + NMS               │
│          │                                                       │
│          ├── /detected_frames  ──────────────────────────┐      │
│          │                                               │      │
│          │ /detection_results                            │      │
│          ▼                                               │      │
│  [skylark_tracking]  ──── SORT (Kalman + Hungarian)      │      │
│          │                                               │      │
│          │ /tracking/tracks                              │      │
│          ├──────────────────────────────────────────┐   │      │
│          │                                          ▼   ▼      │
│          │                                  [skylark_streaming] │
│          │                                  MJPEG :8080         │
│          ▼                                                       │
│  [skylark_control]  ──── Lifecycle node                         │
│          │               State machine: IDLE → REQUESTING_      │
│          │               OFFBOARD → ARMING → TAKEOFF →          │
│          │               FLYING (visual servoing) →             │
│          │               LANDING → DISARMED                     │
│          │                                                       │
│          │ /fmu/in/trajectory_setpoint                          │
│          │ /fmu/in/offboard_control_mode                        │
│          │ /fmu/in/vehicle_command                              │
│          ▼                                                       │
│  [uXRCE-DDS Agent]  ──── ROS2 ↔ PX4 bridge                     │
│          │                                                       │
│          ▼                                                       │
│  [PX4 Flight Controller]  ──── Position control, attitude,      │
│                                 motor outputs                    │
└──────────────────────────────────────────────────────────────────┘
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
- Runs MobileNetV2-SSD inference (416×416, NHWC) via ONNX Runtime
- Applies NMS in ROS2 layer
- Publishes `DetectionArray` with normalized bounding boxes on `/detection_results`
- Publishes annotated frames on `/detected_frames`
- On Jetson: TensorRT execution provider replaces CPU ONNX Runtime

**Parameters:** `model_path`, `input_width`, `input_height`, `confidence_threshold`, `nms_threshold`

```bash
ros2 run skylark_perception perception_node --ros-args -p model_path:=/path/to/model.onnx
ros2 lifecycle set /perception_node configure
ros2 lifecycle set /perception_node activate
```

---

### `skylark_tracking` 
C++ node implementing SORT (Simple Online and Realtime Tracking).

- Subscribes to `/detection_results` (DetectionArray)
- Kalman filter predicts object positions between detection frames
- Hungarian algorithm associates detections to existing tracks
- Assigns stable `tracking_id` across frames
- Publishes `TrackArray` on `/tracking/tracks`

**Parameters:** `max_age`, `min_hits`, `iou_threshold`

```bash
ros2 run skylark_tracking tracking_node
```

---

### `skylark_streaming` 
Python node for streaming annotated video to a ground station.

- Subscribes to `/detected_frames` and `/tracking/tracks`
- Overlays track bounding boxes and IDs on frames
- Serves MJPEG stream over HTTP on port 8080
- Accessible from any browser at `http://<device-ip>:8080/stream`

```bash
ros2 run skylark_streaming streaming_node
```

---

### `skylark_control` 
C++ lifecycle node bridging ROS2 perception to PX4 offboard control via uXRCE-DDS.

- Full flight state machine: `IDLE → REQUESTING_OFFBOARD → ARMING → TAKEOFF → FLYING → LANDING → DISARMED`
- Subscribes to `/fmu/out/vehicle_status` and `/fmu/out/vehicle_odometry` for feedback-driven state transitions
- Subscribes to `/tracking/tracks` for visual servoing in `FLYING` state
- Visual servoing: selects highest-confidence person track, computes lateral error and distance error from normalized bounding box, adjusts NED position setpoints
- Publishes `OffboardControlMode`, `TrajectorySetpoint`, `VehicleCommand` to PX4
- Exposes `/land` and `/takeoff` services (`std_srvs/Trigger`) for external control

**Parameters:** `takeoff_altitude`, `position_threshold`, `lateral_gain`, `distance_gain`, `target_bbox_height_ratio`

```bash
ros2 run skylark_control control_node
ros2 lifecycle set /control_node configure
ros2 lifecycle set /control_node activate

# Trigger landing
ros2 service call /land std_srvs/srv/Trigger {}

# Trigger takeoff after landing
ros2 service call /takeoff std_srvs/srv/Trigger {}
```

---

### `skylark_video_sim` ✅
Python node for SITL testing — simulates a camera by publishing video file frames.

- Plays an MP4 video file, publishing each frame to `/camera/image_raw`
- Loops continuously
- Enables full perception pipeline testing without physical hardware

```bash
ros2 run skylark_video_sim video_sim_node --ros-args -p video_path:=/path/to/video.mp4
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

Full pipeline test using jMAVSim (lightweight, recommended for WSL2):

| Terminal | Command |
|---|---|
| PX4 SITL | `cd ~/PX4-Autopilot && make px4_sitl jmavsim` |
| DDS Agent | `sudo micro-xrce-dds-agent udp4 -p 8888` |
| Perception | `ros2 run skylark_perception perception_node` |
| Tracking | `ros2 run skylark_tracking tracking_node` |
| Streaming | `ros2 run skylark_streaming streaming_node` |
| Video Sim | `ros2 run skylark_video_sim video_sim_node --ros-args -p video_path:=/path/to/video.mp4` |
| Control | `ros2 run skylark_control control_node` then configure + activate |

Open `http://localhost:8080` to view the live annotated stream.

---

## Deployment Target

Production deployment on NVIDIA Jetson Orin:
- ONNX Runtime CPU → TensorRT FP16 execution provider
- CSI camera replacing `skylark_video_sim`
- GStreamer hardware-accelerated pipeline
- Face recognition identity lock (planned) — ArcFace ONNX for owner identification
- Velocity PID control (planned) — smooth DJI-style following
- WebRTC replacing MJPEG for low-latency ground station streaming (planned)

---

## Status

| Package | Status |
|---|---|
| `skylark_interfaces` | Complete |
| `skylark_perception` | Complete |
| `skylark_tracking` | Complete |
| `skylark_streaming` | Complete |
| `skylark_video_sim` | Complete |
| `skylark_control` | Complete — visual servoing active, SITL validated |
| `skylark_identity` | Planned — face recognition for owner tracking |
| `skylark_navigation` | Planned — Nav2 waypoint missions |
