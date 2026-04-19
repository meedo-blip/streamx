#pragma once

#include <memory>
#include <string>
#include "types.h"
#include "frame.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace streamx {

// Base encoder interface
class IEncoder {
public:
    virtual ~IEncoder() = default;

    virtual bool Initialize(const VideoConfig& config) = 0;
    virtual bool Encode(const FramePtr& frame, PacketPtr& output_packet) = 0;
    virtual bool Flush(PacketPtr& output_packet) = 0;
    virtual void Close() = 0;
    virtual const AVCodecContext* GetCodecContext() const = 0;

    virtual double GetEncodingLatencyMs() const = 0;
    virtual double GetCPUUsage() const = 0;
};

// Software encoder using libx264/libx265
class SoftwareEncoder : public IEncoder {
public:
    SoftwareEncoder(CodecType codec = CodecType::H264);
    ~SoftwareEncoder() override;

    bool Initialize(const VideoConfig& config) override;
    bool Encode(const FramePtr& frame, PacketPtr& output_packet) override;
    bool Flush(PacketPtr& output_packet) override;
    void Close() override;
    const AVCodecContext* GetCodecContext() const override { return codec_ctx_; }

    double GetEncodingLatencyMs() const override;
    double GetCPUUsage() const override;

private:
    AVCodecContext* codec_ctx_ = nullptr;
    AVFrame* av_frame_ = nullptr;
    SwsContext* sws_ctx_ = nullptr;
    CodecType codec_type_;
    VideoConfig config_;

    int64_t frame_count_ = 0;
    double encoding_time_ms_ = 0.0;
};

// GPU encoder using NVIDIA NVENC
class NVENCEncoder : public IEncoder {
public:
    NVENCEncoder();
    ~NVENCEncoder() override;

    bool Initialize(const VideoConfig& config) override;
    bool Encode(const FramePtr& frame, PacketPtr& output_packet) override;
    bool Flush(PacketPtr& output_packet) override;
    void Close() override;
    const AVCodecContext* GetCodecContext() const override { return nullptr; }

    double GetEncodingLatencyMs() const override;
    double GetCPUUsage() const override;

private:
    VideoConfig config_;
    bool is_available_;

    // Placeholder for NVIDIA NVENC implementation
    int64_t frame_count_ = 0;
};

// Factory for creating encoders
class EncoderFactory {
public:
    static std::unique_ptr<IEncoder> CreateEncoder(CodecType codec, bool prefer_gpu = false);
    static bool IsGPUEncoderAvailable(CodecType codec);
};

}  // namespace streamx
