# StreamX - Multi-Platform Streaming Application

A professional-grade C++ application for streaming video and audio to multiple platforms simultaneously (Twitch, YouTube, StreamLabs, and custom RTMP servers).

## Features

- **Multi-Platform Streaming**: Stream to Twitch, YouTube, StreamLabs, and custom RTMP servers simultaneously
- **Hardware Encoding**: NVIDIA NVENC support with fallback to software encoding
- **Flexible Capture**: Desktop capture, window capture, and audio capture
- **Thread-Safe Architecture**: Lock-free ring buffers for high-performance streaming
- **Real-Time Monitoring**: Health metrics and performance monitoring for each platform
- **Reconnection Logic**: Automatic reconnection with exponential backoff
- **Configuration Management**: JSON-based configuration system
- **Comprehensive Logging**: Detailed logging to file and console

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    StreamX Application                      │
├─────────────────────────────────────────────────────────────┤
│  Streaming Controller (Orchestration & Coordination)        │
├─────────────────────────────────────────────────────────────┤
│  Capture Layer (Desktop/Window/Audio)                       │
│  ↓                                                           │
│  Encoding Layer (H.264/H.265 with GPU support)             │
│  ↓                                                           │
│  Frame Buffer (Lock-free ring buffer, ~2s @ 60fps)         │
│  ↓                                                           │
│  Platform Manager (Multi-platform packet distribution)      │
├─────────────────────────────────────────────────────────────┤
│  Platform Implementations                                   │
│  ├─ Twitch (RTMP)                                          │
│  ├─ YouTube (RTMP)                                         │
│  ├─ StreamLabs (RTMP wrapper)                              │
│  └─ Custom RTMP Servers                                    │
├─────────────────────────────────────────────────────────────┤
│  Utilities                                                  │
│  ├─ Logger (File + Console)                                │
│  ├─ Config Manager (JSON settings)                         │
│  ├─ Thread Pool (Async task execution)                     │
│  └─ RTMP Handler (Protocol implementation)                 │
└─────────────────────────────────────────────────────────────┘
```

## Directory Structure

```
streamx/
├── CMakeLists.txt              # Build configuration
├── include/streamx/            # Public headers
│   ├── core/                   # Core abstractions
│   │   ├── types.h             # Common types and configurations
│   │   ├── frame.h             # Frame and packet classes
│   │   ├── buffer.h            # Ring buffer implementation
│   │   ├── encoder.h           # Encoder interface & implementations
│   │   └── capture.h           # Capture interface & implementations
│   ├── platform/               # Platform abstraction
│   │   ├── streaming_platform.h
│   │   ├── platform_implementations.h
│   │   └── rtmp_handler.h
│   └── utils/                  # Utility classes
│       ├── logger.h
│       ├── config.h
│       └── thread_pool.h
├── src/
│   ├── core/                   # Core implementations
│   ├── platform/               # Platform implementations
│   ├── platforms/              # Platform-specific code
│   ├── utils/                  # Utility implementations
│   ├── streaming_controller.cpp # Main controller
│   └── main.cpp                # CLI application
├── tests/                      # Unit tests
├── docs/                       # Documentation
└── build/                      # Build output (generated)
```

## Building

### Requirements
- CMake 3.16+
- C++17 compiler (GCC, Clang, MSVC)
- FFmpeg libraries (libavcodec, libavformat, libavutil, libswscale)
- nlohmann/json (included or installed)
- NVIDIA CUDA SDK (optional, for NVENC support)

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt-get install cmake build-essential libavcodec-dev libavformat-dev \
    libavutil-dev libswscale-dev nlohmann-json3-dev

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run tests
make test

# Create executable
./streamx --help
```

### macOS

```bash
# Install dependencies
brew install cmake ffmpeg nlohmann-json

# Build
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.physicalcpu)
```

### Windows (Visual Studio)

```bash
# Using vcpkg for dependencies
vcpkg install ffmpeg:x64-windows nlohmann-json:x64-windows

# Build
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

## Usage

### Basic Commands

```bash
# Show help
./streamx help

# Add Twitch platform (requires stream key)
./streamx add-platform twitch "your-stream-key"

# Add YouTube platform
./streamx add-platform youtube "your-stream-key"

# Add StreamLabs
./streamx add-platform streamlabs "your-stream-key"

# Start streaming to all added platforms
./streamx start

# Show current status
./streamx status

# View real-time monitoring dashboard
./streamx monitor

# Stop current stream
./streamx stop

# List connected platforms
./streamx list-platforms

# Manage configuration
./streamx config save    # Save current config
./streamx config load    # Load saved config
./streamx config show    # Display current config
```

## Configuration

StreamX uses JSON configuration files for settings. Example `streamx_config.json`:

```json
{
    "video": {
        "width": 1920,
        "height": 1080,
        "fps": 60,
        "bitrate_kbps": 5000,
        "codec": "H264",
        "preset": "medium"
    },
    "audio": {
        "sample_rate": 48000,
        "channels": 2,
        "bitrate_kbps": 128,
        "codec": "AAC"
    },
    "platforms": {
        "twitch": {
            "stream_key": "your-key-here"
        },
        "youtube": {
            "stream_key": "your-key-here"
        }
    }
}
```

## API Usage

### C++ Streaming Library

```cpp
#include "streamx/streaming_controller.h"

using namespace streamx;

int main() {
    // Initialize logger
    Logger::Instance().Initialize("streamx.log");

    // Create controller
    StreamingController controller;

    // Configure
    VideoConfig video{.width = 1920, .height = 1080, .fps = 60};
    AudioConfig audio{.sample_rate = 48000, .channels = 2};
    
    controller.Initialize(video, audio);

    // Add platforms
    controller.AddPlatform("twitch", "twitch");
    controller.AddPlatform("youtube", "youtube");

    // Connect to platforms
    StreamConfig config{.stream_key = "your-key"};
    Credentials creds;
    controller.ConnectAll(creds, config);

    // Start streaming
    controller.StartCapture();
    controller.StartStreaming();

    // Monitor
    while (controller.IsStreaming()) {
        auto health = controller.GetPlatformHealth();
        for (const auto& [name, h] : health) {
            std::cout << name << ": " << h.bitrate_actual_kbps << " kbps\n";
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
```

## Performance Tuning

### Encoding Presets
- `ultrafast` - Lowest latency, highest CPU usage
- `superfast` - Very fast, ~50ms latency
- `veryfast` - Fast, ~100ms latency
- `faster` - Moderate, ~150ms latency
- `fast` - Balanced, ~200ms latency
- `medium` - Default, ~250ms latency (recommended)
- `slow` - High quality, ~500ms latency
- `slower` - Best quality, ~1s latency

### Buffer Configuration
- `max_buffer_frames`: 60-120 recommended (1-2 seconds at 60fps)
- Larger buffers tolerate network jitter but increase latency

### Hardware Encoding
- Enable NVENC for NVIDIA GPUs (requires CUDA SDK)
- Significantly reduces CPU usage (20% vs 80%)
- Lower latency and better quality

## Monitoring Commands

```bash
# Real-time dashboard (updates every 2 seconds)
./streamx monitor

# Platform-specific health metrics
./streamx status

# Connected platforms list
./streamx list-platforms
```

## Troubleshooting

### Unable to find FFmpeg
```bash
# Set FFmpeg path
export PKG_CONFIG_PATH=/path/to/ffmpeg/lib/pkgconfig:$PKG_CONFIG_PATH
cmake ..
```

### High CPU usage
- Use software encoding preset `fast` or `faster`
- Enable GPU encoding (NVENC)
- Reduce video resolution or fps
- Use adaptive bitrate

### Connection drops
- Check network stability
- Increase buffer size
- Reduce bitrate
- Enable automatic reconnection (default: enabled)

### Audio/Video sync issues
- Verify timestamp generation
- Adjust encoder GopSize (default: 60 frames)
- Check platform audio settings

## Development

### Project Structure

#### Core Components
- **Frame**: Audio/video frame abstraction with FFmpeg integration
- **Encoder**: H.264/H.265 software and GPU encoding
- **Capture**: Desktop, window, and audio capture implementations
- **Buffer**: Thread-safe ring buffer for frame/packet management

#### Platform Abstraction
- **IStreamingPlatform**: Base interface for streaming platforms
- **RTMPHandler**: RTMP protocol handler
- **Platform Implementations**: Twitch, YouTube, StreamLabs
- **StreamingPlatformManager**: Multi-platform coordination

#### Utilities
- **Logger**: Async logging with file/console output
- **Config**: JSON-based configuration management
- **ThreadPool**: Work-stealing thread pool for async tasks

### Adding a Custom Platform

```cpp
class MyCustomPlatform : public IStreamingPlatform {
public:
    bool Connect(const Credentials& creds, const StreamConfig& config) override {
        // Implement connection logic
        return true;
    }

    bool SendPacket(const PacketPtr& packet) override {
        // Send encoded packet to platform
        return true;
    }

    // Implement other virtual methods...
};

// Register with controller
controller.AddPlatform("my-platform", std::make_unique<MyCustomPlatform>());
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## Performance Metrics

Typical performance on modern hardware:

| Metric | Value |
|--------|-------|
| Encoding latency | 8-12ms (H.264, 1080p60) |
| Buffer latency | 33-66ms (ring buffer) |
| Network latency | 500-3000ms (platform dependent) |
| Total latency | 1-5 seconds|
| CPU usage (software) | 40-80% (single core) |
| CPU usage (GPU) | 5-10% |
| Memory usage | 100-200MB |

## License

Proprietary - StreamX Streaming Application

## Support

For issues, feature requests, or documentation, visit the project repository.

---

**StreamX** - Professional Multi-Platform Streaming for C++ Developers
