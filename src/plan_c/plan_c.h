#pragma once

// Plan C: separate, external capture pipelines (C1/C2)
// Lightweight wrapper for starting external capture pipelines (wf-recorder/ffmpeg or GStreamer)

class VideoConfig; // forward, actual type will be included where needed

namespace streamx {
namespace plan_c {
    bool StartC1(int width, int height, int fps, const char* rtmp, int& pid_out);
    bool StartC2(int width, int height, int fps, const char* rtmp, int& pid_out);
    void StopC(int pid);
}
}
