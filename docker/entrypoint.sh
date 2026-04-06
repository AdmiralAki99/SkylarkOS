#!/bin/bash
set -e

# Source ROS2 and workspace
source /opt/ros/humble/setup.bash
source /skylark/install/setup.bash

# Validate required model files are mounted
REQUIRED_MODELS=(
    "/skylark/models/yolo11n.onnx"
    "/skylark/models/yolo11n-pose.onnx"
    "/skylark/models/osnet_x0_25.onnx"
    "/skylark/models/w600k_mbf.onnx"
    "/skylark/models/scrfd_500m.onnx"
)

for model in "${REQUIRED_MODELS[@]}"; do
    if [ ! -f "$model" ]; then
        echo "[skylark] ERROR: Missing model file: $model"
        echo "[skylark] Mount models directory: docker run -v /path/to/models:/skylark/models ..."
        exit 1
    fi
done

# Validate owner embedding
if [ ! -f "/skylark/config/owner_embedding.npy" ]; then
    echo "[skylark] WARNING: No owner embedding found at /skylark/config/owner_embedding.npy"
    echo "[skylark] Run the enrollment server first: ros2 run skylark_identity enroll_server"
fi

echo "[skylark] Starting SkylarkOS on $(hostname)"
echo "[skylark] ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-0}"

exec "$@"
