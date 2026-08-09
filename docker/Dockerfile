# Jetpack Version: 6.2

FROM dustynv/ros:humble-desktop-l4t-r36.4.0
# FROM dustynv/ros:humble-ros-base-l4t-r36.3.0
# Creating the environment variables for the ROS workspace
ENV DEBIAN_FRONTEND=noninteractive \
    ROS_DISTRO=humble \
    RMW_IMPLEMENTATION=rmw_cyclonedds_cpp \
    PYTHONUNBUFFERED=1

# Add NVIDIA Jetson apt repositories for TensorRT and cuDNN
COPY docker/jetson-ota-public.asc /etc/apt/trusted.gpg.d/jetson-ota-public.asc
RUN echo "deb https://repo.download.nvidia.com/jetson/common r36.4 main" > /etc/apt/sources.list.d/nvidia-jetson.list && \
    echo "deb https://repo.download.nvidia.com/jetson/t234 r36.4 main" >> /etc/apt/sources.list.d/nvidia-jetson.list

# Refresh expired ROS2 GPG key and fix apt source
RUN curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
    -o /usr/share/keyrings/ros-archive-keyring.gpg && \
    echo "deb [arch=arm64 signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu jammy main" \
    > /etc/apt/sources.list.d/ros2.list

# Instructions to install ROS packages and dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-humble-rmw-cyclonedds-cpp \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-pip \
    liblapack3 \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-rtsp \
    libgstrtspserver-1.0-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libnvinfer10 \
    libnvonnxparsers10 \
    libcudnn9-cuda-12 \
    kmod \
    && rm -rf /var/lib/apt/lists/*

# Remove GStreamer plugins that link against libjpeg.so.8 (struct size 584).
# These conflict with OpenCV which is compiled against libjpeg-turbo (struct size 728).
# None of these are used in the drone pipeline — all encoding uses Jetson hardware
# codecs (nvjpegenc, nvvidconv, nvarguscamerasrc).
RUN rm -f \
    /usr/lib/aarch64-linux-gnu/gstreamer-1.0/libgstjpeg.so \
    /usr/lib/aarch64-linux-gnu/gstreamer-1.0/libgstlibav.so \
    /usr/lib/aarch64-linux-gnu/gstreamer-1.0/libgstchromaprint.so \
    /usr/lib/aarch64-linux-gnu/gstreamer-1.0/libgstgdkpixbuf.so \
    /usr/lib/aarch64-linux-gnu/gstreamer-1.0/libgstopenni2.so \
    /usr/lib/aarch64-linux-gnu/gstreamer-1.0/libgstrsvg.so \
    /usr/lib/aarch64-linux-gnu/gstreamer-1.0/libgstspandsp.so \
    /usr/lib/aarch64-linux-gnu/gstreamer-1.0/libgstzbar.so

# Installing the Python dependencies for the ROS workspace.
# insightface pulls in CPU onnxruntime 1.23.2 — the GPU files below replace the key pieces.
RUN pip3 install --no-cache-dir \
    --index-url https://pypi.org/simple \
    insightface \
    "numpy<2" \
    scipy \
    fastapi \
    "uvicorn[standard]" \
    python-multipart

# GPU onnxruntime 1.24.0 files extracted from a known-good skylark image.
# Replaces the CPU onnxruntime installed by insightface without needing the Jetson pip server.
COPY docker/ort_gpu_files/ /tmp/ort_gpu_files/

# Overwrite CPU onnxruntime with GPU 1.24.0 files and remove the cpython-tagged .so.
# Python prefers onnxruntime_pybind11_state.cpython-310-aarch64-linux-gnu.so (CPU 1.23.2)
# over onnxruntime_pybind11_state.so (GPU 1.24.0) so the CPU one must be deleted.
RUN SITE=/usr/local/lib/python3.10/dist-packages && \
    cp /tmp/ort_gpu_files/onnxruntime_pybind11_state.so $SITE/onnxruntime/capi/ && \
    cp /tmp/ort_gpu_files/libonnxruntime.so.1.23.0 $SITE/onnxruntime/capi/ && \
    cp /tmp/ort_gpu_files/libonnxruntime_providers_tensorrt.so $SITE/onnxruntime/capi/ && \
    cp /tmp/ort_gpu_files/libonnxruntime_providers_cuda.so $SITE/onnxruntime/capi/ && \
    cp /tmp/ort_gpu_files/ort_init.py $SITE/onnxruntime/__init__.py && \
    cp /tmp/ort_gpu_files/ort_pybind_state.py $SITE/onnxruntime/capi/_pybind_state.py && \
    cp /tmp/ort_gpu_files/ort_ld_preload.py $SITE/onnxruntime/capi/_ld_preload.py && \
    cp /tmp/ort_gpu_files/onnxruntime_inference_collection.py $SITE/onnxruntime/capi/onnxruntime_inference_collection.py && \
    rm -f $SITE/onnxruntime/capi/onnxruntime_pybind11_state.cpython-310-aarch64-linux-gnu.so && \
    rm -rf /tmp/ort_gpu_files

# Install ONNX Runtime C++ headers (from GitHub aarch64 release)
# Library comes from the Python wheel above which has TensorRT EP
RUN wget -q https://github.com/microsoft/onnxruntime/releases/download/v1.23.0/onnxruntime-linux-aarch64-1.23.0.tgz && \
    tar -xzf onnxruntime-linux-aarch64-1.23.0.tgz && \
    mkdir -p /usr/local/onnxruntime/include /usr/local/onnxruntime/lib && \
    cp -r onnxruntime-linux-aarch64-1.23.0/include/* /usr/local/onnxruntime/include/ && \
    rm -rf onnxruntime-linux-aarch64-1.23.0*

# Symlink the TensorRT-enabled library into the C++ search path.
# Use symlinks not copies — two copies of the same lib causes heap corruption.
RUN SITE=/usr/local/lib/python3.10/dist-packages/onnxruntime/capi && \
    mkdir -p /usr/local/onnxruntime/lib && \
    ln -sf "$SITE/libonnxruntime.so.1.23.0" /usr/local/onnxruntime/lib/libonnxruntime.so && \
    ln -sf "$SITE/libonnxruntime.so.1.23.0" /usr/local/onnxruntime/lib/libonnxruntime.so.1 && \
    ln -sf "$SITE/libonnxruntime_providers_shared.so" /usr/local/onnxruntime/lib/libonnxruntime_providers_shared.so && \
    ln -sf "$SITE/libonnxruntime_providers_tensorrt.so" /usr/local/onnxruntime/lib/libonnxruntime_providers_tensorrt.so && \
    ln -sf "$SITE/libonnxruntime_providers_cuda.so" /usr/local/onnxruntime/lib/libonnxruntime_providers_cuda.so && \
    echo "/usr/local/onnxruntime/lib" > /etc/ld.so.conf.d/onnxruntime.conf && \
    ldconfig

# Setting up the Docker workspace and copying the source code
WORKDIR /skylark
COPY ws/src ./ws/src

# Clone cv_bridge from source to build against dustynv's CUDA OpenCV 4.8.1
RUN git clone --depth 1 --branch humble \
    https://github.com/ros-perception/vision_opencv.git \
    /skylark/ws/src/vision_opencv

# Building the ROS workspace
RUN bash -c "\
    source /opt/ros/humble/install/setup.bash && \
    cd /skylark && \
    colcon build \
        --cmake-args -DCMAKE_BUILD_TYPE=Release \
        --event-handlers console_cohesion+ \
    "

# Making directory for the data files and the ONNX models
RUN mkdir -p /skylark/models /skylark/data /skylark/config

# Creating the entrypoint script to source the ROS workspace and run the application
COPY docker/entrypoint.sh /entrypoint.sh
COPY scripts/ /skylark/scripts/

RUN chmod +x /skylark/scripts/gen_cert.sh
RUN chmod +x /entrypoint.sh

# Need the 8080 PORT Exposed
EXPOSE 8080
EXPOSE 8554

# Starting up the application using the entrypoint script
ENTRYPOINT [ "/entrypoint.sh" ]
CMD [ "ros2", "launch", "skylark_bringup", "drone.launch.py" ]

# 