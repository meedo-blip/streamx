#pragma once

// Minimal Plan D bridge interface. This will attempt to bridge GPU Screen Recorder
// output into the app's PipeWire path. For now, this is a stub that always returns false
// (bridging not implemented yet). It provides a single entry point for future work.
namespace streamx {
namespace plan_d_bridge {
    bool TryBridge(uint32_t& node_out, int& fd_out);
} 
}
