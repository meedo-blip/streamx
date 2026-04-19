#include "streamx/platform/rtmp_handler.h"
#include "streamx/utils/logger.h"
#include <thread>
#include <chrono>
#include <cstring>

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/error.h>
}

namespace streamx {

RTMPHandler::RTMPHandler() {
    avformat_network_init();
    STREAMX_INFO("RTMPHandler created");
}

RTMPHandler::~RTMPHandler() {
    Disconnect();
}

bool RTMPHandler::Connect(const std::string& url, const AVCodecContext* video_codec_context,
                          uint32_t timeout_ms) {
    server_url_ = url;
    video_codec_context_ = video_codec_context;
    STREAMX_INFO("Attempting to connect to RTMP server: " + url);

    last_error_.clear();

    if (!video_codec_context) {
        last_error_ = "Missing video codec context for RTMP publishing";
        STREAMX_ERROR("RTMP connect failed: " + last_error_);
        return false;
    }

    AVFormatContext* output_context = nullptr;
    if (avformat_alloc_output_context2(&output_context, nullptr, "flv", url.c_str()) < 0 || !output_context) {
        last_error_ = "Failed to allocate FLV output context";
        STREAMX_ERROR("RTMP connect failed: " + last_error_);
        return false;
    }

    AVStream* video_stream = avformat_new_stream(output_context, nullptr);
    if (!video_stream) {
        last_error_ = "Failed to create video stream";
        avformat_free_context(output_context);
        STREAMX_ERROR("RTMP connect failed: " + last_error_);
        return false;
    }

    if (avcodec_parameters_from_context(video_stream->codecpar,
                                        const_cast<AVCodecContext*>(video_codec_context)) < 0) {
        last_error_ = "Failed to copy codec parameters";
        avformat_free_context(output_context);
        STREAMX_ERROR("RTMP connect failed: " + last_error_);
        return false;
    }

    video_stream->codecpar->codec_tag = 0;
    video_stream->time_base = video_codec_context->time_base;
    input_time_base_ = video_codec_context->time_base;

    AVDictionary* options = nullptr;

    if (!(output_context->oformat->flags & AVFMT_NOFILE)) {
        const int open_result = avio_open2(&output_context->pb, url.c_str(), AVIO_FLAG_WRITE, nullptr, &options);
        if (open_result < 0) {
            char error_buffer[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(open_result, error_buffer, sizeof(error_buffer));
            last_error_ = "Failed to open RTMP output: " + std::string(error_buffer);
            av_dict_free(&options);
            avformat_free_context(output_context);
            STREAMX_ERROR("RTMP connect failed: " + last_error_);
            return false;
        }
    }

    const int header_result = avformat_write_header(output_context, &options);
    if (header_result < 0) {
        char error_buffer[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(header_result, error_buffer, sizeof(error_buffer));
        last_error_ = "Failed to write RTMP stream header: " + std::string(error_buffer);
        av_dict_free(&options);
        if (!(output_context->oformat->flags & AVFMT_NOFILE) && output_context->pb) {
            avio_closep(&output_context->pb);
        }
        avformat_free_context(output_context);
        STREAMX_ERROR("RTMP connect failed: " + last_error_);
        return false;
    }

    av_dict_free(&options);

    output_context_ = output_context;
    video_stream_ = video_stream;
    is_connected_ = true;
    STREAMX_INFO("Connected to RTMP server");
    return true;
}

bool RTMPHandler::Disconnect() {
    if (!is_connected_) {
        return true;
    }

    STREAMX_INFO("Disconnecting from RTMP server");
    if (output_context_) {
        av_write_trailer(output_context_);
        if (!(output_context_->oformat->flags & AVFMT_NOFILE) && output_context_->pb) {
            avio_closep(&output_context_->pb);
        }
        avformat_free_context(output_context_);
        output_context_ = nullptr;
        video_stream_ = nullptr;
    }
    is_connected_ = false;
    bytes_sent_ = 0;
    bytes_received_ = 0;
    packets_sent_ = 0;

    return true;
}

bool RTMPHandler::SendPacket(const PacketPtr& packet) {
    if (!is_connected_) {
        if (last_error_.empty()) {
            last_error_ = "Not connected to RTMP server";
        }
        return false;
    }

    if (!packet || !video_stream_ || !output_context_) {
        last_error_ = "RTMP output is not initialized";
        return false;
    }

    AVPacket* source_packet = packet->GetAVPacket();
    if (!source_packet || !source_packet->data || source_packet->size == 0) {
        last_error_ = "Encoded packet is empty";
        return false;
    }

    AVPacket* mux_packet = av_packet_alloc();
    if (!mux_packet) {
        last_error_ = "Failed to allocate RTMP packet";
        return false;
    }

    if (av_new_packet(mux_packet, source_packet->size) < 0) {
        av_packet_free(&mux_packet);
        last_error_ = "Failed to allocate RTMP packet payload";
        return false;
    }

    std::memcpy(mux_packet->data, source_packet->data, source_packet->size);
    mux_packet->size = source_packet->size;
    mux_packet->pts = source_packet->pts;
    mux_packet->dts = source_packet->dts;
    mux_packet->flags = source_packet->flags;
    mux_packet->stream_index = video_stream_->index;
    av_packet_rescale_ts(mux_packet, input_time_base_, video_stream_->time_base);

    const int write_result = av_interleaved_write_frame(output_context_, mux_packet);
    av_packet_free(&mux_packet);
    if (write_result < 0) {
        char error_buffer[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(write_result, error_buffer, sizeof(error_buffer));
        last_error_ = "Failed to write packet to RTMP output: " + std::string(error_buffer);
        STREAMX_ERROR(last_error_);
        return false;
    }

    bytes_sent_ += packet->GetSize();
    packets_sent_++;

    return true;
}

bool RTMPHandler::SendMetadata(const std::string& metadata) {
    if (!is_connected_) {
        return false;
    }

    STREAMX_DEBUG("Sending metadata: " + metadata);
    return true;
}

bool RTMPHandler::ReconnectWithBackoff(uint32_t max_retries) {
    uint32_t retry_count = 0;
    uint32_t backoff_ms = 1000;

    while (retry_count < max_retries) {
        STREAMX_INFO("Reconnection attempt " + std::to_string(retry_count + 1) + "/" + std::to_string(max_retries));

        if (Connect(server_url_, video_codec_context_)) {
            STREAMX_INFO("Reconnection successful");
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
        backoff_ms = std::min(backoff_ms * 2, 30000u);  // Max 30 seconds
        retry_count++;
    }

    STREAMX_ERROR("Failed to reconnect after " + std::to_string(max_retries) + " attempts");
    return false;
}

}  // namespace streamx
