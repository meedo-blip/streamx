#include "streamx/plan_c.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "streamx/utils/logger.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

// Simple wrappers that spawn external capture pipelines.
// This is a placeholder implementation to enable Plan C experimentation.

bool streamx::plan_c::StartC1(int width, int height, int fps, const char* rtmp, int& pid_out) {
    pid_out = -1;
    char w[16], h[16], f[16];
    snprintf(w, sizeof(w), "%d", width);
    snprintf(h, sizeof(h), "%d", height);
    snprintf(f, sizeof(f), "%d", fps);
    // Absolute path to script for Plan C1
    const char* script = "/home/hamid/Documents/Work/Projects/streamx/scripts/plan_c/plan_c1_stream.sh";
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }
    if (pid == 0) {
        execl("bash", "bash", script, w, h, f, rtmp, (char*)NULL);
        _exit(1);
    }
    pid_out = static_cast<int>(pid);
    STREAMX_INFO("[PlanC] Spawned Plan C1 process (pid=" + std::to_string(pid) + ")");
    return true;
}

bool streamx::plan_c::StartC2(int width, int height, int fps, const char* rtmp, int& pid_out) {
    pid_out = -1;
    char w[16], h[16], f[16];
    snprintf(w, sizeof(w), "%d", width);
    snprintf(h, sizeof(h), "%d", height);
    snprintf(f, sizeof(f), "%d", fps);
    const char* script = "/home/hamid/Documents/Work/Projects/streamx/scripts/plan_c/plan_c2_stream.sh";
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }
    if (pid == 0) {
        execl("bash", "bash", script, w, h, f, rtmp, (char*)NULL);
        _exit(1);
    }
    pid_out = static_cast<int>(pid);
    STREAMX_INFO("[PlanC] Spawned Plan C2 process (pid=" + std::to_string(pid) + ")");
    return true;
}

void streamx::plan_c::StopC(int pid) {
    if (pid > 0) {
        kill(pid, SIGTERM);
        // best-effort; not awaited
    }
}
