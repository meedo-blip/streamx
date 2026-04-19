#include "streamx/core/capture.h"
#include "streamx/core/types.h"
#include "streamx/core/frame.h"
#include "streamx/utils/logger.h"
#include <thread>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

extern "C" {
#ifdef signals
#pragma push_macro("signals")
#undef signals
#define STREAMX_RESTORE_QT_SIGNALS_MACRO
#endif
#include <libportal/portal.h>
#ifdef STREAMX_RESTORE_QT_SIGNALS_MACRO
#pragma pop_macro("signals")
#undef STREAMX_RESTORE_QT_SIGNALS_MACRO
#endif
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <pipewire/pipewire.h>
#include <spa/param/video/raw-utils.h>
#include <spa/utils/hook.h>
#include <gio/gio.h>
}

#ifdef STREAMX_HAVE_QT_GUI
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QImage>
#endif

#include <pipewire/keys.h>

#ifdef ENABLE_PLAN_A
#include "streamx/portal/dbus_screencast.h"
#endif

#ifdef ENABLE_PLAN_D
#include "streamx/plan_d.h"
#endif

namespace streamx {

namespace {

AVPixelFormat ToAVPixelFormat(PixelFormat fmt) {
    switch (fmt) {
        case PixelFormat::YUV420P: return AV_PIX_FMT_YUV420P;
        case PixelFormat::NV12: return AV_PIX_FMT_NV12;
        case PixelFormat::RGB24: return AV_PIX_FMT_RGB24;
        case PixelFormat::RGBA32: return AV_PIX_FMT_RGBA;
        default: return AV_PIX_FMT_YUV420P;
    }
}

AVPixelFormat FromSpaVideoFormat(uint32_t format) {
    switch (format) {
        case SPA_VIDEO_FORMAT_BGRx: return AV_PIX_FMT_BGR0;
        case SPA_VIDEO_FORMAT_BGRA: return AV_PIX_FMT_BGRA;
        case SPA_VIDEO_FORMAT_RGBx: return AV_PIX_FMT_RGB0;
        case SPA_VIDEO_FORMAT_RGBA: return AV_PIX_FMT_RGBA;
        case SPA_VIDEO_FORMAT_YV12: return AV_PIX_FMT_YUV420P;
        default: return AV_PIX_FMT_BGR0;
    }
}

void FillTestPattern(const FramePtr& frame, int64_t frame_index) {
    if (!frame || !frame->IsVideo()) return;
    uint8_t** data = frame->GetData();
    int* linesize = frame->GetLinesize();
    if (!data || !linesize) return;
    const int width = static_cast<int>(frame->GetWidth());
    const int height = static_cast<int>(frame->GetHeight());
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            data[0][y * linesize[0] + x] = static_cast<uint8_t>((x + frame_index * 2) % 256);
        }
    }
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width / 2; x++) {
            data[1][y * linesize[1] + x] = static_cast<uint8_t>(128);
            data[2][y * linesize[2] + x] = static_cast<uint8_t>(128);
        }
    }
}

std::string DefaultDisplayInput() {
    const char* display = std::getenv("DISPLAY");
    if (!display || std::string(display).empty()) return ":0.0";
    std::string value(display);
    if (value.find('.') == std::string::npos) value += ".0";
    return value;
}

bool IsWaylandSession() {
    const char* session_type = std::getenv("XDG_SESSION_TYPE");
    if (session_type && std::string(session_type) == "wayland") return true;
    if (std::getenv("WAYLAND_DISPLAY")) return true;
    return false;
}

} // anonymous namespace

static int pw_fd = -1;

struct PortalAsyncState {
    DesktopCapture* capture = nullptr;
};

void QuitPortalLoop(DesktopCapture* capture) {
    if (capture && capture->portal_loop_) {
        g_main_loop_quit(capture->portal_loop_);
    }
}

void OnPortalSessionStarted(GObject* source_object, GAsyncResult* result, gpointer user_data) {
    auto* state = static_cast<PortalAsyncState*>(user_data);
    auto* capture = state->capture;
    g_autoptr(GError) error = nullptr;

    if (!xdp_session_start_finish(capture->screencast_session_, result, &error)) {
        capture->portal_failed_ = true;
        if (error) STREAMX_WARN("Wayland screencast start failed: " + std::string(error->message));
        QuitPortalLoop(capture);
        return;
    }

    GVariant* streams = xdp_session_get_streams(capture->screencast_session_);
    if (!streams) {
        capture->portal_failed_ = true;
        STREAMX_WARN("Wayland screencast returned no streams");
        QuitPortalLoop(capture);
        return;
    }

    GVariantIter iter;
    g_variant_iter_init(&iter, streams);
    guint32 node_id = 0;
    GVariant* props = nullptr;
    if (!g_variant_iter_next(&iter, "(u@a{sv})", &node_id, &props)) {
        capture->portal_failed_ = true;
        STREAMX_WARN("Failed to parse Wayland screencast stream metadata");
        QuitPortalLoop(capture);
        return;
    }
    if (props) g_variant_unref(props);

    capture->pipewire_node_id_ = node_id;
    STREAMX_INFO("Portal session started, node_id=" + std::to_string(node_id));
    pw_fd = xdp_session_open_pipewire_remote(capture->screencast_session_);
    STREAMX_INFO("Got PipeWire FD: " + std::to_string(pw_fd));
    capture->portal_ready_ = true;
    QuitPortalLoop(capture);
}

void OnPortalSessionCreated(GObject* source_object, GAsyncResult* result, gpointer user_data) {
    auto* state = static_cast<PortalAsyncState*>(user_data);
    auto* capture = state->capture;
    g_autoptr(GError) error = nullptr;

    capture->screencast_session_ = xdp_portal_create_screencast_session_finish(XDP_PORTAL(source_object), result, &error);
    if (!capture->screencast_session_) {
        capture->portal_failed_ = true;
        if (error) STREAMX_WARN("Wayland screencast session creation failed: " + std::string(error->message));
        QuitPortalLoop(capture);
        return;
    }
    xdp_session_start(capture->screencast_session_, nullptr, capture->portal_cancellable_, OnPortalSessionStarted, user_data);
}

void OnPipeWireStreamStateChanged(void* data, enum pw_stream_state old_state, enum pw_stream_state state, const char* error) {
    auto* capture = static_cast<DesktopCapture*>(data);
    capture->pw_stream_state_ = static_cast<int>(state);
    if (state == PW_STREAM_STATE_ERROR) {
        capture->portal_failed_ = true;
        STREAMX_WARN(std::string("PipeWire stream error: ") + (error ? error : "unknown"));
    }
}

void OnPipeWireParamChanged(void* data, uint32_t id, const struct spa_pod* param) {
    auto* capture = static_cast<DesktopCapture*>(data);
    if (id != SPA_PARAM_Format || !param) return;
    spa_video_info_raw info{};
    if (spa_format_video_raw_parse(param, &info) == 0) {
        capture->pw_format_ = info.format;
        capture->pw_frame_width_ = static_cast<int>(info.size.width);
        capture->pw_frame_height_ = static_cast<int>(info.size.height);
    }
}

void OnPipeWireProcess(void* data) {
    auto* capture = static_cast<DesktopCapture*>(data);
    pw_buffer* buffer = pw_stream_dequeue_buffer(capture->pw_stream_);
    if (!buffer || !buffer->buffer || buffer->buffer->n_datas == 0) return;

    spa_data* spa_data_ptr = &buffer->buffer->datas[0];
    const int height = capture->pw_frame_height_ > 0 ? capture->pw_frame_height_ : static_cast<int>(capture->video_config_.height);
    const int stride = spa_data_ptr->chunk && spa_data_ptr->chunk->stride > 0 ? spa_data_ptr->chunk->stride : static_cast<int>(capture->video_config_.width) * 4;
    const size_t bytes = spa_data_ptr->chunk && spa_data_ptr->chunk->size > 0 ? spa_data_ptr->chunk->size : static_cast<size_t>(stride) * static_cast<size_t>(height);

    if (bytes == 0 || height == 0 || stride == 0) {
        pw_stream_queue_buffer(capture->pw_stream_, buffer);
        return;
    }

    if (spa_data_ptr->type == SPA_DATA_MemPtr && spa_data_ptr->data && bytes > 0) {
        std::lock_guard<std::mutex> lock(capture->pw_frame_mutex_);
        capture->pw_frame_copy_.resize(bytes);
        std::memcpy(capture->pw_frame_copy_.data(), spa_data_ptr->data, bytes);
        capture->pw_stride_ = stride;
        capture->pw_frame_available_ = true;
        capture->pw_frame_cv_.notify_one();
    }
    pw_stream_queue_buffer(capture->pw_stream_, buffer);
}

DesktopCapture::DesktopCapture(uint32_t display_index) : display_index_(display_index) {
    STREAMX_INFO("DesktopCapture created for display " + std::to_string(display_index));
}

DesktopCapture::~DesktopCapture() {
    Close();
}

bool DesktopCapture::Initialize(const VideoConfig& video_config, const AudioConfig& audio_config) {
    video_config_ = video_config;
    avdevice_register_all();

    const bool is_wayland = IsWaylandSession();

    if (is_wayland) {
        STREAMX_INFO("Wayland session - trying capture methods...");

#ifdef ENABLE_PLAN_D
        {
            STREAMX_INFO("Trying Plan D: GPU Screen Recorder CLI (RTMP streaming)...");
            
            // Get RTMP URL from video_config (set by StreamingController)
            std::string rtmp_url = video_config.rtmp_url;
            
            if (rtmp_url.empty()) {
                STREAMX_WARN("Plan D: No RTMP URL in video_config");
            } else {
                STREAMX_INFO("Plan D: Using RTMP URL from video_config");
            }
            
            int plan_d_pid = -1;
            if (streamx::plan_d::StartPlanD(plan_d_pid, (int)video_config.width, (int)video_config.height, (int)video_config.fps, rtmp_url)) {
                plan_d_active_ = true;
                plan_d_pid_ = plan_d_pid;
                
                if (!rtmp_url.empty()) {
                    STREAMX_INFO("Plan D SUCCESS: GPU Screen Recorder streaming to RTMP");
                    auto frame = std::make_shared<Frame>(video_config.width, video_config.height, video_config.pixel_format);
                    FillTestPattern(frame, 0);
                    STREAMX_INFO("DesktopCapture initialized: Plan D handles RTMP streaming");
                    return true;
                } else {
                    STREAMX_WARN("Plan D: No RTMP URL, starting in background only");
                    plan_d_active_ = false;
                }
            } else {
                STREAMX_WARN("Plan D failed to start");
            }
        }
#endif

        STREAMX_INFO("Trying Plan B: Direct portal/PipeWire...");
        portal_ = xdp_portal_new();
        if (!portal_) {
            STREAMX_WARN("No xdg-desktop-portal available");
        } else {
            portal_loop_ = g_main_loop_new(nullptr, false);
            portal_cancellable_ = g_cancellable_new();

            PortalAsyncState state{this};
            xdp_portal_create_screencast_session(
                portal_, static_cast<XdpOutputType>(XDP_OUTPUT_MONITOR | XDP_OUTPUT_WINDOW),
                XDP_SCREENCAST_FLAG_NONE, XDP_CURSOR_MODE_EMBEDDED, XDP_PERSIST_MODE_NONE,
                nullptr, portal_cancellable_, OnPortalSessionCreated, &state);
            g_main_loop_run(portal_loop_);

            STREAMX_INFO("Portal started - node=" + std::to_string(pipewire_node_id_) + " fd=" + std::to_string(pw_fd));

            if (!portal_failed_ && pw_fd >= 0 && pipewire_node_id_ > 0) {
                STREAMX_INFO("Connecting to PipeWire with node_id=" + std::to_string(pipewire_node_id_));
                pw_init(0, nullptr);
                pw_loop_ = pw_thread_loop_new("streamx", nullptr);
                if (!pw_loop_) {
                    STREAMX_WARN("Failed to create pw_loop");
                } else {
                    pw_context_ = pw_context_new(pw_thread_loop_get_loop(pw_loop_), nullptr, 0);
                    if (!pw_context_) {
                        STREAMX_WARN("Failed to create pw_context");
                    } else {
                        struct pw_properties* props = pw_properties_new(nullptr, nullptr);
                        pw_properties_set(props, "media.category", "Screen");
                        pw_properties_set(props, "media.role", "Screen");
                        pw_core_ = pw_context_connect_fd(pw_context_, pw_fd, props, 0);
                        if (!pw_core_) {
                            STREAMX_WARN("Failed to connect to pw_core");
                        } else {
                            pw_thread_loop_start(pw_loop_);
                            pw_thread_loop_lock(pw_loop_);
                            pw_stream_ = pw_stream_new(pw_core_, "streamx", nullptr);
                            if (!pw_stream_) {
                                STREAMX_WARN("Failed to create PipeWire stream");
                                pw_thread_loop_unlock(pw_loop_);
                            } else {
                                static struct spa_hook listener;
                                static const struct pw_stream_events events = {
                                    PW_VERSION_STREAM_EVENTS,
                                    .state_changed = OnPipeWireStreamStateChanged,
                                    .param_changed = OnPipeWireParamChanged,
                                    .process = OnPipeWireProcess,
                                };
                                pw_stream_add_listener(pw_stream_, &listener, &events, this);
                                pw_thread_loop_unlock(pw_loop_);
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                pw_thread_loop_lock(pw_loop_);
                                int r = pw_stream_connect(pw_stream_, PW_DIRECTION_INPUT, pipewire_node_id_,
                                    (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS), nullptr, 0);
                                if (r < 0) {
                                    STREAMX_WARN("Failed to connect PipeWire stream with node_id: " + std::to_string(r));
                                    pw_stream_destroy(pw_stream_);
                                    pw_stream_ = nullptr;
                                    pw_thread_loop_unlock(pw_loop_);
                                } else {
                                    pw_thread_loop_unlock(pw_loop_);
                                    STREAMX_INFO("Plan B connected - waiting for frames...");
                                    std::this_thread::sleep_for(std::chrono::seconds(3));
                                    if (pw_frame_available_) {
                                        STREAMX_INFO("Plan B SUCCESS: frames arriving");
                                        return true;
                                    } else {
                                        STREAMX_WARN("Plan B FAILED: no frames within 3s timeout - falling back to test pattern");
                                        pw_thread_loop_lock(pw_loop_);
                                        if (pw_stream_) {
                                            pw_stream_disconnect(pw_stream_);
                                            pw_stream_destroy(pw_stream_);
                                            pw_stream_ = nullptr;
                                        }
                                        pw_thread_loop_unlock(pw_loop_);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        STREAMX_WARN("All capture methods failed - using test pattern");
        auto frame = std::make_shared<Frame>(video_config.width, video_config.height, video_config.pixel_format);
        FillTestPattern(frame, 0);
        STREAMX_INFO("DesktopCapture initialized: test pattern");
        return true;
    }

    const AVInputFormat* input_format = av_find_input_format("x11grab");
    if (!input_format) {
        STREAMX_WARN("x11grab input not available; using test pattern fallback");
        return true;
    }

    std::ostringstream video_size;
    video_size << video_config.width << "x" << video_config.height;
    AVDictionary* options = nullptr;
    av_dict_set(&options, "video_size", video_size.str().c_str(), 0);
    av_dict_set(&options, "framerate", std::to_string(video_config.fps).c_str(), 0);
    av_dict_set(&options, "draw_mouse", "1", 0);

    const std::string input_source = DefaultDisplayInput();
    const int open_result = avformat_open_input(&input_context_, input_source.c_str(), input_format, &options);
    av_dict_free(&options);
    if (open_result < 0) {
        STREAMX_WARN("Failed to open x11grab input; using test pattern fallback");
        input_context_ = nullptr;
        return true;
    }

    if (avformat_find_stream_info(input_context_, nullptr) < 0) {
        STREAMX_WARN("Failed to read x11grab stream info; using test pattern fallback");
        Close();
        return true;
    }

    for (unsigned int i = 0; i < input_context_->nb_streams; ++i) {
        if (input_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index_ = static_cast<int>(i);
            break;
        }
    }

    if (video_stream_index_ < 0) {
        STREAMX_WARN("No video stream found in x11grab input; using test pattern fallback");
        Close();
        return true;
    }

    const AVCodecParameters* codec_params = input_context_->streams[video_stream_index_]->codecpar;
    const AVCodec* decoder = avcodec_find_decoder(codec_params->codec_id);
    if (!decoder) {
        STREAMX_WARN("No decoder for x11grab input; using test pattern fallback");
        Close();
        return true;
    }

    decoder_context_ = avcodec_alloc_context3(decoder);
    if (!decoder_context_ || avcodec_parameters_to_context(decoder_context_, codec_params) < 0 ||
        avcodec_open2(decoder_context_, decoder, nullptr) < 0) {
        STREAMX_WARN("Failed to initialize x11grab decoder; using test pattern fallback");
        Close();
        return true;
    }

    input_frame_ = av_frame_alloc();
    input_packet_ = av_packet_alloc();
    if (!input_frame_ || !input_packet_) {
        STREAMX_WARN("Failed to allocate x11grab buffers; using test pattern fallback");
        Close();
        return true;
    }

    source_width_ = decoder_context_->width;
    source_height_ = decoder_context_->height;
    sws_context_ = sws_getContext(source_width_, source_height_, decoder_context_->pix_fmt,
                                  video_config.width, video_config.height,
                                  ToAVPixelFormat(video_config.pixel_format), SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!sws_context_) {
        STREAMX_WARN("Failed to initialize SwsContext; using test pattern fallback");
        Close();
        return true;
    }

    if (input_context_) {
        avformat_close_input(&input_context_);
        input_context_ = nullptr;
    }
    STREAMX_INFO("DesktopCapture initialized from x11grab: " + std::to_string(video_config.width) + "x" + std::to_string(video_config.height));
    return true;
}

bool DesktopCapture::Start() {
    is_running_ = true;
    STREAMX_INFO("DesktopCapture started");
    return true;
}

bool DesktopCapture::Stop() {
    is_running_ = false;
    STREAMX_INFO("DesktopCapture stopped");
    return true;
}

FramePtr DesktopCapture::CaptureFrame(std::chrono::milliseconds timeout) {
    if (!is_running_) return nullptr;
    const auto start_time = std::chrono::steady_clock::now();

    if (pw_stream_) {
        std::unique_lock<std::mutex> lock(pw_frame_mutex_);
        if (pw_frame_cv_.wait_for(lock, timeout, [this] { return pw_frame_available_; })) {
            std::vector<uint8_t> frame_copy = pw_frame_copy_;
            const int stride = pw_stride_;
            const int width = pw_frame_width_ > 0 ? pw_frame_width_ : static_cast<int>(video_config_.width);
            const int height = pw_frame_height_ > 0 ? pw_frame_height_ : static_cast<int>(video_config_.height);
            const uint32_t spa_format = pw_format_ ? pw_format_ : SPA_VIDEO_FORMAT_BGRx;
            pw_frame_available_ = false;
            lock.unlock();

            auto frame = std::make_shared<Frame>(video_config_.width, video_config_.height, video_config_.pixel_format);
            const AVPixelFormat src_format = FromSpaVideoFormat(spa_format);
            if (src_format != AV_PIX_FMT_NONE) {
                const uint8_t* src_slices[4] = { frame_copy.data(), nullptr, nullptr, nullptr };
                const int src_linesizes[4] = { stride, 0, 0, 0 };
                sws_context_ = sws_getCachedContext(sws_context_, width, height, src_format,
                    static_cast<int>(video_config_.width), static_cast<int>(video_config_.height),
                    ToAVPixelFormat(video_config_.pixel_format), SWS_BILINEAR, nullptr, nullptr, nullptr);
                if (sws_context_) {
                    sws_scale(sws_context_, src_slices, src_linesizes, 0, height, frame->GetData(), frame->GetLinesize());
                    frame->SetPTS(frames_captured_);
                    frames_captured_++;
                    capture_latency_ms_ = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start_time).count();
                    return frame;
                }
            }
        }
    }

    if (input_context_ && decoder_context_ && input_frame_ && input_packet_ && sws_context_) {
        while (std::chrono::steady_clock::now() - start_time < timeout) {
            const int read_result = av_read_frame(input_context_, input_packet_);
            if (read_result < 0) break;
            if (input_packet_->stream_index != video_stream_index_) {
                av_packet_unref(input_packet_);
                continue;
            }
            const int send_result = avcodec_send_packet(decoder_context_, input_packet_);
            av_packet_unref(input_packet_);
            if (send_result < 0) continue;
            const int receive_result = avcodec_receive_frame(decoder_context_, input_frame_);
            if (receive_result < 0) continue;
            auto frame = std::make_shared<Frame>(video_config_.width, video_config_.height, video_config_.pixel_format);
            sws_scale(sws_context_, input_frame_->data, input_frame_->linesize, 0, source_height_, frame->GetData(), frame->GetLinesize());
            frame->SetPTS(frames_captured_);
            frames_captured_++;
            capture_latency_ms_ = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start_time).count();
            av_frame_unref(input_frame_);
            return frame;
        }
    }

    auto frame = std::make_shared<Frame>(video_config_.width, video_config_.height, video_config_.pixel_format);
    FillTestPattern(frame, frames_captured_);
    frame->SetPTS(frames_captured_);
    frames_captured_++;
    capture_latency_ms_ = 16.67;
    return frame;
}

void DesktopCapture::Close() {
    is_running_ = false;
    // Only cleanup PipeWire if we own the loop (we started it in this thread)
    // Otherwise just set to nullptr and let the creator clean up
    if (pw_stream_) {
        pw_stream_destroy(pw_stream_);
        pw_stream_ = nullptr;
    }
    if (pw_core_) {
        pw_core_disconnect(pw_core_);
        pw_core_ = nullptr;
    }
    if (pw_context_) {
        pw_context_destroy(pw_context_);
        pw_context_ = nullptr;
    }
    if (pw_loop_) {
        pw_thread_loop_destroy(pw_loop_);
        pw_loop_ = nullptr;
    }
    if (portal_) { g_object_unref(portal_); portal_ = nullptr; }
    if (sws_context_) { sws_freeContext(sws_context_); sws_context_ = nullptr; }
    if (input_frame_) { av_frame_free(&input_frame_); input_frame_ = nullptr; }
    if (input_packet_) { av_packet_free(&input_packet_); input_packet_ = nullptr; }
    if (decoder_context_) { avcodec_free_context(&decoder_context_); decoder_context_ = nullptr; }
    if (input_context_) { avformat_close_input(&input_context_); input_context_ = nullptr; }
#ifdef ENABLE_PLAN_D
    if (plan_d_active_ && plan_d_pid_ > 0) {
        streamx::plan_d::StopPlanD(plan_d_pid_);
        plan_d_active_ = false;
    }
#endif
}

double DesktopCapture::GetCaptureLatencyMs() const {
    return capture_latency_ms_;
}

StreamHealth DesktopCapture::GetHealth() const {
    StreamHealth health;
    health.connected = is_running_;
    return health;
}

const VideoConfig& DesktopCapture::GetVideoConfig() const {
    return video_config_;
}

WindowCapture::WindowCapture(const std::string& window_title) : window_title_(window_title) {}
WindowCapture::~WindowCapture() = default;

bool WindowCapture::Initialize(const VideoConfig& video_config, const AudioConfig& audio_config) {
    video_config_ = video_config;
    STREAMX_INFO("WindowCapture initialized");
    return true;
}

bool WindowCapture::Start() {
    is_running_ = true;
    STREAMX_INFO("WindowCapture started");
    return true;
}

bool WindowCapture::Stop() {
    is_running_ = false;
    return true;
}

FramePtr WindowCapture::CaptureFrame(std::chrono::milliseconds timeout) {
    if (!is_running_) return nullptr;
    auto frame = std::make_shared<Frame>(video_config_.width, video_config_.height, video_config_.pixel_format);
    FillTestPattern(frame, frames_captured_);
    frames_captured_++;
    return frame;
}

void WindowCapture::Close() {
    is_running_ = false;
}

double WindowCapture::GetCaptureLatencyMs() const {
    return 0.0;
}

StreamHealth WindowCapture::GetHealth() const {
    StreamHealth health;
    health.connected = is_running_;
    return health;
}

const VideoConfig& WindowCapture::GetVideoConfig() const {
    return video_config_;
}

AudioCapture::AudioCapture(const std::string& device) : device_(device) {
    STREAMX_INFO("AudioCapture created for device: " + device);
}

AudioCapture::~AudioCapture() {
    Close();
}

bool AudioCapture::Initialize(const VideoConfig& video_config, const AudioConfig& audio_config) {
    audio_config_ = audio_config;
    STREAMX_INFO("AudioCapture initialized");
    return true;
}

bool AudioCapture::Start() {
    is_running_ = true;
    STREAMX_INFO("AudioCapture started");
    return true;
}

bool AudioCapture::Stop() {
    is_running_ = false;
    return true;
}

FramePtr AudioCapture::CaptureFrame(std::chrono::milliseconds timeout) {
    if (!is_running_) return nullptr;
    auto frame = std::make_shared<Frame>(audio_config_.sample_rate, audio_config_.channels,
                                        audio_config_.sample_format, audio_config_.sample_rate / 100);
    frames_captured_++;
    return frame;
}

void AudioCapture::Close() {
    is_running_ = false;
}

double AudioCapture::GetCaptureLatencyMs() const {
    return 0.0;
}

StreamHealth AudioCapture::GetHealth() const {
    StreamHealth health;
    health.connected = is_running_;
    return health;
}

const VideoConfig& AudioCapture::GetVideoConfig() const {
    return video_config_;
}

std::unique_ptr<streamx::ICapture> streamx::CaptureFactory::CreateCapture(int display_index) {
    streamx::ICapture* cap = new streamx::DesktopCapture(static_cast<uint32_t>(display_index));
    return std::unique_ptr<streamx::ICapture>(cap);
}

} // namespace streamx