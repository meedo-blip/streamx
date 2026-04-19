#include "streamx/core/frame.h"
#include "streamx/utils/logger.h"
#include <cstring>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>
}

namespace streamx {

// Helper to convert PixelFormat to AVPixelFormat
static AVPixelFormat ToAVPixelFormat(PixelFormat fmt) {
    switch (fmt) {
        case PixelFormat::YUV420P: return AV_PIX_FMT_YUV420P;
        case PixelFormat::NV12: return AV_PIX_FMT_NV12;
        case PixelFormat::RGB24: return AV_PIX_FMT_RGB24;
        case PixelFormat::RGBA32: return AV_PIX_FMT_RGBA;
        default: return AV_PIX_FMT_YUV420P;
    }
}

// Helper to convert SampleFormat to AVSampleFormat
static AVSampleFormat ToAVSampleFormat(SampleFormat fmt) {
    switch (fmt) {
        case SampleFormat::S16: return AV_SAMPLE_FMT_S16;
        case SampleFormat::S32: return AV_SAMPLE_FMT_S32;
        case SampleFormat::FLT: return AV_SAMPLE_FMT_FLT;
        case SampleFormat::FLTP: return AV_SAMPLE_FMT_FLTP;
        default: return AV_SAMPLE_FMT_S16;
    }
}

// Frame constructor for video
Frame::Frame(uint32_t width, uint32_t height, PixelFormat format)
    : width_(width), height_(height), pixel_format_(format), is_video_(true) {
    frame_ = av_frame_alloc();
    AllocateVideoFrame();
}

// Frame constructor for audio
Frame::Frame(uint32_t sample_rate, uint32_t channels, SampleFormat format, uint32_t samples)
    : sample_rate_(sample_rate), channels_(channels), 
      nb_samples_(samples), sample_format_(format), is_video_(false) {
    frame_ = av_frame_alloc();
    AllocateAudioFrame();
}

Frame::~Frame() {
    if (frame_) {
        av_frame_free(&frame_);
    }
}

Frame::Frame(Frame&& other) noexcept 
    : frame_(other.frame_), data_(other.data_), linesize_(other.linesize_),
      width_(other.width_), height_(other.height_), pixel_format_(other.pixel_format_),
      is_video_(other.is_video_), sample_rate_(other.sample_rate_),
      channels_(other.channels_), nb_samples_(other.nb_samples_),
      sample_format_(other.sample_format_), pts_(other.pts_), dts_(other.dts_) {
    other.frame_ = nullptr;
    other.data_ = nullptr;
    other.linesize_ = nullptr;
}

Frame& Frame::operator=(Frame&& other) noexcept {
    if (this != &other) {
        if (frame_) {
            av_frame_free(&frame_);
        }
        frame_ = other.frame_;
        data_ = other.data_;
        linesize_ = other.linesize_;
        width_ = other.width_;
        height_ = other.height_;
        pixel_format_ = other.pixel_format_;
        is_video_ = other.is_video_;
        sample_rate_ = other.sample_rate_;
        channels_ = other.channels_;
        nb_samples_ = other.nb_samples_;
        sample_format_ = other.sample_format_;
        pts_ = other.pts_;
        dts_ = other.dts_;

        other.frame_ = nullptr;
        other.data_ = nullptr;
        other.linesize_ = nullptr;
    }
    return *this;
}

void Frame::AllocateVideoFrame() {
    if (!frame_) return;

    frame_->width = width_;
    frame_->height = height_;
    frame_->format = ToAVPixelFormat(pixel_format_);

    if (av_frame_get_buffer(frame_, 0) < 0) {
        STREAMX_ERROR("Failed to allocate video frame buffer");
        return;
    }

    data_ = frame_->data;
    linesize_ = frame_->linesize;
}

void Frame::AllocateAudioFrame() {
    if (!frame_) return;

    frame_->sample_rate = sample_rate_;
    frame_->nb_samples = nb_samples_;
    frame_->format = ToAVSampleFormat(sample_format_);
    
    // Set channel layout using the modern FFmpeg API
    av_channel_layout_default(&frame_->ch_layout, channels_);

    if (av_frame_get_buffer(frame_, 0) < 0) {
        STREAMX_ERROR("Failed to allocate audio frame buffer");
        return;
    }

    data_ = frame_->data;
    linesize_ = frame_->linesize;
}

// Packet implementation
Packet::Packet(uint32_t capacity)
    : capacity_(capacity), packet_(av_packet_alloc()) {
    data_ = new uint8_t[capacity];
}

Packet::~Packet() {
    delete[] data_;
    if (packet_) {
        av_packet_free(&packet_);
    }
}

Packet::Packet(Packet&& other) noexcept
    : data_(other.data_), size_(other.size_), capacity_(other.capacity_),
      is_keyframe_(other.is_keyframe_), pts_(other.pts_), dts_(other.dts_),
      packet_(other.packet_) {
    other.data_ = nullptr;
    other.packet_ = nullptr;
}

Packet& Packet::operator=(Packet&& other) noexcept {
    if (this != &other) {
        delete[] data_;
        if (packet_) {
            av_packet_free(&packet_);
        }

        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        is_keyframe_ = other.is_keyframe_;
        pts_ = other.pts_;
        dts_ = other.dts_;
        packet_ = other.packet_;

        other.data_ = nullptr;
        other.packet_ = nullptr;
    }
    return *this;
}

void Packet::SetKeyframe(bool key) {
    is_keyframe_ = key;
    if (packet_) {
        if (key) {
            packet_->flags |= AV_PKT_FLAG_KEY;
        } else {
            packet_->flags &= ~AV_PKT_FLAG_KEY;
        }
    }
}

void Packet::SetPTS(int64_t pts) {
    pts_ = pts;
    if (packet_) {
        packet_->pts = pts;
    }
}

void Packet::SetDTS(int64_t dts) {
    dts_ = dts;
    if (packet_) {
        packet_->dts = dts;
    }
}

void Packet::SetData(const uint8_t* data, uint32_t size) {
    if (size > capacity_) {
        uint8_t* new_data = new uint8_t[size];
        delete[] data_;
        data_ = new_data;
        capacity_ = size;
    }

    if (data && size > 0) {
        std::memcpy(data_, data, size);
    }
    size_ = size;

    if (packet_) {
        av_packet_unref(packet_);
        packet_->data = data_;
        packet_->size = size_;
        packet_->pts = pts_;
        packet_->dts = dts_;
        if (is_keyframe_) {
            packet_->flags |= AV_PKT_FLAG_KEY;
        }
    }
}

}  // namespace streamx
