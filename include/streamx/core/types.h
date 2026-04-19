#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace streamx {

// Codec types
enum class CodecType {
    H264,
    H265,
    VP8,
    VP9
};

// Audio codec types
enum class AudioCodecType {
    AAC,
    OPUS,
    MP3
};

// Pixel formats
enum class PixelFormat {
    YUV420P,    // Standard for H.264/H.265
    NV12,       // NVIDIA common format
    RGB24,
    RGBA32
};

// Sample formats
enum class SampleFormat {
    S16,        // 16-bit signed
    S32,        // 32-bit signed
    FLT,        // 32-bit float
    FLTP        // 32-bit float planar
};

// Video frame configuration
struct VideoConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t fps = 60;
    uint32_t bitrate_kbps = 5000;
    CodecType codec = CodecType::H264;
    PixelFormat pixel_format = PixelFormat::YUV420P;
    bool enable_bframes = true;
    uint32_t gop_size = 60;  // Keyframe interval
    std::string preset = "medium";  // ultrafast, superfast, veryfast, faster, fast, medium, slow, slower
    std::string rtmp_url;  // For external streaming (e.g., GPU Screen Recorder)
};

// Audio configuration
struct AudioConfig {
    uint32_t sample_rate = 48000;
    uint32_t channels = 2;
    uint32_t bitrate_kbps = 128;
    AudioCodecType codec = AudioCodecType::AAC;
    SampleFormat sample_format = SampleFormat::S16;
};

// Stream configuration combining audio and video
struct StreamConfig {
    VideoConfig video;
    AudioConfig audio;
    std::string stream_key;
    std::string server_url;
    uint32_t max_buffer_frames = 120;  // ~2 seconds at 60fps
};

// Credentials for platform authentication
struct Credentials {
    std::string username;
    std::string password;
    std::string token;
    std::string client_id;
    std::string client_secret;
    std::string channel_id;
};

// Stream health metrics
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

enum class StreamStatus {
    Idle,
    Connecting,
    Connected,
    Reconnecting,
    Failed,
    Stopped
};

}  // namespace streamx
