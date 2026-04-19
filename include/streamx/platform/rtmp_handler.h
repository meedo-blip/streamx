#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "../core/types.h"
#include "../core/frame.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace streamx {

// RTMP protocol handler
class RTMPHandler {
public:
    RTMPHandler();
    ~RTMPHandler();

    // Connection management
    bool Connect(const std::string& url, const AVCodecContext* video_codec_context,
                 uint32_t timeout_ms = 5000);
    bool Disconnect();
    bool IsConnected() const { return is_connected_; }

    // Streaming operations
    bool SendPacket(const PacketPtr& packet);
    bool SendMetadata(const std::string& metadata);

    // Configuration
    void SetBufferSize(uint32_t size) { buffer_size_ = size; }
    void SetSendTimeout(uint32_t ms) { send_timeout_ms_ = ms; }
    void SetRecvTimeout(uint32_t ms) { recv_timeout_ms_ = ms; }

    // Health & diagnostics
    uint64_t GetBytesSent() const { return bytes_sent_; }
    uint64_t GetBytesReceived() const { return bytes_received_; }
    uint32_t GetPacketsSent() const { return packets_sent_; }
    double GetLatencyMs() const { return latency_ms_; }

    // Error handling
    std::string GetLastError() const { return last_error_; }
    bool ReconnectWithBackoff(uint32_t max_retries = 5);

private:
    bool is_connected_ = false;
    std::string server_url_;
    AVFormatContext* output_context_ = nullptr;
    AVStream* video_stream_ = nullptr;
    const AVCodecContext* video_codec_context_ = nullptr;
    AVRational input_time_base_{1, 1};

    uint32_t buffer_size_ = 65536;
    uint32_t send_timeout_ms_ = 5000;
    uint32_t recv_timeout_ms_ = 5000;

    uint64_t bytes_sent_ = 0;
    uint64_t bytes_received_ = 0;
    uint32_t packets_sent_ = 0;
    double latency_ms_ = 0.0;
    std::string last_error_;
};

}  // namespace streamx
