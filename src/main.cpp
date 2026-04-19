#include "streamx/streaming_controller.h"
#include "streamx/utils/logger.h"
#include "streamx/utils/config.h"
#include <iostream>
#include <string>
#include <map>
#include <iomanip>

using namespace streamx;

void PrintHelp() {
    std::cout << R"(
=== StreamX - Multi-Platform Streaming Application ===

Usage: streamx [COMMAND] [OPTIONS]

Commands:
  help                Show this help message
  start               Start streaming
  stop                Stop streaming
  status              Show streaming status
  add-platform        Add a streaming platform
  remove-platform     Remove a streaming platform
  list-platforms      List all configured platforms
  config              Manage configuration
  monitor             Real-time monitoring dashboard

Examples:
  # Add Twitch platform
  streamx add-platform twitch "My Stream Key"

  # Add YouTube platform
  streamx add-platform youtube "My Stream Key"

  # Add StreamLabs
  streamx add-platform streamlabs "My Stream Key"

  # Start streaming to all platforms
  streamx start

  # Show status
  streamx status

  # Monitor in real-time
  streamx monitor
)" << std::endl;
}

void ShowStatus(StreamingController& controller) {
    std::cout << "\n=== Streaming Status ===" << std::endl;
    
    std::cout << "Streaming: " << (controller.IsStreaming() ? "YES" : "NO") << std::endl;
    std::cout << "Capturing: " << (controller.IsCapturing() ? "YES" : "NO") << std::endl;

    auto platforms = controller.GetConnectedPlatforms();
    std::cout << "Connected Platforms: " << platforms.size() << std::endl;
    for (const auto& platform : platforms) {
        std::cout << "  - " << platform << std::endl;
    }

    auto health_map = controller.GetPlatformHealth();
    if (!health_map.empty()) {
        std::cout << "\nPlatform Health:" << std::endl;
        std::cout << std::left << std::setw(20) << "Platform" 
                  << std::setw(15) << "Status" 
                  << std::setw(15) << "Bitrate (kbps)"
                  << std::setw(15) << "Latency (ms)" << std::endl;
        std::cout << std::string(65, '-') << std::endl;

        for (const auto& [name, health] : health_map) {
            std::cout << std::left 
                      << std::setw(20) << name
                      << std::setw(15) << (health.connected ? "Connected" : "Disconnected")
                      << std::setw(15) << std::fixed << std::setprecision(1) << health.bitrate_actual_kbps
                      << std::setw(15) << health.network_latency_ms << std::endl;
        }
    }
    std::cout << std::endl;
}

void MonitorDashboard(StreamingController& controller) {
    std::cout << "=== Real-Time Monitoring (Press Ctrl+C to exit) ===" << std::endl;
    std::cout.flush();

    while (true) {
        // Clear screen (simple cross-platform approach)
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif

        ShowStatus(controller);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main(int argc, char* argv[]) {
    // Initialize logger
    Logger::Instance().Initialize("streamx.log", LogLevel::Info);
    STREAMX_INFO("=== StreamX Application Started ===");

    // Initialize config
    Config& config = Config::Instance();
    config.LoadFromFile("streamx_config.json");

    // Create controller
    StreamingController controller;

    // Default video and audio config
    VideoConfig video_config{
        .width = 1920,
        .height = 1080,
        .fps = 60,
        .bitrate_kbps = 5000,
        .codec = CodecType::H264,
        .pixel_format = PixelFormat::YUV420P,
        .preset = "medium"
    };

    AudioConfig audio_config{
        .sample_rate = 48000,
        .channels = 2,
        .bitrate_kbps = 128,
        .codec = AudioCodecType::AAC
    };

    if (!controller.Initialize(video_config, audio_config)) {
        STREAMX_ERROR("Failed to initialize controller");
        return 1;
    }

    // Parse command line arguments
    if (argc < 2) {
        PrintHelp();
        return 0;
    }

    std::string command = argv[1];

    if (command == "help") {
        PrintHelp();
    }
    else if (command == "add-platform") {
        if (argc < 4) {
            std::cout << "Usage: streamx add-platform <platform_type> <stream_key>" << std::endl;
            return 1;
        }

        std::string platform_type = argv[2];
        std::string stream_key = argv[3];

        if (controller.AddPlatform(platform_type, platform_type)) {
            std::cout << "Platform added: " << platform_type << std::endl;

            // Create config
            StreamConfig stream_config{
                .video = video_config,
                .audio = audio_config,
                .stream_key = stream_key,
                .server_url = "rtmps://live-ams.twitch.tv/app/"
            };

            Credentials creds;
            if (controller.ConnectAll(creds, stream_config)) {
                std::cout << "Connected to " << platform_type << std::endl;
            } else {
                std::cout << "Failed to connect to " << platform_type << std::endl;
                return 1;
            }
        } else {
            std::cout << "Failed to add platform" << std::endl;
            return 1;
        }
    }
    else if (command == "start") {
        std::cout << "Starting capture and streaming..." << std::endl;

        if (!controller.StartCapture(0, "")) {
            std::cout << "Failed to start capture" << std::endl;
            return 1;
        }

        if (!controller.StartStreaming()) {
            std::cout << "Failed to start streaming" << std::endl;
            return 1;
        }

        std::cout << "Streaming started successfully!" << std::endl;
        std::cout << "Press Ctrl+C to stop..." << std::endl;

        // Keep running until interrupted
        while (controller.IsStreaming()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    else if (command == "stop") {
        if (controller.IsStreaming()) {
            controller.StopStreaming();
            std::cout << "Streaming stopped" << std::endl;
        } else {
            std::cout << "Streaming is not running" << std::endl;
        }
    }
    else if (command == "status") {
        ShowStatus(controller);
    }
    else if (command == "list-platforms") {
        auto platforms = controller.GetConnectedPlatforms();
        if (platforms.empty()) {
            std::cout << "No platforms connected" << std::endl;
        } else {
            std::cout << "Connected Platforms:" << std::endl;
            for (const auto& platform : platforms) {
                std::cout << "  - " << platform << std::endl;
            }
        }
    }
    else if (command == "monitor") {
        MonitorDashboard(controller);
    }
    else if (command == "config") {
        if (argc < 3) {
            std::cout << "Usage: streamx config <action>" << std::endl;
            std::cout << "Actions: save, load, show" << std::endl;
            return 1;
        }

        std::string action = argv[2];
        if (action == "save") {
            config.SaveToFile("streamx_config.json");
            std::cout << "Config saved" << std::endl;
        } else if (action == "load") {
            config.LoadFromFile("streamx_config.json");
            std::cout << "Config loaded" << std::endl;
        } else if (action == "show") {
            std::cout << config.GetRaw().dump(2) << std::endl;
        }
    }
    else {
        std::cout << "Unknown command: " << command << std::endl;
        PrintHelp();
        return 1;
    }

    STREAMX_INFO("=== StreamX Application Ended ===");
    return 0;
}
