#include "streamx/plan_d.h"
#include "streamx/utils/logger.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <thread>
#include <chrono>

static int g_plan_d_pid = -1;

bool streamx::plan_d::StartPlanD(int& pid_out, int width, int height, int fps, const std::string& rtmp_url) {
    pid_out = -1;

    if (rtmp_url.empty()) {
        STREAMX_WARN("[PlanD] No RTMP URL provided");
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }

    if (pid == 0) {
        char size[32], fr[16];
        snprintf(size, sizeof(size), "%ux%u", width, height);
        snprintf(fr, sizeof(fr), "%u", fps);

        // Use GPU Screen Recorder's built-in RTMP streaming
        // -c flv for RTMP, -a default_output for audio
        execl("/usr/bin/gpu-screen-recorder", "gpu-screen-recorder",
              "-w", "screen",
              "-c", "flv",
              "-s", size,
              "-f", fr,
              "-k", "h264",
              "-a", "default_output",
              "-o", rtmp_url.c_str(),
              (char*)nullptr);

        STREAMX_WARN("[PlanD] Failed to exec gpu-screen-recorder");
        _exit(1);
    }

    pid_out = pid;
    g_plan_d_pid = pid;

    std::this_thread::sleep_for(std::chrono::seconds(2));

    int status;
    if (waitpid(pid, &status, WNOHANG) != 0) {
        STREAMX_WARN("[PlanD] gpu-screen-recorder failed to start");
        return false;
    }

    STREAMX_INFO("[PlanD] GPU Screen Recorder started (pid=" + std::to_string(pid) + ") - RTMP streaming to " + rtmp_url);
    return true;
}

void streamx::plan_d::StopPlanD(int pid) {
    if (pid > 0) {
        kill(pid, SIGTERM);
        int status;
        waitpid(pid, &status, 0);
    }
    g_plan_d_pid = -1;
}

int streamx::plan_d::GetReadFd() {
    return -1;
}

std::string streamx::plan_d::GetOutputFile() {
    return "";
}