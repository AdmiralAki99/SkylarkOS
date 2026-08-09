#!/bin/bash
set -e

# Source ROS2 and workspace
source /opt/ros/humble/install/setup.bash
source /skylark/install/setup.bash

# Validate required model files are mounted
REQUIRED_MODELS=(
    "/skylark/models/model.onnx"
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
if [ ! -f "/skylark/data/owner_embedding.npy" ]; then
    echo "[skylark] WARNING: No owner embedding found at /skylark/data/owner_embedding.npy"
    echo "[skylark] Run the enrollment server first: ros2 run skylark_identity enroll_server"
fi

export GST_DEBUG=3
echo "[skylark] Starting SkylarkOS on $(hostname)"
echo "[skylark] ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-0}"

exec python3 /skylark/scripts/bootstrap.py
