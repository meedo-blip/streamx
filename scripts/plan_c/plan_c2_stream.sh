#!/usr/bin/env bash
# Plan C2: GStreamer with pipewiresrc streaming to RTMP
WIDTH=${1:-1280}
HEIGHT=${2:-720}
FPS=${3:-30}
RTMP=${4:-""}

if [[ -z "$RTMP" ]]; then
  echo "Plan C2: RTMP URL not provided" >&2
  exit 1
fi

gst-launch-1.0 pipewiresrc ! video/x-raw, width=${WIDTH}, height=${HEIGHT}, framerate=${FPS}/1 ! videoconvert ! x264enc speed-preset=ultrafast tune=zerolatency ! flvmux ! rtmpsink location="$RTMP" 
