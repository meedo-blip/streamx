#!/usr/bin/env bash
# Plan C1: wf-recorder -> FFmpeg streaming
WIDTH=${1:-1280}
HEIGHT=${2:-720}
FPS=${3:-30}
RTMP=${4:-""}

if [[ -z "$RTMP" ]]; then
  echo "Plan C1: RTMP URL not provided" >&2
  exit 1
fi

# Try to capture the primary display; adjust as needed for your setup
wf-recorder -d :0 -f rawvideo -r ${FPS} -s ${WIDTH}x${HEIGHT} | \
ffmpeg -f rawvideo -pix_fmt yuv420p -s ${WIDTH}x${HEIGHT} -r ${FPS} -i - \
-c:v libx264 -preset veryfast -tune zerolatency -f flv "$RTMP" 
