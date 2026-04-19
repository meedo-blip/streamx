#pragma once

#include <memory>
#include <string>
#include <functional>
#include <map>
#include <vector>
#include <mutex>
#include "streamx/core/types.h"
#include "streamx/core/frame.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace streamx {

// Base streaming platform interface
class IStreamingPlatform {
public:
    virtual ~IStreamingPlatform() = default;

    virtual bool Connect(const Credentials& creds, const StreamConfig& config,
                         const AVCodecContext* video_codec_context = nullptr) = 0;
    virtual bool Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    virtual bool PublishStream(const StreamConfig& config) = 0;
    virtual bool SendPacket(const PacketPtr& packet) = 0;

    virtual void UpdateConfig(const StreamConfig& config) = 0;
    virtual StreamHealth GetHealth() const = 0;
    virtual StreamStatus GetStatus() const = 0;

    virtual bool Reconnect() = 0;
    virtual void SetReconnectCallback(std::function<void()> callback) = 0;
    virtual void SetConnected(bool connected) = 0;
};

// Streaming platform manager - handles multi-platform streaming
class StreamingPlatformManager {
public:
    StreamingPlatformManager();
    ~StreamingPlatformManager();

    // Register a platform
    bool AddPlatform(const std::string& name, std::unique_ptr<IStreamingPlatform> platform);
    bool RemovePlatform(const std::string& name);
    IStreamingPlatform* GetPlatform(const std::string& name);

    // Send packet to all connected platforms
    bool BroadcastPacket(const PacketPtr& packet);
    
    // Get health of all platforms
    std::map<std::string, StreamHealth> GetAllPlatformHealth() const;
    
    // Connect all platforms
    bool ConnectAll(const Credentials& creds, const StreamConfig& config,
                    const AVCodecContext* video_codec_context = nullptr);
    bool DisconnectAll();

    // Get list of connected platforms
    std::vector<std::string> GetConnectedPlatforms() const;
    std::vector<std::string> GetAllPlatforms() const;

private:
    std::map<std::string, std::unique_ptr<IStreamingPlatform>> platforms_;
    mutable std::mutex mutex_;
};

}  // namespace streamx
