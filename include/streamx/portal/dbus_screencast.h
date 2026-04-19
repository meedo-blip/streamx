#pragma once

namespace streamx {
class DesktopCapture;
namespace portal {
    bool start_screencast_dbus(DesktopCapture* cap, unsigned int& out_node_id, int& out_pipewire_fd);
    void end_screencast_dbus();
}
}
