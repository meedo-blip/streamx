#include "streamx/platform/streaming_platform.h"
#include "streamx/utils/logger.h"

namespace streamx {

StreamingPlatformManager::StreamingPlatformManager() {
    STREAMX_INFO("StreamingPlatformManager created");
}

StreamingPlatformManager::~StreamingPlatformManager() {
    DisconnectAll();
}

bool StreamingPlatformManager::AddPlatform(const std::string& name, std::unique_ptr<IStreamingPlatform> platform) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (platforms_.find(name) != platforms_.end()) {
        STREAMX_WARN("Platform already exists: " + name);
        return false;
    }

    platforms_[name] = std::move(platform);
    STREAMX_INFO("Platform added: " + name);
    return true;
}

bool StreamingPlatformManager::RemovePlatform(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = platforms_.find(name);
    if (it == platforms_.end()) {
        STREAMX_WARN("Platform not found: " + name);
        return false;
    }

    platforms_.erase(it);
    STREAMX_INFO("Platform removed: " + name);
    return true;
}

IStreamingPlatform* StreamingPlatformManager::GetPlatform(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = platforms_.find(name);
    if (it == platforms_.end()) {
        return nullptr;
    }

    return it->second.get();
}

bool StreamingPlatformManager::BroadcastPacket(const PacketPtr& packet) {
    std::lock_guard<std::mutex> lock(mutex_);

    bool success = true;
    for (auto& [name, platform] : platforms_) {
        if (platform && platform->IsConnected()) {
            if (!platform->SendPacket(packet)) {
                STREAMX_WARN("Failed to send packet to platform: " + name);
                success = false;
            }
        }
    }

    return success;
}

std::map<std::string, StreamHealth> StreamingPlatformManager::GetAllPlatformHealth() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::map<std::string, StreamHealth> health_map;
    for (const auto& [name, platform] : platforms_) {
        if (platform) {
            health_map[name] = platform->GetHealth();
        }
    }

    return health_map;
}

bool StreamingPlatformManager::ConnectAll(const Credentials& creds, const StreamConfig& config,
                                          const AVCodecContext* video_codec_context) {
    std::lock_guard<std::mutex> lock(mutex_);

    bool all_success = true;
    for (auto& [name, platform] : platforms_) {
        if (!platform->Connect(creds, config, video_codec_context)) {
            STREAMX_ERROR("Failed to connect platform: " + name);
            all_success = false;
        }
    }

    return all_success;
}

bool StreamingPlatformManager::DisconnectAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [name, platform] : platforms_) {
        if (platform && platform->IsConnected()) {
            platform->Disconnect();
        }
    }

    return true;
}

std::vector<std::string> StreamingPlatformManager::GetConnectedPlatforms() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> connected;
    for (const auto& [name, platform] : platforms_) {
        if (platform && platform->IsConnected()) {
            connected.push_back(name);
        }
    }

    return connected;
}

std::vector<std::string> StreamingPlatformManager::GetAllPlatforms() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> all;
    for (const auto& [name, platform] : platforms_) {
        all.push_back(name);
    }

    return all;
}

}  // namespace streamx
