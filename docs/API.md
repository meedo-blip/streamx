# StreamX C++ API Documentation

## Core Classes

### StreamingController

Main controller for managing streaming operations.

```cpp
class StreamingController {
public:
    // Initialization
    bool Initialize(const VideoConfig& video_config, const AudioConfig& audio_config);

    // Platform management
    bool AddPlatform(const std::string& name, const std::string& platform_type);
    bool ConnectAll(const Credentials& creds, const StreamConfig& config);
    bool DisconnectAll();

    // Capture control
    bool StartCapture(CaptureFactory::CaptureType type = CaptureFactory::CaptureType::Desktop,
                     const std::string& source = "");
    bool StopCapture();
    bool IsCapturing() const;

    // Streaming control
    bool StartStreaming();
    bool StopStreaming();
    bool IsStreaming() const;

    // Configuration
    void UpdateVideoConfig(const VideoConfig& config);
    void UpdateAudioConfig(const AudioConfig& config);

    // Monitoring
    StreamHealth GetOverallHealth() const;
    std::map<std::string, StreamHealth> GetPlatformHealth() const;
    std::vector<std::string> GetConnectedPlatforms() const;
};
```

### Frame

Represents audio or video frame with FFmpeg integration.

```cpp
class Frame {
public:
    // Video frame constructor
    Frame(uint32_t width, uint32_t height, PixelFormat format);
    
    // Audio frame constructor
    Frame(uint32_t sample_rate, uint32_t channels, SampleFormat format, uint32_t samples);

    // Accessors
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint8_t** GetData() const;
    int* GetLinesize() const;
    
    // Timestamps
    void SetPTS(int64_t pts);
    int64_t GetPTS() const;
    void SetDTS(int64_t dts);
    int64_t GetDTS() const;

    bool IsVideo() const;
    bool IsAudio() const;
};
```

### Packet

Encoded media packet.

```cpp
class Packet {
public:
    explicit Packet(uint32_t capacity = 65536);

    uint8_t* GetData();
    const uint8_t* GetData() const;
    uint32_t GetSize() const;

    bool IsKeyframe() const;
    void SetKeyframe(bool key);

    void SetData(const uint8_t* data, uint32_t size);
    void Clear();

    int64_t GetPTS() const;
    int64_t GetDTS() const;
};
```

### IEncoder

Base encoder interface.

```cpp
class IEncoder {
public:
    virtual bool Initialize(const VideoConfig& config) = 0;
    virtual bool Encode(const FramePtr& frame, PacketPtr& output_packet) = 0;
    virtual bool Flush(PacketPtr& output_packet) = 0;
    virtual void Close() = 0;

    virtual double GetEncodingLatencyMs() const = 0;
    virtual double GetCPUUsage() const = 0;
};
```

### ICapture

Base capture interface.

```cpp
class ICapture {
public:
    virtual bool Initialize(const VideoConfig& video_cfg, const AudioConfig& audio_cfg) = 0;
    virtual bool Start() = 0;
    virtual bool Stop() = 0;
    virtual FramePtr CaptureFrame(std::chrono::milliseconds timeout) = 0;
    virtual void Close() = 0;

    virtual double GetCaptureLatencyMs() const = 0;
    virtual StreamHealth GetHealth() const = 0;
};
```

### IStreamingPlatform

Base streaming platform interface.

```cpp
class IStreamingPlatform {
public:
    virtual bool Connect(const Credentials& creds, const StreamConfig& config) = 0;
    virtual bool Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    virtual bool PublishStream(const StreamConfig& config) = 0;
    virtual bool SendPacket(const PacketPtr& packet) = 0;

    virtual void UpdateConfig(const StreamConfig& config) = 0;
    virtual StreamHealth GetHealth() const = 0;
    virtual StreamStatus GetStatus() const = 0;

    virtual bool Reconnect() = 0;
    virtual void SetReconnectCallback(std::function<void()> callback) = 0;
};
```

### TwitchPlatform, YouTubePlatform, StreamLabsPlatform

Platform-specific implementations of `IStreamingPlatform`.

```cpp
class TwitchPlatform : public IStreamingPlatform { /* ... */ };
class YouTubePlatform : public IStreamingPlatform { /* ... */ };
class StreamLabsPlatform : public IStreamingPlatform { /* ... */ };
```

### FrameBuffer

Thread-safe ring buffer for frames/packets.

```cpp
template<typename T>
class FrameBuffer {
public:
    explicit FrameBuffer(uint32_t capacity = 120);

    bool TryPush(std::shared_ptr<T> item);
    bool Push(std::shared_ptr<T> item, std::chrono::milliseconds timeout);
    std::shared_ptr<T> Pop(std::chrono::milliseconds timeout);
    std::shared_ptr<T> TryPop();

    uint32_t GetSize() const;
    bool Empty() const;
    bool Full() const;
};

// Common instantiations
using FrameBufferVideo = FrameBuffer<Frame>;
using PacketBuffer = FrameBuffer<Packet>;
```

### Logger

Singleton logger with file and console output.

```cpp
class Logger {
public:
    static Logger& Instance();

    void Initialize(const std::string& log_file = "", LogLevel level = LogLevel::Info);
    void SetLevel(LogLevel level);

    void Debug(const std::string& message);
    void Info(const std::string& message);
    void Warning(const std::string& message);
    void Error(const std::string& message);
    void Critical(const std::string& message);

    void Debugf(const char* fmt, ...);
    void Infof(const char* fmt, ...);
    // ... other format functions
};

// Convenience macros
#define STREAMX_DEBUG(msg) streamx::Logger::Instance().Debug(msg)
#define STREAMX_INFO(msg) streamx::Logger::Instance().Info(msg)
#define STREAMX_WARN(msg) streamx::Logger::Instance().Warning(msg)
#define STREAMX_ERROR(msg) streamx::Logger::Instance().Error(msg)
```

### Config

JSON-based configuration manager.

```cpp
class Config {
public:
    static Config& Instance();

    bool LoadFromFile(const std::string& filename);
    bool SaveToFile(const std::string& filename) const;

    template<typename T>
    T Get(const std::string& key, const T& default_value = T()) const;
    
    template<typename T>
    void Set(const std::string& key, const T& value);

    bool Has(const std::string& key) const;
    void Remove(const std::string& key);
    void Clear();
};
```

### ThreadPool

Work-stealing thread pool for async operations.

```cpp
class ThreadPool {
public:
    explicit ThreadPool(uint32_t num_threads = 4);

    template<typename Func, typename... Args>
    void Submit(Func&& func, Args&&... args);

    void WaitAll();
    void Shutdown();

    uint32_t GetThreadCount() const;
    uint32_t GetPendingTasks() const;
};
```

## Data Types

### VideoConfig

```cpp
struct VideoConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t fps = 60;
    uint32_t bitrate_kbps = 5000;
    CodecType codec = CodecType::H264;
    PixelFormat pixel_format = PixelFormat::YUV420P;
    bool enable_bframes = true;
    uint32_t gop_size = 60;
    std::string preset = "medium";
};
```

### AudioConfig

```cpp
struct AudioConfig {
    uint32_t sample_rate = 48000;
    uint32_t channels = 2;
    uint32_t bitrate_kbps = 128;
    AudioCodecType codec = AudioCodecType::AAC;
    SampleFormat sample_format = SampleFormat::S16;
};
```

### StreamConfig

```cpp
struct StreamConfig {
    VideoConfig video;
    AudioConfig audio;
    std::string stream_key;
    std::string server_url;
    uint32_t max_buffer_frames = 120;
};
```

### StreamHealth

```cpp
struct StreamHealth {
    bool connected = false;
    double cpu_usage = 0.0;
    double gpu_usage = 0.0;
    double network_latency_ms = 0.0;
    double bitrate_actual_kbps = 0.0;
    uint32_t frames_dropped = 0;
    uint32_t reconnect_count = 0;
    std::chrono::system_clock::time_point last_packet_time;
};
```

### Credentials

```cpp
struct Credentials {
    std::string username;
    std::string password;
    std::string token;
    std::string client_id;
    std::string client_secret;
    std::string channel_id;
};
```

## Enumerations

### CodecType
- `H264` - H.264/AVC
- `H265` - H.265/HEVC
- `VP8` - VP8
- `VP9` - VP9

### AudioCodecType
- `AAC` - Advanced Audio Coding
- `OPUS` - Opus Codec
- `MP3` - MPEG-3

### PixelFormat
- `YUV420P` - YUV 4:2:0 planar (standard)
- `NV12` - NVIDIA common format
- `RGB24` - RGB 24-bit
- `RGBA32` - RGBA 32-bit

### SampleFormat
- `S16` - 16-bit signed
- `S32` - 32-bit signed
- `FLT` - 32-bit float
- `FLTP` - 32-bit float planar

### StreamStatus
- `Idle` - Not streaming
- `Connecting` - Attempting connection
- `Connected` - Active stream
- `Reconnecting` - Recovering from connection loss
- `Failed` - Connection failed
- `Stopped` - Streaming stopped

### LogLevel
- `Debug`
- `Info`
- `Warning`
- `Error`
- `Critical`

## Common Patterns

### Basic Streaming

```cpp
#include "streamx/streaming_controller.h"
using namespace streamx;

StreamingController controller;

// Initialize
VideoConfig video{.width = 1280, .height = 720, .fps = 30};
AudioConfig audio{};
controller.Initialize(video, audio);

// Add platform
controller.AddPlatform("twitch", "twitch");

// Connect
StreamConfig config{.stream_key = "key"};
controller.ConnectAll(Credentials{}, config);

// Stream
controller.StartCapture();
controller.StartStreaming();

// ... streaming happens ...

controller.StopStreaming();
```

### Custom Platform

```cpp
class MyPlatform : public IStreamingPlatform {
    // Implement required methods
    bool Connect(...) override { /* ... */ return true; }
    bool SendPacket(const PacketPtr& packet) override { /* send */ return true; }
    // ... other methods
};

controller.AddPlatform("custom", std::make_unique<MyPlatform>());
```

### Monitoring Streams

```cpp
while (controller.IsStreaming()) {
    auto health_map = controller.GetPlatformHealth();
    for (const auto& [name, health] : health_map) {
        std::cout << name << ": " << health.bitrate_actual_kbps << " kbps\n";
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
```

## Signal Handling

```cpp
#include <signal.h>

static StreamingController* g_controller = nullptr;

void SignalHandler(int sig) {
    if (g_controller) {
        g_controller->StopStreaming();
    }
}

int main() {
    StreamingController controller;
    g_controller = &controller;
    
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    
    // ... streaming code ...
}
```

---

For more examples, see [EXAMPLES.md](EXAMPLES.md)
