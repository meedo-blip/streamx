# StreamX Examples

## 1. Basic Streaming to Twitch

```cpp
#include "streamx/streaming_controller.h"
#include "streamx/utils/logger.h"
#include <iostream>

using namespace streamx;

int main() {
    // Initialize logging
    Logger::Instance().Initialize("streamx.log", LogLevel::Info);

    // Create controller
    StreamingController controller;

    // Configure video/audio
    VideoConfig video{
        .width = 1920,
        .height = 1080,
        .fps = 60,
        .bitrate_kbps = 6000,
        .codec = CodecType::H264,
        .preset = "medium"
    };

    AudioConfig audio{
        .sample_rate = 48000,
        .channels = 2,
        .bitrate_kbps = 128
    };

    // Initialize
    if (!controller.Initialize(video, audio)) {
        STREAMX_ERROR("Failed to initialize controller");
        return 1;
    }

    // Add Twitch platform
    controller.AddPlatform("twitch", "twitch");

    // Stream configuration
    StreamConfig stream_config{
        .video = video,
        .audio = audio,
        .stream_key = "live_123456_abcdefg"  // Your stream key
    };

    // Connect to Twitch
    Credentials creds;  // Can leave empty for Twitch
    if (!controller.ConnectAll(creds, stream_config)) {
        STREAMX_ERROR("Failed to connect to Twitch");
        return 1;
    }

    // Start capturing desktop
    if (!controller.StartCapture(CaptureFactory::CaptureType::Desktop)) {
        STREAMX_ERROR("Failed to start capture");
        return 1;
    }

    // Start streaming
    if (!controller.StartStreaming()) {
        STREAMX_ERROR("Failed to start streaming");
        return 1;
    }

    STREAMX_INFO("Streaming started!");
    std::cout << "Press Ctrl+C to stop..." << std::endl;

    // Keep running until interrupted
    while (controller.IsStreaming()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
```

## 2. Multi-Platform Streaming (Twitch + YouTube)

```cpp
#include "streamx/streaming_controller.h"
#include "streamx/utils/logger.h"

using namespace streamx;

int main() {
    Logger::Instance().Initialize("streamx.log");
    
    StreamingController controller;

    VideoConfig video{.width = 1280, .height = 720, .fps = 30};
    AudioConfig audio{.sample_rate = 44100, .channels = 2};
    
    controller.Initialize(video, audio);

    // Add multiple platforms
    controller.AddPlatform("twitch", "twitch");
    controller.AddPlatform("youtube", "youtube");
    controller.AddPlatform("streamlabs", "streamlabs");

    // Platform-specific stream keys
    std::map<std::string, std::string> keys{
        {"twitch", "live_123_abc"},
        {"youtube", "live_456_def"},
        {"streamlabs", "live_789_ghi"}
    };

    // Connect each platform
    for (const auto& [name, key] : keys) {
        StreamConfig config{.stream_key = key};
        auto platform = controller.GetPlatformManager()->GetPlatform(name);
        // Connect logic would go here
    }

    // Start streaming
    controller.StartCapture();
    controller.StartStreaming();

    // Monitor all platforms
    for (int i = 0; i < 300; ++i) {  // Run for 5 minutes
        auto health_map = controller.GetPlatformHealth();

        std::cout << "=== Stream Status ===\n";
        for (const auto& [name, health] : health_map) {
            std::cout << name << ":\n";
            std::cout << "  Status: " << (health.connected ? "Connected" : "Disconnected") << "\n";
            std::cout << "  Bitrate: " << health.bitrate_actual_kbps << " kbps\n";
            std::cout << "  Latency: " << health.network_latency_ms << " ms\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    controller.StopStreaming();
    return 0;
}
```

## 3. Streaming with Performance Monitoring

```cpp
#include "streamx/streaming_controller.h"
#include "streamx/utils/logger.h"
#include <chrono>
#include <iomanip>

using namespace streamx;

void PrintStreamingStats(const StreamingController& controller) {
    auto health = controller.GetOverallHealth();
    auto platform_health = controller.GetPlatformHealth();

    std::cout << "\n=== Streaming Statistics ===\n";
    std::cout << "Overall Connection: " << (health.connected ? "Yes" : "No") << "\n";
    std::cout << "Average Latency: " << health.network_latency_ms << "ms\n";
    std::cout << "Average Bitrate: " << health.bitrate_actual_kbps << " kbps\n\n";

    std::cout << std::left << std::setw(20) << "Platform"
              << std::setw(15) << "Connected"
              << std::setw(15) << "Bitrate (kbps)"
              << std::setw(15) << "Latency (ms)"
              << std::setw(10) << "Dropped\n";
    std::cout << std::string(75, '-') << "\n";

    for (const auto& [name, h] : platform_health) {
        std::cout << std::left 
                  << std::setw(20) << name
                  << std::setw(15) << (h.connected ? "Yes" : "No")
                  << std::setw(15) << std::fixed << std::setprecision(1) << h.bitrate_actual_kbps
                  << std::setw(15) << h.network_latency_ms
                  << std::setw(10) << h.frames_dropped << "\n";
    }
}

int main() {
    Logger::Instance().Initialize("streamx.log");

    StreamingController controller;
    VideoConfig video{.width = 1920, .height = 1080, .fps = 60, .bitrate_kbps = 5000};
    AudioConfig audio{.sample_rate = 48000, .channels = 2};

    controller.Initialize(video, audio);
    controller.AddPlatform("twitch", "twitch");
    controller.AddPlatform("youtube", "youtube");

    // Connect platforms (connection logic omitted)
    
    controller.StartCapture();
    controller.StartStreaming();

    // Print stats every 5 seconds
    auto start_time = std::chrono::high_resolution_clock::now();
    while (controller.IsStreaming()) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();

        if (elapsed % 5 == 0) {
            PrintStreamingStats(controller);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Stop after 10 minutes for this example
        if (elapsed > 600) {
            break;
        }
    }

    controller.StopStreaming();
    return 0;
}
```

## 4. Custom Platform Implementation

```cpp
#include "streamx/platform/streaming_platform.h"
#include "streamx/utils/logger.h"

using namespace streamx;

class CustomLiveStreamPlatform : public IStreamingPlatform {
public:
    CustomLiveStreamPlatform() : status_(StreamStatus::Idle) {}

    bool Connect(const Credentials& creds, const StreamConfig& config) override {
        config_ = config;
        // Custom connection logic
        STREAMX_INFO("Connecting to custom platform with server: " + config.server_url);
        status_ = StreamStatus::Connected;
        health_.connected = true;
        return true;
    }

    bool Disconnect() override {
        status_ = StreamStatus::Stopped;
        health_.connected = false;
        return true;
    }

    bool IsConnected() const override {
        return status_ == StreamStatus::Connected;
    }

    bool PublishStream(const StreamConfig& config) override {
        config_ = config;
        return true;
    }

    bool SendPacket(const PacketPtr& packet) override {
        if (!IsConnected() || !packet) return false;
        
        // Custom packet sending logic
        bytes_sent_ += packet->GetSize();
        packets_sent_++;
        return true;
    }

    void UpdateConfig(const StreamConfig& config) override {
        config_ = config;
    }

    StreamHealth GetHealth() const override {
        StreamHealth h = health_;
        h.bitrate_actual_kbps = static_cast<double>(bytes_sent_) * 8 / 1000;
        return h;
    }

    StreamStatus GetStatus() const override { return status_; }
    bool Reconnect() override { return Connect(Credentials{}, config_); }
    void SetReconnectCallback(std::function<void()> callback) override {
        reconnect_callback_ = callback;
    }

private:
    StreamConfig config_;
    StreamStatus status_;
    StreamHealth health_;
    std::function<void()> reconnect_callback_;
    uint64_t bytes_sent_ = 0;
    uint64_t packets_sent_ = 0;
};

int main() {
    StreamingController controller;
    VideoConfig video{.width = 1920, .height = 1080};
    AudioConfig audio{};
    
    controller.Initialize(video, audio);

    // Add custom platform
    controller.AddPlatform("custom", std::make_unique<CustomLiveStreamPlatform>());

    // Use it like any other platform
    StreamConfig config{.server_url = "rtmp://custom.server/live"};
    controller.ConnectAll(Credentials{}, config);
    controller.StartCapture();
    controller.StartStreaming();

    return 0;
}
```

## 5. Window Capture and Streaming

```cpp
#include "streamx/streaming_controller.h"

using namespace streamx;

int main() {
    Logger::Instance().Initialize();

    StreamingController controller;
    VideoConfig video{.width = 1920, .height = 1080, .fps = 30};
    AudioConfig audio{};

    controller.Initialize(video, audio);
    controller.AddPlatform("twitch", "twitch");

    // Capture specific window instead of desktop
    if (!controller.StartCapture(
        CaptureFactory::CaptureType::Window,
        "Game Window Title")) {
        STREAMX_ERROR("Failed to capture window");
        return 1;
    }

    // StreamConfig and connection...
    controller.StartStreaming();

    // Stream until stopped
    while (controller.IsStreaming()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
```

## 6. Configuration Management

```cpp
#include "streamx/utils/config.h"
#include "streamx/utils/logger.h"

using namespace streamx;

int main() {
    Logger::Instance().Initialize();
    Config& config = Config::Instance();

    // Load configuration
    if (config.LoadFromFile("streamx_config.json")) {
        auto width = config.Get<int>("video.width", 1920);
        auto height = config.Get<int>("video.height", 1080);
        auto fps = config.Get<int>("video.fps", 60);
        auto bitrate = config.Get<int>("video.bitrate_kbps", 5000);

        std::cout << "Loaded config: " << width << "x" << height 
                  << " @ " << fps << "fps, " << bitrate << " kbps\n";
    } else {
        STREAMX_WARN("Config file not found, using defaults");

        // Set defaults
        config.Set("video.width", 1920);
        config.Set("video.height", 1080);
        config.Set("video.fps", 60);
        config.Set("video.bitrate_kbps", 5000);

        config.Set("audio.sample_rate", 48000);
        config.Set("audio.channels", 2);
        config.Set("audio.bitrate_kbps", 128);

        // Save configuration
        config.SaveToFile("streamx_config.json");
    }

    return 0;
}
```

## 7. Error Handling and Recovery

```cpp
#include "streamx/streaming_controller.h"

using namespace streamx;

int main() {
    Logger::Instance().Initialize("streamx.log");

    StreamingController controller;
    VideoConfig video{.width = 1920, .height = 1080};
    AudioConfig audio{};

    try {
        if (!controller.Initialize(video, audio)) {
            throw std::runtime_error("Failed to initialize controller");
        }

        controller.AddPlatform("twitch", "twitch");

        // Attempt connection with retry logic
        int retry_count = 0;
        bool connected = false;

        while (!connected && retry_count < 3) {
            try {
                StreamConfig config{.stream_key = "your-key"};
                if (controller.ConnectAll(Credentials{}, config)) {
                    connected = true;
                } else {
                    throw std::runtime_error("Connection failed");
                }
            } catch (const std::exception& e) {
                STREAMX_WARN("Connection attempt " + std::to_string(retry_count + 1) + 
                            " failed: " + std::string(e.what()));
                retry_count++;
                std::this_thread::sleep_for(std::chrono::seconds(5 * (retry_count + 1)));
            }
        }

        if (!connected) {
            throw std::runtime_error("Failed to connect after 3 attempts");
        }

        controller.StartCapture();
        controller.StartStreaming();

        // Monitor for disconnections
        while (controller.IsStreaming()) {
            auto health = controller.GetOverallHealth();
            
            if (!health.connected) {
                STREAMX_WARN("Stream disconnected, attempting reconnection...");
                // Reconnect logic would go here
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    } catch (const std::exception& e) {
        STREAMX_CRITICAL("Fatal error: " + std::string(e.what()));
        controller.StopStreaming();
        return 1;
    }

    return 0;
}
```

---

For more information, see the [API Documentation](API.md) and [Build Instructions](BUILD.md).
