# SkylarkOS

> ROS2-based autonomous UAV operating system with real-time object detection, multi-object tracking, and live video streaming — built for NVIDIA Jetson edge deployment.

---

## Overview

SkylarkOS is an onboard software stack for autonomous UAVs. It runs on an NVIDIA Jetson Orin companion computer paired with a Radiolink Pixhawk Advanced flight controller running PX4. The system handles perception, tracking, flight control, and ground station streaming as a modular set of ROS2 packages.

---

## Hardware

| Component | Details |
|---|---|
| Companion Computer | NVIDIA Jetson Orin / Xavier |
| Flight Controller | Radiolink Pixhawk Advanced (PX4) |
| PX4 Bridge | uXRCE-DDS (native ROS2, no MAVROS) |
| Inference | ONNX Runtime (dev) → TensorRT (deployment) |

---

## Software Stack

| Layer | Technology |
|---|---|
| OS | Ubuntu 22.04 (Jetson) / WSL2 (dev) |
| Middleware | ROS2 Humble (LTS) |
| Build System | colcon / ament_cmake / ament_python |
| CV | OpenCV 4 |
| Inference | ONNX Runtime → TensorRT |
| Streaming | MJPEG over HTTP → WebRTC (planned) |
| Navigation | Nav2 (planned) |

---

## Package Architecture

```
┌─────────────────────────────────────────────────────┐
│                     SkylarkOS                       │
│                                                     │
│  Camera ──► skylark_perception ──► /detected_frames │
│                    │                                │
│                    └──► /detection_results          │
│                                │                   │
│                    skylark_tracking (planned)       │
│                                │                   │
│                    skylark_control (planned)        │
│                          │                         │
│                        PX4 (uXRCE-DDS)             │
│                                                     │
│  /detected_frames ──► skylark_streaming ──► Browser │
└─────────────────────────────────────────────────────┘
```

---

## Packages

### `skylark_interfaces` ✅
Custom ROS2 message, service, and action definitions shared across all packages.

| Type | Name | Description |
|---|---|---|
| msg | `Detection` | Single object detection (bbox, label, confidence, class_id) |
| msg | `DetectionArray` | Array of detections with header |
| msg | `FlightState` | Vehicle pose, velocity, battery, mode, arm state |
| srv | `ArmDisarm` | Arm or disarm the vehicle |
| action | `ExecuteMission` | Execute a waypoint mission with feedback |

---

### `skylark_perception` ✅
Lifecycle node for real-time object detection using ONNX Runtime.

- Subscribes to `/camera/image_raw`
- Runs SSD MobileNet inference via ONNX Runtime (TensorRT on Jetson)
- Publishes `DetectionArray` on `/detection_results`
- Publishes annotated frames on `/detected_frames`
- Configurable via ROS2 parameters: `model_path`, `input_width`, `input_height`, `confidence_threshold`

**Run:**
```bash
ros2 run skylark_perception perception_node --ros-args -p model_path:=/path/to/model.onnx
ros2 lifecycle set /perception_node configure
ros2 lifecycle set /perception_node activate
```

---

### `skylark_streaming` (In Progress)
Python node for streaming annotated video to a ground station over WiFi.

- Subscribes to `/detected_frames`
- Serves MJPEG stream over HTTP on port 8080
- Accessible from any browser or VLC at `http://<jetson-ip>:8080/stream`

---

### `skylark_tracking` (Planned)
Multi-object tracking node using SORT (Kalman filter + Hungarian algorithm).

- Subscribes to `/detection_results`
- Assigns persistent track IDs across frames
- Publishes `TrackArray` with ID, bbox, and velocity estimate

---

### `skylark_control` (Planned)
Flight control node bridging ROS2 and PX4 via uXRCE-DDS.

- Arms/disarms vehicle
- Switches to offboard mode
- Sends `TrajectorySetpoint` commands to PX4
- Reads `VehicleOdometry` and publishes `FlightState`

---

### `skylark_navigation` (Planned)
Waypoint navigation and mission execution using Nav2.

---

## Development Setup

### Prerequisites
- ROS2 Humble
- ONNX Runtime (installed at `/usr/local/onnxruntime`)
- OpenCV 4
- cv_bridge

### Build
```bash
cd ws
source /opt/ros/humble/setup.bash
colcon build
source install/setup.bash
```

### Build a specific package
```bash
colcon build --packages-select skylark_perception
```

---

## Deployment Target

Production deployment runs on NVIDIA Jetson Orin with:
- TensorRT execution provider replacing ONNX Runtime CPU
- GStreamer pipeline for low-latency video
- PX4 SITL → real hardware migration

---

## Status

| Package | Status |
|---|---|
| `skylark_interfaces` | Complete |
| `skylark_perception` | Complete |
| `skylark_streaming` | In Progress |
| `skylark_tracking` | Planned |
| `skylark_control` | Planned |
| `skylark_navigation` | Planned |
