#pragma once

// Lightweight OBS-style D-Bus screencast interface (Plan A)

namespace streamx {
class DesktopCapture;

namespace portal {
    // Start screencast via D-Bus screencast (OBS-like flow)
    // Returns true if a Plan A pathway was successfully started and
    // outputs the PipeWire node_id and PipeWire fd viaout parameters.
    bool start_screencast_dbus(streamx::DesktopCapture* cap, unsigned int& out_node_id, int& out_pipewire_fd);
    void end_screencast_dbus();
}
}
