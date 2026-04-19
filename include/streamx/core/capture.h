#pragma once

#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "types.h"
#include "frame.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

struct _XdpPortal;
using XdpPortal = _XdpPortal;
struct _XdpSession;
using XdpSession = _XdpSession;
struct _GMainLoop;
using GMainLoop = _GMainLoop;
struct _GCancellable;
using GCancellable = _GCancellable;
struct _GObject;
using GObject = _GObject;
struct _GAsyncResult;
using GAsyncResult = _GAsyncResult;
using gpointer = void*;

struct pw_thread_loop;
struct pw_context;
struct pw_core;
struct pw_stream;
struct spa_pod;
#include <spa/utils/hook.h>

namespace streamx {

// Base capture interface
class ICapture {
public:
    virtual ~ICapture() = default;

    virtual bool Initialize(const VideoConfig& video_config, const AudioConfig& audio_config) = 0;
    virtual bool Start() = 0;
    virtual bool Stop() = 0;
    virtual FramePtr CaptureFrame(std::chrono::milliseconds timeout = std::chrono::milliseconds{33}) = 0;
    virtual void Close() = 0;

    virtual double GetCaptureLatencyMs() const = 0;
    virtual StreamHealth GetHealth() const = 0;

    // Access video config (needed for Plan D RTMP URL)
    virtual const VideoConfig& GetVideoConfig() const = 0;
};

// Desktop/Screen capture
class DesktopCapture : public ICapture {
 public:
    DesktopCapture(uint32_t display_index = 0);
    ~DesktopCapture() override;

    bool Initialize(const VideoConfig& video_config, const AudioConfig& audio_config) override;
    bool Start() override;
    bool Stop() override;
    FramePtr CaptureFrame(std::chrono::milliseconds timeout = std::chrono::milliseconds{33}) override;
    void Close() override;

    double GetCaptureLatencyMs() const override;
    StreamHealth GetHealth() const override;
    const VideoConfig& GetVideoConfig() const override;

    // Plan D integration state
    bool plan_d_active_ = false;
    int plan_d_pid_ = -1;
    int plan_d_fd_ = -1;
    // Plan D bridge handles
    uint32_t plan_d_bridge_node_ = 0;
    int plan_d_bridge_fd_ = -1;
    bool plan_d_bridge_connected_ = false;

public:
    uint32_t display_index_ = 0;
    VideoConfig video_config_;
    bool is_running_ = false;
    int64_t frames_captured_ = 0;
    double capture_latency_ms_ = 0.0;
    AVFormatContext* input_context_ = nullptr;
    AVCodecContext* decoder_context_ = nullptr;
    AVFrame* input_frame_ = nullptr;
    AVPacket* input_packet_ = nullptr;
    SwsContext* sws_context_ = nullptr;
    int video_stream_index_ = -1;
    int source_width_ = 0;
    int source_height_ = 0;

    XdpPortal* portal_ = nullptr;
    XdpSession* screencast_session_ = nullptr;
    GMainLoop* portal_loop_ = nullptr;
    GCancellable* portal_cancellable_ = nullptr;
uint32_t pipewire_node_id_ = 0;
    bool portal_ready_ = false;
    bool portal_failed_ = false;
    struct spa_hook stream_listener_;

    pw_thread_loop* pw_loop_ = nullptr;
    pw_context* pw_context_ = nullptr;
    pw_core* pw_core_ = nullptr;
    pw_stream* pw_stream_ = nullptr;
    int pw_stream_state_ = 0;
    std::vector<uint8_t> pw_frame_copy_;
    std::mutex pw_frame_mutex_;
    std::condition_variable pw_frame_cv_;
    bool pw_frame_available_ = false;
    int pw_stride_ = 0;
    int pw_frame_width_ = 0;
    int pw_frame_height_ = 0;
    uint32_t pw_format_ = 0;
    // Plan C coordination (Plan C1 then Plan C2)
    bool plan_c_active_ = false;
    int plan_c_pid_ = -1;
};

// Window capture  
class WindowCapture : public ICapture {
public:
    explicit WindowCapture(const std::string& window_title = "");
    ~WindowCapture() override;

    bool Initialize(const VideoConfig& video_config, const AudioConfig& audio_config) override;
    bool Start() override;
    bool Stop() override;
    FramePtr CaptureFrame(std::chrono::milliseconds timeout = std::chrono::milliseconds{33}) override;
    void Close() override;

    double GetCaptureLatencyMs() const override;
    StreamHealth GetHealth() const override;
    const VideoConfig& GetVideoConfig() const override;

    void SetWindowTitle(const std::string& title) { window_title_ = title; }

private:
    std::string window_title_;
    VideoConfig video_config_;
    bool is_running_ = false;
    int64_t frames_captured_ = 0;
    double capture_latency_ms_ = 0.0;
};

// Audio capture
class AudioCapture : public ICapture {
public:
    explicit AudioCapture(const std::string& device = "");
    ~AudioCapture() override;

    bool Initialize(const VideoConfig& video_config, const AudioConfig& audio_config) override;
    bool Start() override;
    bool Stop() override;
    FramePtr CaptureFrame(std::chrono::milliseconds timeout = std::chrono::milliseconds{33}) override;
    void Close() override;

    double GetCaptureLatencyMs() const override;
    StreamHealth GetHealth() const override;
    const VideoConfig& GetVideoConfig() const override;

    void SetDevice(const std::string& device) { device_ = device; }

private:
    std::string device_;
    AudioConfig audio_config_;
    VideoConfig video_config_;
    bool is_running_ = false;
    int64_t frames_captured_ = 0;
    double capture_latency_ms_ = 0.0;
};

// Capture factory
class CaptureFactory {
public:
    static std::unique_ptr<ICapture> CreateCapture(int display_index = 0);
};

}  // namespace streamx
