#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include "streamx/core/encoder.h"
#include "streamx/core/capture.h"
#include "streamx/core/buffer.h"
#include "streamx/platform/streaming_platform.h"
#include "streamx/utils/thread_pool.h"

namespace streamx {

// Main streaming controller
class StreamingController {
public:
    StreamingController();
    ~StreamingController();

    // Initialization
    bool Initialize(const VideoConfig& video_config, const AudioConfig& audio_config);

    // Platform management
    bool AddPlatform(const std::string& name, const std::string& platform_type,
                     const std::string& stream_key = "",
                     const std::string& server_url = "");
    bool RemovePlatform(const std::string& name);
    bool ConnectAll(const Credentials& creds, const StreamConfig& config);
    bool DisconnectAll();

    // Capture control
    bool StartCapture(int display_index = 0, const std::string& source = "");
    bool StopCapture();
    bool IsCapturing() const { return is_capturing_; }

    // Streaming control
    bool StartStreaming();
    bool StopStreaming();
    bool IsStreaming() const { return is_streaming_; }

    // Configuration
    void UpdateVideoConfig(const VideoConfig& config);
    void UpdateAudioConfig(const AudioConfig& config);

    // Monitoring
    StreamHealth GetOverallHealth() const;
    std::map<std::string, StreamHealth> GetPlatformHealth() const;
    std::vector<std::string> GetConnectedPlatforms() const;
    std::vector<std::string> GetAllPlatforms() const;

private:
    std::unique_ptr<StreamingPlatformManager> platform_manager_;
    std::unique_ptr<ICapture> capture_;
    std::unique_ptr<IEncoder> encoder_;
    std::unique_ptr<ThreadPool> thread_pool_;

    VideoConfig video_config_;
    AudioConfig audio_config_;
    std::map<std::string, StreamConfig> platform_configs_;

    bool is_capturing_ = false;
    bool is_streaming_ = false;
    bool is_running_ = false;

    std::thread capture_thread_;
    std::thread encode_thread_;

    FrameBuffer<Frame> capture_buffer_;
    FrameBuffer<Packet> encode_buffer_;

    int64_t total_frames_captured_ = 0;
    int64_t total_frames_encoded_ = 0;

    void CaptureThreadFunc();
    void EncodeThreadFunc();
};

}  // namespace streamx
