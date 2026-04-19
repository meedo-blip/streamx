#include <iostream>
#include <cassert>
#include "streamx/core/frame.h"
#include "streamx/utils/logger.h"

using namespace streamx;

void TestVideoFrame() {
    std::cout << "Testing VideoFrame..." << std::endl;

    Frame frame(1920, 1080, PixelFormat::YUV420P);
    assert(frame.GetWidth() == 1920);
    assert(frame.GetHeight() == 1080);
    assert(frame.IsVideo() == true);

    std::cout << "  ✓ VideoFrame creation succeeded" << std::endl;
}

void TestAudioFrame() {
    std::cout << "Testing AudioFrame..." << std::endl;

    Frame frame(48000, 2, SampleFormat::S16, 2048);
    assert(frame.GetSampleRate() == 48000);
    assert(frame.GetChannels() == 2);
    assert(frame.IsAudio() == true);

    std::cout << "  ✓ AudioFrame creation succeeded" << std::endl;
}

void TestPacket() {
    std::cout << "Testing Packet..." << std::endl;

    Packet packet(1024);
    uint8_t data[100] = {0};
    packet.SetData(data, sizeof(data));

    assert(packet.GetSize() == sizeof(data));
    assert(packet.GetCapacity() >= sizeof(data));

    std::cout << "  ✓ Packet creation succeeded" << std::endl;
}

int main() {
    Logger::Instance().Initialize("");

    std::cout << "\n=== StreamX Unit Tests ===" << std::endl;

    TestVideoFrame();
    TestAudioFrame();
    TestPacket();

    std::cout << "\n✓ All tests passed!" << std::endl;
    return 0;
}
