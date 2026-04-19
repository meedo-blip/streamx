#pragma once

#include "streaming_platform.h"
#include "../utils/thread_pool.h"
#include <queue>
#include <thread>
#include <memory>

namespace streamx {

class RTMPHandler;

// Twitch streaming platform
class TwitchPlatform : public IStreamingPlatform {
public:
    TwitchPlatform();
    ~TwitchPlatform() override;

    bool Connect(const Credentials& creds, const StreamConfig& config,
                 const AVCodecContext* video_codec_context = nullptr) override;
    bool Disconnect() override;
    bool IsConnected() const override;

    bool PublishStream(const StreamConfig& config) override;
    bool SendPacket(const PacketPtr& packet) override;

    void UpdateConfig(const StreamConfig& config) override;
    StreamHealth GetHealth() const override;
    StreamStatus GetStatus() const override;

    bool Reconnect() override;
    void SetReconnectCallback(std::function<void()> callback) override;
    void SetConnected(bool connected) override;

private:
    std::unique_ptr<RTMPHandler> rtmp_;
    StreamConfig config_;
    StreamStatus status_ = StreamStatus::Idle;
    StreamHealth health_;
    std::function<void()> reconnect_callback_;
    std::thread packet_sender_;
    bool should_run_ = false;

    void PacketSenderThread();
};

// YouTube streaming platform
class YouTubePlatform : public IStreamingPlatform {
public:
    YouTubePlatform();
    ~YouTubePlatform() override;

    bool Connect(const Credentials& creds, const StreamConfig& config,
                 const AVCodecContext* video_codec_context = nullptr) override;
    bool Disconnect() override;
    bool IsConnected() const override;

    bool PublishStream(const StreamConfig& config) override;
    bool SendPacket(const PacketPtr& packet) override;

    void UpdateConfig(const StreamConfig& config) override;
    StreamHealth GetHealth() const override;
    StreamStatus GetStatus() const override;

    bool Reconnect() override;
    void SetReconnectCallback(std::function<void()> callback) override;
    void SetConnected(bool connected) override;

private:
    std::unique_ptr<RTMPHandler> rtmp_;
    StreamConfig config_;
    StreamStatus status_ = StreamStatus::Idle;
    StreamHealth health_;
    std::function<void()> reconnect_callback_;
    std::thread packet_sender_;
    bool should_run_ = false;

    void PacketSenderThread();
};

// StreamLabs streaming platform
class StreamLabsPlatform : public IStreamingPlatform {
public:
    StreamLabsPlatform();
    ~StreamLabsPlatform() override;

    bool Connect(const Credentials& creds, const StreamConfig& config,
                 const AVCodecContext* video_codec_context = nullptr) override;
    bool Disconnect() override;
    bool IsConnected() const override;

    bool PublishStream(const StreamConfig& config) override;
    bool SendPacket(const PacketPtr& packet) override;

    void UpdateConfig(const StreamConfig& config) override;
    StreamHealth GetHealth() const override;
    StreamStatus GetStatus() const override;

    bool Reconnect() override;
    void SetReconnectCallback(std::function<void()> callback) override;
    void SetConnected(bool connected) override;

private:
    std::unique_ptr<RTMPHandler> rtmp_;
    StreamConfig config_;
    StreamStatus status_ = StreamStatus::Idle;
    StreamHealth health_;
    std::function<void()> reconnect_callback_;
    std::thread packet_sender_;
    bool should_run_ = false;

    void PacketSenderThread();
};

// Generic custom RTMP platform
class CustomRTMPPlatform : public IStreamingPlatform {
public:
    explicit CustomRTMPPlatform(const std::string& platform_name);
    ~CustomRTMPPlatform() override;

    bool Connect(const Credentials& creds, const StreamConfig& config,
                 const AVCodecContext* video_codec_context = nullptr) override;
    bool Disconnect() override;
    bool IsConnected() const override;

    bool PublishStream(const StreamConfig& config) override;
    bool SendPacket(const PacketPtr& packet) override;

    void UpdateConfig(const StreamConfig& config) override;
    StreamHealth GetHealth() const override;
    StreamStatus GetStatus() const override;

    bool Reconnect() override;
    void SetReconnectCallback(std::function<void()> callback) override;
    void SetConnected(bool connected) override;

private:
    std::string platform_name_;
    std::unique_ptr<RTMPHandler> rtmp_;
    StreamConfig config_;
    StreamStatus status_ = StreamStatus::Idle;
    StreamHealth health_;
    std::function<void()> reconnect_callback_;
    std::thread packet_sender_;
    bool should_run_ = false;

    void PacketSenderThread();
};

}  // namespace streamx
