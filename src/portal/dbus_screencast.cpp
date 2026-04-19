#include "streamx/portal/dbus_screencast.h"
#include "streamx/utils/logger.h"
#include <cstdint>

// Minimal placeholder implementation for Plan A.
// This patch implements a skeleton that can be extended to wire up
// a full D-Bus screencast handshake (OBS-like flow).

bool streamx::portal::start_screencast_dbus(streamx::DesktopCapture* cap, unsigned int& out_node_id, int& out_pipewire_fd) {
    // Plan A skeleton: not implemented yet. Return false to fall back to Plan B.
    (void)cap; // suppress unused warning in this stub
    STREAMX_INFO("[PlanA-DBus] start_screencast_dbus not implemented yet");
    out_node_id = 0;
    out_pipewire_fd = -1;
    return false;
}

void streamx::portal::end_screencast_dbus() {
    STREAMX_INFO("[PlanA-DBus] end_screencast_dbus (no-op in stub)");
}
