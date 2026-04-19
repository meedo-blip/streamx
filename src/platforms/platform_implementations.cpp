#include "streamx/platform/platform_implementations.h"
#include "streamx/platform/rtmp_handler.h"
#include "streamx/utils/logger.h"
#include <thread>
#include <chrono>
#include <queue>

namespace streamx {

// ============== Twitch Platform ==============

TwitchPlatform::TwitchPlatform()
    : rtmp_(std::make_unique<RTMPHandler>()) {
    STREAMX_INFO("TwitchPlatform created");
    status_ = StreamStatus::Idle;
}

TwitchPlatform::~TwitchPlatform() {
    Disconnect();
}

bool TwitchPlatform::Connect(const Credentials& creds, const StreamConfig& config,
                             const AVCodecContext* video_codec_context) {
    config_ = config;

    // Construct Twitch RTMP URL
    std::string rtmp_url = "rtmps://live-ams.twitch.tv/app/" + config.stream_key;

    if (!rtmp_->Connect(rtmp_url, video_codec_context)) {
        status_ = StreamStatus::Failed;
        return false;
    }

    status_ = StreamStatus::Connected;
    health_.connected = true;
    should_run_ = true;

    // Start packet sender thread
    packet_sender_ = std::thread(&TwitchPlatform::PacketSenderThread, this);

    STREAMX_INFO("Connected to Twitch");
    return true;
}

bool TwitchPlatform::Disconnect() {
    should_run_ = false;
    if (packet_sender_.joinable()) {
        packet_sender_.join();
    }

    if (rtmp_) {
        rtmp_->Disconnect();
    }

    status_ = StreamStatus::Stopped;
    health_.connected = false;
    STREAMX_INFO("Disconnected from Twitch");
    return true;
}

bool TwitchPlatform::IsConnected() const {
    if (health_.connected) return true;
    return rtmp_ && rtmp_->IsConnected() && status_ == StreamStatus::Connected;
}

void TwitchPlatform::SetConnected(bool connected) {
    if (connected) {
        status_ = StreamStatus::Connected;
        health_.connected = true;
    } else {
        status_ = StreamStatus::Stopped;
        health_.connected = false;
    }
}

bool TwitchPlatform::PublishStream(const StreamConfig& config) {
    config_ = config;
    return true;
}

bool TwitchPlatform::SendPacket(const PacketPtr& packet) {
    if (!IsConnected() || !packet) {
        return false;
    }

    const bool success = rtmp_->SendPacket(packet);
    if (!success && rtmp_) {
        STREAMX_WARN("Twitch packet send failed: " + rtmp_->GetLastError());
    }
    return success;
}

void TwitchPlatform::UpdateConfig(const StreamConfig& config) {
    config_ = config;
}

StreamHealth TwitchPlatform::GetHealth() const {
    return health_;
}

StreamStatus TwitchPlatform::GetStatus() const {
    return status_;
}

bool TwitchPlatform::Reconnect() {
    STREAMX_INFO("Attempting to reconnect to Twitch");
    Disconnect();
    
    if (reconnect_callback_) {
        reconnect_callback_();
    }

    return Connect(Credentials{}, config_, nullptr);
}

void TwitchPlatform::SetReconnectCallback(std::function<void()> callback) {
    reconnect_callback_ = callback;
}

void TwitchPlatform::PacketSenderThread() {
    while (should_run_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Update health metrics
        health_.bitrate_actual_kbps = rtmp_ ? static_cast<double>(rtmp_->GetBytesSent()) * 8 / 1000 / 1000 : 0;
        health_.last_packet_time = std::chrono::system_clock::now();
    }
}

// ============== YouTube Platform ==============

YouTubePlatform::YouTubePlatform()
    : rtmp_(std::make_unique<RTMPHandler>()) {
    STREAMX_INFO("YouTubePlatform created");
    status_ = StreamStatus::Idle;
}

YouTubePlatform::~YouTubePlatform() {
    Disconnect();
}

bool YouTubePlatform::Connect(const Credentials& creds, const StreamConfig& config,
                              const AVCodecContext* video_codec_context) {
    config_ = config;

    // YouTube RTMP server
    std::string rtmp_url = "rtmps://a.rtmp.youtube.com/live2/" + config.stream_key;

    if (!rtmp_->Connect(rtmp_url, video_codec_context)) {
        status_ = StreamStatus::Failed;
        return false;
    }

    status_ = StreamStatus::Connected;
    health_.connected = true;
    should_run_ = true;

    packet_sender_ = std::thread(&YouTubePlatform::PacketSenderThread, this);

    STREAMX_INFO("Connected to YouTube");
    return true;
}

bool YouTubePlatform::Disconnect() {
    should_run_ = false;
    if (packet_sender_.joinable()) {
        packet_sender_.join();
    }

    if (rtmp_) {
        rtmp_->Disconnect();
    }

    status_ = StreamStatus::Stopped;
    health_.connected = false;
    STREAMX_INFO("Disconnected from YouTube");
    return true;
}

bool YouTubePlatform::IsConnected() const {
    if (health_.connected) return true;
    return rtmp_ && rtmp_->IsConnected() && status_ == StreamStatus::Connected;
}

void YouTubePlatform::SetConnected(bool connected) {
    if (connected) {
        status_ = StreamStatus::Connected;
        health_.connected = true;
    } else {
        status_ = StreamStatus::Stopped;
        health_.connected = false;
    }
}

bool YouTubePlatform::PublishStream(const StreamConfig& config) {
    config_ = config;
    return true;
}

bool YouTubePlatform::SendPacket(const PacketPtr& packet) {
    if (!IsConnected() || !packet) {
        return false;
    }

    const bool success = rtmp_->SendPacket(packet);
    if (!success && rtmp_) {
        STREAMX_WARN("YouTube packet send failed: " + rtmp_->GetLastError());
    }
    return success;
}

void YouTubePlatform::UpdateConfig(const StreamConfig& config) {
    config_ = config;
}

StreamHealth YouTubePlatform::GetHealth() const {
    return health_;
}

StreamStatus YouTubePlatform::GetStatus() const {
    return status_;
}

bool YouTubePlatform::Reconnect() {
    STREAMX_INFO("Attempting to reconnect to YouTube");
    Disconnect();
    
    if (reconnect_callback_) {
        reconnect_callback_();
    }

    return Connect(Credentials{}, config_, nullptr);
}

void YouTubePlatform::SetReconnectCallback(std::function<void()> callback) {
    reconnect_callback_ = callback;
}

void YouTubePlatform::PacketSenderThread() {
    while (should_run_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        health_.bitrate_actual_kbps = rtmp_ ? static_cast<double>(rtmp_->GetBytesSent()) * 8 / 1000 / 1000 : 0;
        health_.last_packet_time = std::chrono::system_clock::now();
    }
}

// ============== StreamLabs Platform ==============

StreamLabsPlatform::StreamLabsPlatform()
    : rtmp_(std::make_unique<RTMPHandler>()) {
    STREAMX_INFO("StreamLabsPlatform created");
    status_ = StreamStatus::Idle;
}

StreamLabsPlatform::~StreamLabsPlatform() {
    Disconnect();
}

bool StreamLabsPlatform::Connect(const Credentials& creds, const StreamConfig& config,
                                 const AVCodecContext* video_codec_context) {
    config_ = config;

    // StreamLabs uses Twitch RTMP server but with StreamLabs credentials
    std::string rtmp_url = "rtmps://live-ams.twitch.tv/app/" + config.stream_key;

    if (!rtmp_->Connect(rtmp_url, video_codec_context)) {
        status_ = StreamStatus::Failed;
        return false;
    }

    status_ = StreamStatus::Connected;
    health_.connected = true;
    should_run_ = true;

    packet_sender_ = std::thread(&StreamLabsPlatform::PacketSenderThread, this);

    STREAMX_INFO("Connected to StreamLabs");
    return true;
}

bool StreamLabsPlatform::Disconnect() {
    should_run_ = false;
    if (packet_sender_.joinable()) {
        packet_sender_.join();
    }

    if (rtmp_) {
        rtmp_->Disconnect();
    }

    status_ = StreamStatus::Stopped;
    health_.connected = false;
    STREAMX_INFO("Disconnected from StreamLabs");
    return true;
}

bool StreamLabsPlatform::IsConnected() const {
    if (health_.connected) return true;
    return rtmp_ && rtmp_->IsConnected() && status_ == StreamStatus::Connected;
}

void StreamLabsPlatform::SetConnected(bool connected) {
    if (connected) {
        status_ = StreamStatus::Connected;
        health_.connected = true;
    } else {
        status_ = StreamStatus::Stopped;
        health_.connected = false;
    }
}

bool StreamLabsPlatform::PublishStream(const StreamConfig& config) {
    config_ = config;
    return true;
}

bool StreamLabsPlatform::SendPacket(const PacketPtr& packet) {
    if (!IsConnected() || !packet) {
        return false;
    }

    const bool success = rtmp_->SendPacket(packet);
    if (!success && rtmp_) {
        STREAMX_WARN("StreamLabs packet send failed: " + rtmp_->GetLastError());
    }
    return success;
}

void StreamLabsPlatform::UpdateConfig(const StreamConfig& config) {
    config_ = config;
}

StreamHealth StreamLabsPlatform::GetHealth() const {
    return health_;
}

StreamStatus StreamLabsPlatform::GetStatus() const {
    return status_;
}

bool StreamLabsPlatform::Reconnect() {
    STREAMX_INFO("Attempting to reconnect to StreamLabs");
    Disconnect();
    
    if (reconnect_callback_) {
        reconnect_callback_();
    }

    return Connect(Credentials{}, config_, nullptr);
}

void StreamLabsPlatform::SetReconnectCallback(std::function<void()> callback) {
    reconnect_callback_ = callback;
}

void StreamLabsPlatform::PacketSenderThread() {
    while (should_run_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        health_.bitrate_actual_kbps = rtmp_ ? static_cast<double>(rtmp_->GetBytesSent()) * 8 / 1000 / 1000 : 0;
        health_.last_packet_time = std::chrono::system_clock::now();
    }
}

// ============== Custom RTMP Platform ==============

CustomRTMPPlatform::CustomRTMPPlatform(const std::string& platform_name)
    : platform_name_(platform_name),
      rtmp_(std::make_unique<RTMPHandler>()) {
    STREAMX_INFO("CustomRTMPPlatform created: " + platform_name);
    status_ = StreamStatus::Idle;
}

CustomRTMPPlatform::~CustomRTMPPlatform() {
    Disconnect();
}

bool CustomRTMPPlatform::Connect(const Credentials& creds, const StreamConfig& config,
                                 const AVCodecContext* video_codec_context) {
    config_ = config;

    if (!rtmp_->Connect(config.server_url, video_codec_context)) {
        status_ = StreamStatus::Failed;
        return false;
    }

    status_ = StreamStatus::Connected;
    health_.connected = true;
    should_run_ = true;

    packet_sender_ = std::thread(&CustomRTMPPlatform::PacketSenderThread, this);

    STREAMX_INFO("Connected to custom RTMP platform: " + platform_name_);
    return true;
}

bool CustomRTMPPlatform::Disconnect() {
    should_run_ = false;
    if (packet_sender_.joinable()) {
        packet_sender_.join();
    }

    if (rtmp_) {
        rtmp_->Disconnect();
    }

    status_ = StreamStatus::Stopped;
    health_.connected = false;
    STREAMX_INFO("Disconnected from custom RTMP platform: " + platform_name_);
    return true;
}

bool CustomRTMPPlatform::IsConnected() const {
    if (health_.connected) return true;
    return rtmp_ && rtmp_->IsConnected() && status_ == StreamStatus::Connected;
}

void CustomRTMPPlatform::SetConnected(bool connected) {
    if (connected) {
        status_ = StreamStatus::Connected;
        health_.connected = true;
    } else {
        status_ = StreamStatus::Stopped;
        health_.connected = false;
    }
}

bool CustomRTMPPlatform::PublishStream(const StreamConfig& config) {
    config_ = config;
    return true;
}

bool CustomRTMPPlatform::SendPacket(const PacketPtr& packet) {
    if (!IsConnected() || !packet) {
        return false;
    }

    const bool success = rtmp_->SendPacket(packet);
    if (!success && rtmp_) {
        STREAMX_WARN("Custom RTMP packet send failed: " + rtmp_->GetLastError());
    }
    return success;
}

void CustomRTMPPlatform::UpdateConfig(const StreamConfig& config) {
    config_ = config;
}

StreamHealth CustomRTMPPlatform::GetHealth() const {
    return health_;
}

StreamStatus CustomRTMPPlatform::GetStatus() const {
    return status_;
}

bool CustomRTMPPlatform::Reconnect() {
    STREAMX_INFO("Attempting to reconnect to custom RTMP platform: " + platform_name_);
    Disconnect();
    
    if (reconnect_callback_) {
        reconnect_callback_();
    }

    return Connect(Credentials{}, config_, nullptr);
}

void CustomRTMPPlatform::SetReconnectCallback(std::function<void()> callback) {
    reconnect_callback_ = callback;
}

void CustomRTMPPlatform::PacketSenderThread() {
    while (should_run_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        health_.bitrate_actual_kbps = rtmp_ ? static_cast<double>(rtmp_->GetBytesSent()) * 8 / 1000 / 1000 : 0;
        health_.last_packet_time = std::chrono::system_clock::now();
    }
}

}  // namespace streamx
