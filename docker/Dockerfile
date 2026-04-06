# SkylarkOS — Jetson Orin Nano Super deployment image
# Base: dustynv/ros:humble-ros-base-l4t-r36.3.0
# JetPack 6.1 | ROS2 Humble | CUDA 12.2 | TensorRT 8.6 | cuDNN 8.9

FROM dustynv/ros:humble-ros-base-l4t-r36.3.0

ENV DEBIAN_FRONTEND=noninteractive \
    ROS_DISTRO=humble \
    RMW_IMPLEMENTATION=rmw_cyclonedds_cpp \
    PYTHONUNBUFFERED=1

# System dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-humble-rmw-cyclonedds-cpp \
    ros-humble-cv-bridge \
    ros-humble-image-transport \
    ros-humble-image-pipeline \
    ros-humble-vision-opencv \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-pip \
    libopencv-dev \
    v4l-utils \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-rtsp \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    && rm -rf /var/lib/apt/lists/*

# Python dependencies
# onnxruntime-gpu: NVIDIA provides a JetPack-specific wheel with TensorRT EP
RUN pip3 install --no-cache-dir \
    insightface \
    opencv-python-headless \
    numpy \
    scipy \
    fastapi \
    uvicorn[standard] \
    python-multipart

# Install onnxruntime-gpu for JetPack 6.x (TensorRT execution provider)
# Wheel source: https://elinux.org/Jetson_Zoo#ONNX_Runtime
RUN pip3 install --no-cache-dir \
    https://nvidia.box.com/shared/static/48dtuob7meiw6ebgfsfqakc9vse62sg4.whl

# Copy workspace source
WORKDIR /skylark
COPY ws/src ./ws/src

# Build ROS2 packages
RUN bash -c "\
    source /opt/ros/humble/setup.bash && \
    cd /skylark && \
    colcon build \
        --cmake-args -DCMAKE_BUILD_TYPE=Release \
        --packages-ignore skylark_video_sim \
    "

# Copy model directory placeholder — models are volume-mounted at runtime
# docker run -v /path/to/models:/skylark/models ...
RUN mkdir -p /skylark/models /skylark/config

COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 8080

ENTRYPOINT ["/entrypoint.sh"]
CMD ["ros2", "launch", "skylark_bringup", "drone.launch.py"]
