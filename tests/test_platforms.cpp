#include <iostream>
#include <cassert>
#include "streamx/platform/streaming_platform.h"
#include "streamx/platform/platform_implementations.h"
#include "streamx/utils/logger.h"

using namespace streamx;

void TestPlatformManager() {
    std::cout << "Testing StreamingPlatformManager..." << std::endl;

    StreamingPlatformManager manager;

    // Add platforms
    manager.AddPlatform("twitch", std::make_unique<TwitchPlatform>());
    manager.AddPlatform("youtube", std::make_unique<YouTubePlatform>());

    auto platforms = manager.GetAllPlatforms();
    assert(platforms.size() == 2);

    std::cout << "  ✓ PlatformManager add/get succeeded" << std::endl;
}

int main() {
    Logger::Instance().Initialize("");

    std::cout << "\n=== Platform Tests ===" << std::endl;

    TestPlatformManager();

    std::cout << "\n✓ Platform tests passed!" << std::endl;
    return 0;
}
