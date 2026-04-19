#pragma once

namespace streamx {
namespace plan_c {
    bool StartC1(int width, int height, int fps, const char* rtmp, int& pid_out);
    bool StartC2(int width, int height, int fps, const char* rtmp, int& pid_out);
    void StopC(int pid);
}
}
