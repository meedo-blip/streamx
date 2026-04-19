#include "streamx/core/encoder.h"
#include "streamx/utils/logger.h"
#include <chrono>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace streamx {

static const char* CodecTypeToString(CodecType codec) {
    switch (codec) {
        case CodecType::H264: return "h264";
        case CodecType::H265: return "hevc";
        case CodecType::VP8: return "vp8";
        case CodecType::VP9: return "vp9";
        default: return "h264";
    }
}

static const AVCodecID CodecTypeToAVCodecID(CodecType codec) {
    switch (codec) {
        case CodecType::H264: return AV_CODEC_ID_H264;
        case CodecType::H265: return AV_CODEC_ID_HEVC;
        case CodecType::VP8: return AV_CODEC_ID_VP8;
        case CodecType::VP9: return AV_CODEC_ID_VP9;
        default: return AV_CODEC_ID_H264;
    }
}

static AVPixelFormat CodecPixelFormat(CodecType codec) {
    switch (codec) {
        case CodecType::H265: return AV_PIX_FMT_YUV420P;
        default: return AV_PIX_FMT_YUV420P;
    }
}

SoftwareEncoder::SoftwareEncoder(CodecType codec)
    : codec_type_(codec) {
    STREAMX_INFO("SoftwareEncoder initialized: " + std::string(CodecTypeToString(codec)));
}

SoftwareEncoder::~SoftwareEncoder() {
    Close();
}

bool SoftwareEncoder::Initialize(const VideoConfig& config) {
    config_ = config;

    AVCodecID codec_id = CodecTypeToAVCodecID(config_.codec);
    const AVCodec* codec = avcodec_find_encoder(codec_id);

    if (!codec) {
        STREAMX_ERROR("Encoder not found for codec");
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        STREAMX_ERROR("Failed to allocate codec context");
        return false;
    }

    // Configure codec
    codec_ctx_->width = config_.width;
    codec_ctx_->height = config_.height;
    codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx_->time_base = {1, (int)config_.fps};
    codec_ctx_->framerate = {(int)config_.fps, 1};
    codec_ctx_->bit_rate = config_.bitrate_kbps * 1000;
    codec_ctx_->gop_size = config_.gop_size;
    codec_ctx_->max_b_frames = 0;

    // Set preset
    av_opt_set(codec_ctx_->priv_data, "preset", config_.preset.c_str(), 0);
    av_opt_set(codec_ctx_->priv_data, "tune", "zerolatency", 0);
    av_opt_set(codec_ctx_->priv_data, "rc-lookahead", "0", 0);
    av_opt_set(codec_ctx_->priv_data, "repeat-headers", "1", 0);

    // Open codec
    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        STREAMX_ERROR("Failed to open codec");
        return false;
    }

    // Allocate frame
    av_frame_ = av_frame_alloc();
    if (!av_frame_) {
        STREAMX_ERROR("Failed to allocate AVFrame");
        return false;
    }

    av_frame_->format = codec_ctx_->pix_fmt;
    av_frame_->width = codec_ctx_->width;
    av_frame_->height = codec_ctx_->height;

    if (av_frame_get_buffer(av_frame_, 0) < 0) {
        STREAMX_ERROR("Failed to allocate frame buffer");
        return false;
    }

    STREAMX_INFO("Software encoder initialized successfully");
    return true;
}

bool SoftwareEncoder::Encode(const FramePtr& frame, PacketPtr& output_packet) {
    if (!codec_ctx_ || !frame) {
        return false;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Get input frame data
    AVFrame* input_frame = frame->GetAVFrame();
    if (!input_frame) {
        return false;
    }

    input_frame->pts = frame_count_++;

    // Encode frame
    if (avcodec_send_frame(codec_ctx_, input_frame) < 0) {
        STREAMX_ERROR("Error sending frame to encoder");
        return false;
    }

    // Receive encoded packet
    if (!output_packet) {
        output_packet = std::make_shared<Packet>(65536);
    }

    AVPacket* pkt = av_packet_alloc();
    const int receive_result = avcodec_receive_packet(codec_ctx_, pkt);
    if (receive_result == 0) {
        output_packet->SetData(pkt->data, pkt->size);
        output_packet->SetKeyframe(pkt->flags & AV_PKT_FLAG_KEY);
        output_packet->SetPTS(pkt->pts);
        output_packet->SetDTS(pkt->dts);
        av_packet_unref(pkt);
        av_packet_free(&pkt);

        auto end_time = std::chrono::high_resolution_clock::now();
        encoding_time_ms_ = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        return true;
    }

    if (receive_result != AVERROR(EAGAIN) && receive_result != AVERROR_EOF) {
        STREAMX_ERROR("Error receiving packet from encoder");
    }
    av_packet_free(&pkt);

    auto end_time = std::chrono::high_resolution_clock::now();
    encoding_time_ms_ = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    output_packet.reset();
    return false;
}

bool SoftwareEncoder::Flush(PacketPtr& output_packet) {
    if (!codec_ctx_) return false;

    const int send_result = avcodec_send_frame(codec_ctx_, nullptr);
    if (send_result < 0 && send_result != AVERROR_EOF) {
        STREAMX_ERROR("Error flushing encoder");
        return false;
    }

    AVPacket* pkt = av_packet_alloc();
    const int receive_result = avcodec_receive_packet(codec_ctx_, pkt);
    if (receive_result == 0) {
        if (!output_packet) {
            output_packet = std::make_shared<Packet>(65536);
        }
        output_packet->SetData(pkt->data, pkt->size);
        output_packet->SetKeyframe(pkt->flags & AV_PKT_FLAG_KEY);
        output_packet->SetPTS(pkt->pts);
        output_packet->SetDTS(pkt->dts);
        av_packet_unref(pkt);
        av_packet_free(&pkt);
        return true;
    }

    if (receive_result != AVERROR(EAGAIN) && receive_result != AVERROR_EOF) {
        STREAMX_ERROR("Error receiving flushed packet from encoder");
    }
    av_packet_free(&pkt);

    output_packet.reset();
    return false;
}

void SoftwareEncoder::Close() {
    if (codec_ctx_) {
        avcodec_close(codec_ctx_);
        avcodec_free_context(&codec_ctx_);
    }
    if (av_frame_) {
        av_frame_free(&av_frame_);
    }
}

double SoftwareEncoder::GetEncodingLatencyMs() const {
    return encoding_time_ms_;
}

double SoftwareEncoder::GetCPUUsage() const {
    // Placeholder - would require process monitoring
    return 0.0;
}

// NVENC Encoder (placeholder)
NVENCEncoder::NVENCEncoder() : is_available_(false) {
    #ifdef HAVE_NVENC
    is_available_ = true;
    #endif
}

NVENCEncoder::~NVENCEncoder() {
    Close();
}

bool NVENCEncoder::Initialize(const VideoConfig& config) {
    if (!is_available_) {
        STREAMX_WARN("NVIDIA NVENC not available");
        return false;
    }
    config_ = config;
    STREAMX_INFO("NVENC encoder initialized");
    return true;
}

bool NVENCEncoder::Encode(const FramePtr& frame, PacketPtr& output_packet) {
    // Placeholder implementation
    return true;
}

bool NVENCEncoder::Flush(PacketPtr& output_packet) {
    return true;
}

void NVENCEncoder::Close() {
}

double NVENCEncoder::GetEncodingLatencyMs() const {
    return 0.0;
}

double NVENCEncoder::GetCPUUsage() const {
    return 0.0;
}

std::unique_ptr<IEncoder> EncoderFactory::CreateEncoder(CodecType codec, bool prefer_gpu) {
    if (prefer_gpu) {
        auto gpu_encoder = std::make_unique<NVENCEncoder>();
        if (gpu_encoder->Initialize(VideoConfig{.codec = codec})) {
            return gpu_encoder;
        }
        STREAMX_WARN("GPU encoder not available, falling back to software encoder");
    }

    return std::make_unique<SoftwareEncoder>(codec);
}

bool EncoderFactory::IsGPUEncoderAvailable(CodecType codec) {
    #ifdef HAVE_NVENC
    return true;
    #else
    return false;
    #endif
}

}  // namespace streamx
