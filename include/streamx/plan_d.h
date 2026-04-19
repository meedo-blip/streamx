#pragma once

#include <string>

namespace streamx {
namespace plan_d {
    bool StartPlanD(int& pid_out, int width, int height, int fps, const std::string& rtmp_url);
    void StopPlanD(int pid);
    int GetReadFd();
    std::string GetOutputFile();
}
}
