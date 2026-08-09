#!/bin/bash
docker run -d \
  --runtime nvidia \
  --privileged \
  --device /dev/video0 \
  -v /tmp/argus_socket:/tmp/argus_socket \
  -v ~/SkylarkOS/models:/skylark/models \
  -v ~/SkylarkOS/data:/skylark/data \
  -p 8080:8080 \
  -p 8554:8554 \
  skylark