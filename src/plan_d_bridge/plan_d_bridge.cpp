#include "streamx/plan_d_bridge.h"
#include "streamx/utils/logger.h"
#include <cstdint>

bool streamx::plan_d_bridge::TryBridge(uint32_t& node_out, int& fd_out) {
    // Stub: bridging not implemented yet. Return false so fallback paths can be tested.
    (void)node_out; (void)fd_out;
    STREAMX_INFO("[PlanD-Bridge] TryBridge called - not implemented yet");
    node_out = 0; fd_out = -1;
    return false;
}
