#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <chrono>
#include "types.h"

extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/packet.h>
}

namespace streamx {

// AVFrame wrapper for ownership and management
class Frame {
public:
    Frame(uint32_t width, uint32_t height, PixelFormat format);
    Frame(uint32_t sample_rate, uint32_t channels, SampleFormat format, uint32_t samples);
    ~Frame();

    // Deleted copy, allowed move
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    Frame(Frame&&) noexcept;
    Frame& operator=(Frame&&) noexcept;

    // Video frame getters
    uint8_t** GetData() const { return data_; }
    int* GetLinesize() const { return linesize_; }
    uint32_t GetWidth() const { return width_; }
    uint32_t GetHeight() const { return height_; }
    PixelFormat GetPixelFormat() const { return pixel_format_; }
    bool IsVideo() const { return is_video_; }

    // Audio frame getters
    uint32_t GetSampleRate() const { return sample_rate_; }
    uint32_t GetChannels() const { return channels_; }
    uint32_t GetSamples() const { return nb_samples_; }
    SampleFormat GetSampleFormat() const { return sample_format_; }
    bool IsAudio() const { return !is_video_; }

    // Timestamps
    void SetPTS(int64_t pts) { pts_ = pts; }
    int64_t GetPTS() const { return pts_; }
    void SetDTS(int64_t dts) { dts_ = dts; }
    int64_t GetDTS() const { return dts_; }

    // Accessors for raw FFmpeg frame (internal use)
    AVFrame* GetAVFrame() const { return frame_; }

private:
    AVFrame* frame_ = nullptr;
    uint8_t** data_ = nullptr;
    int* linesize_ = nullptr;

    // Video properties
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    PixelFormat pixel_format_;
    bool is_video_ = true;

    // Audio properties
    uint32_t sample_rate_ = 0;
    uint32_t channels_ = 0;
    uint32_t nb_samples_ = 0;
    SampleFormat sample_format_;

    // Timestamps
    int64_t pts_ = 0;
    int64_t dts_ = 0;

    void AllocateVideoFrame();
    void AllocateAudioFrame();
};

// Packet wrapper for encoded data
class Packet {
public:
    explicit Packet(uint32_t capacity = 65536);
    ~Packet();

    Packet(const Packet&) = delete;
    Packet& operator=(const Packet&) = delete;
    Packet(Packet&&) noexcept;
    Packet& operator=(Packet&&) noexcept;

    uint8_t* GetData() { return data_; }
    const uint8_t* GetData() const { return data_; }
    uint32_t GetSize() const { return size_; }
    uint32_t GetCapacity() const { return capacity_; }

    bool IsKeyframe() const { return is_keyframe_; }
    void SetKeyframe(bool key);

    void SetPTS(int64_t pts);
    int64_t GetPTS() const { return pts_; }
    void SetDTS(int64_t dts);
    int64_t GetDTS() const { return dts_; }

    void SetData(const uint8_t* data, uint32_t size);
    void Clear() { size_ = 0; }

    AVPacket* GetAVPacket() const { return packet_; }

private:
    uint8_t* data_ = nullptr;
    uint32_t size_ = 0;
    uint32_t capacity_ = 0;
    bool is_keyframe_ = false;
    int64_t pts_ = 0;
    int64_t dts_ = 0;
    AVPacket* packet_ = nullptr;
};

using FramePtr = std::shared_ptr<Frame>;
using PacketPtr = std::shared_ptr<Packet>;

}  // namespace streamx
