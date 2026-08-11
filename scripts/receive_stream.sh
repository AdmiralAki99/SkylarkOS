#!/bin/bash
PORT="${1:-5600}"

gst-launch-1.0 udpsrc port="$PORT" ! application/x-rtp,encoding-name=H264,payload=96 ! \
  rtpjitterbuffer latency=300 drop-on-latency=true ! \
  rtph264depay ! h264parse ! avdec_h264 ! \
  queue max-size-buffers=2 leaky=downstream ! \
  autovideosink sync=false
