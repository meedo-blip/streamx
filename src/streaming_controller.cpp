#include "streamx/streaming_controller.h"
#include "streamx/platform/platform_implementations.h"
#include "streamx/utils/logger.h"
#include <chrono>
#include <algorithm>

namespace streamx {

StreamingController::StreamingController()
    : platform_manager_(std::make_unique<StreamingPlatformManager>()),
      thread_pool_(std::make_unique<ThreadPool>(4)) {
    STREAMX_INFO("StreamingController created");
}

StreamingController::~StreamingController() {
    StopStreaming();
    StopCapture();
    DisconnectAll();
}

bool StreamingController::Initialize(const VideoConfig& video_config, const AudioConfig& audio_config) {
    video_config_ = video_config;
    audio_config_ = audio_config;

    // Create encoder
    encoder_ = EncoderFactory::CreateEncoder(video_config.codec);
    if (!encoder_ || !encoder_->Initialize(video_config)) {
        STREAMX_ERROR("Failed to initialize encoder");
        return false;
    }

    STREAMX_INFO("StreamingController initialized");
    return true;
}

bool StreamingController::AddPlatform(const std::string& name, const std::string& platform_type,
                                     const std::string& stream_key, const std::string& server_url) {
    STREAMX_INFO("AddPlatform called: name=" + name + " type=" + platform_type);
    
    // Normalize platform name to lowercase for consistency
    std::string normalized_name = name;
    std::transform(normalized_name.begin(), normalized_name.end(), normalized_name.begin(), ::tolower);
    std::transform(normalized_name.begin(), normalized_name.end(), normalized_name.begin(), ::tolower);
    
    std::unique_ptr<IStreamingPlatform> platform;
    std::string normalized_type = platform_type;
    std::transform(normalized_type.begin(), normalized_type.end(), normalized_type.begin(), ::tolower);

    if (normalized_type == "twitch") {
        platform = std::make_unique<TwitchPlatform>();
    } else if (normalized_type == "youtube") {
        platform = std::make_unique<YouTubePlatform>();
    } else if (normalized_type == "streamlabs") {
        platform = std::make_unique<StreamLabsPlatform>();
    } else {
        platform = std::make_unique<CustomRTMPPlatform>(platform_type);
    }

    if (!platform_manager_->AddPlatform(normalized_name, std::move(platform))) {
        STREAMX_ERROR("AddPlatform failed - platform_manager returned false");
        return false;
    }

    StreamConfig config;
    config.video = video_config_;
    config.audio = audio_config_;
    config.stream_key = stream_key;
    config.server_url = server_url;
    platform_configs_[normalized_name] = config;

    STREAMX_INFO("AddPlatform completed successfully for: " + normalized_name);
    return true;
}

bool StreamingController::RemovePlatform(const std::string& name) {
    // Normalize platform name to lowercase for consistency
    std::string normalized_name = name;
    std::transform(normalized_name.begin(), normalized_name.end(), normalized_name.begin(), ::tolower);
    if (!platform_manager_->RemovePlatform(normalized_name)) {
        return false;
    }

    platform_configs_.erase(normalized_name);
    return true;
}

bool StreamingController::ConnectAll(const Credentials& creds, const StreamConfig& config) {
    return platform_manager_->ConnectAll(creds, config, encoder_ ? encoder_->GetCodecContext() : nullptr);
}

bool StreamingController::DisconnectAll() {
    return platform_manager_->DisconnectAll();
}

bool StreamingController::StartCapture(int display_index, const std::string& source) {
    if (is_capturing_) {
        STREAMX_WARN("Capture already running");
        return false;
    }

    // Set RTMP URL from first platform's config (for Plan D GPU Screen Recorder)
    auto all_platforms = platform_manager_->GetAllPlatforms();
    if (!all_platforms.empty()) {
        auto* platform = platform_manager_->GetPlatform(all_platforms[0]);
        if (platform) {
            auto config_it = platform_configs_.find(all_platforms[0]);
            if (config_it != platform_configs_.end() && !config_it->second.stream_key.empty()) {
                // Build RTMP URL from stream key
                std::string rtmp_url;
                if (all_platforms[0] == "twitch") {
                    rtmp_url = "rtmps://live-ams.twitch.tv/app/" + config_it->second.stream_key;
                } else if (all_platforms[0] == "youtube") {
                    rtmp_url = "rtmps://a.rtmp.youtube.com/live2/" + config_it->second.stream_key;
                }
                if (!rtmp_url.empty()) {
                    video_config_.rtmp_url = rtmp_url;
                    STREAMX_INFO("Set RTMP URL for Plan D: " + rtmp_url);
                }
            }
        }
    }

    capture_ = CaptureFactory::CreateCapture(display_index);
    if (!capture_ || !capture_->Initialize(video_config_, audio_config_)) {
        STREAMX_ERROR("Failed to initialize capture");
        return false;
    }

    if (!capture_->Start()) {
        STREAMX_ERROR("Failed to start capture");
        return false;
    }

    is_capturing_ = true;
    STREAMX_INFO("Capture started");
    return true;
}

bool StreamingController::StopCapture() {
    if (!is_capturing_) {
        return true;
    }

    if (capture_) {
        capture_->Stop();
        capture_->Close();
    }

    is_capturing_ = false;
    STREAMX_INFO("Capture stopped");
    return true;
}

bool StreamingController::StartStreaming() {
    if (is_streaming_) {
        STREAMX_WARN("Streaming already running");
        return false;
    }

    if (!is_capturing_) {
        STREAMX_ERROR("Capture not started");
        return false;
    }

    auto all_platforms = platform_manager_->GetAllPlatforms();
    if (all_platforms.empty()) {
        STREAMX_ERROR("No platforms added");
        StopCapture();
        return false;
    }

    // Skip RTMP connection if Plan D is handling streaming (GPU Screen Recorder with RTMP)
    // Check if video_config has RTMP URL set (used by Plan D)
    bool plan_d_handles_streaming = !video_config_.rtmp_url.empty();
    
    Credentials creds;
    for (const auto& platform_name : all_platforms) {
        auto* platform = platform_manager_->GetPlatform(platform_name);
        if (!platform) {
            STREAMX_ERROR("Platform missing during stream start: " + platform_name);
            DisconnectAll();
            StopCapture();
            return false;
        }

        auto config_it = platform_configs_.find(platform_name);
        if (config_it == platform_configs_.end() || config_it->second.stream_key.empty()) {
            STREAMX_ERROR("Missing stream configuration for platform: " + platform_name);
            DisconnectAll();
            StopCapture();
            return false;
        }

        auto config = config_it->second;
        config.video = video_config_;
        config.audio = audio_config_;

        // Skip platform connect if Plan D is handling RTMP streaming
        if (!plan_d_handles_streaming && !platform->IsConnected() &&
            !platform->Connect(creds, config, encoder_ ? encoder_->GetCodecContext() : nullptr)) {
            STREAMX_ERROR("Failed to connect platform: " + platform_name);
            DisconnectAll();
            StopCapture();
            return false;
        } else if (plan_d_handles_streaming) {
            STREAMX_INFO("Skipping platform connect - Plan D handles RTMP streaming");
            // Mark platform as "connected" for UI purposes
            platform->SetConnected(true);
        }
    }

    is_running_ = true;
    is_streaming_ = true;

    // Start capture thread
    capture_thread_ = std::thread(&StreamingController::CaptureThreadFunc, this);

    // Start encode thread
    encode_thread_ = std::thread(&StreamingController::EncodeThreadFunc, this);

    STREAMX_INFO("Streaming started");
    return true;
}

bool StreamingController::StopStreaming() {
    if (!is_streaming_) {
        return true;
    }

    is_running_ = false;
    is_streaming_ = false;

    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }

    if (encode_thread_.joinable()) {
        encode_thread_.join();
    }
    
    // Disconnect all platforms
    DisconnectAll();
    
    // Stop capture
    StopCapture();

    STREAMX_INFO("Streaming stopped");
    STREAMX_INFO("Total frames captured: " + std::to_string(total_frames_captured_));
    STREAMX_INFO("Total frames encoded: " + std::to_string(total_frames_encoded_));
    return true;
}

void StreamingController::UpdateVideoConfig(const VideoConfig& config) {
    video_config_ = config;
    if (encoder_) {
        encoder_->Initialize(config);
    }

    for (auto& [name, platform_config] : platform_configs_) {
        platform_config.video = config;
    }
}

void StreamingController::UpdateAudioConfig(const AudioConfig& config) {
    audio_config_ = config;

    for (auto& [name, platform_config] : platform_configs_) {
        platform_config.audio = config;
    }
}

StreamHealth StreamingController::GetOverallHealth() const {
    StreamHealth overall;
    overall.connected = is_streaming_;

    auto platform_health = GetPlatformHealth();
    double total_latency = 0;
    int count = 0;

    for (const auto& [name, health] : platform_health) {
        if (health.connected) {
            total_latency += health.network_latency_ms;
            count++;
        }
    }

    if (count > 0) {
        overall.network_latency_ms = total_latency / count;
    }

    return overall;
}

std::map<std::string, StreamHealth> StreamingController::GetPlatformHealth() const {
    return platform_manager_->GetAllPlatformHealth();
}

std::vector<std::string> StreamingController::GetConnectedPlatforms() const {
    return platform_manager_->GetConnectedPlatforms();
}

std::vector<std::string> StreamingController::GetAllPlatforms() const {
    return platform_manager_->GetAllPlatforms();
}

void StreamingController::CaptureThreadFunc() {
    STREAMX_DEBUG("Capture thread started");

    while (is_running_ && capture_) {
        auto frame = capture_->CaptureFrame(std::chrono::milliseconds(33));
        if (frame) {
            capture_buffer_.TryPush(frame);
            total_frames_captured_++;
        }
    }

    STREAMX_DEBUG("Capture thread finished");
}

void StreamingController::EncodeThreadFunc() {
    STREAMX_DEBUG("Encode thread started");

    bool plan_d_handles_streaming = !video_config_.rtmp_url.empty();

    while (is_running_ && encoder_) {
        auto frame = capture_buffer_.Pop(std::chrono::milliseconds(100));
        if (!frame) {
            continue;
        }

        PacketPtr packet;
        if (encoder_->Encode(frame, packet) && packet) {
            encode_buffer_.TryPush(packet);
            total_frames_encoded_++;

            // Skip packet broadcast if Plan D handles RTMP streaming
            if (!plan_d_handles_streaming) {
                platform_manager_->BroadcastPacket(packet);
            }
        }
    }

    // Flush encoder - skip if Plan D handles streaming
    if (!plan_d_handles_streaming) {
        PacketPtr flush_packet;
        while (encoder_->Flush(flush_packet) && flush_packet) {
            platform_manager_->BroadcastPacket(flush_packet);
        }
    }

    STREAMX_DEBUG("Encode thread finished");
}

}  // namespace streamx
