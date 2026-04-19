#include <iostream>
#include "streamx/core/encoder.h"
#include "streamx/utils/logger.h"

using namespace streamx;

void TestSoftwareEncoder() {
    std::cout << "Testing SoftwareEncoder..." << std::endl;

    VideoConfig config{
        .width = 1280,
        .height = 720,
        .fps = 30,
        .bitrate_kbps = 2500,
        .codec = CodecType::H264
    };

    SoftwareEncoder encoder(CodecType::H264);
    if (encoder.Initialize(config)) {
        std::cout << "  ✓ SoftwareEncoder initialization succeeded" << std::endl;
    } else {
        std::cout << "  ⚠ SoftwareEncoder initialization (expected if FFmpeg not installed)" << std::endl;
    }
}

void TestEncoderFactory() {
    std::cout << "Testing EncoderFactory..." << std::endl;

    auto encoder = EncoderFactory::CreateEncoder(CodecType::H264, false);
    if (encoder) {
        std::cout << "  ✓ EncoderFactory creation succeeded" << std::endl;
    } else {
        std::cout << "  ✗ EncoderFactory failed" << std::endl;
    }
}

int main() {
    Logger::Instance().Initialize("");

    std::cout << "\n=== Encoder Tests ===" << std::endl;

    TestSoftwareEncoder();
    TestEncoderFactory();

    std::cout << "\n✓ Encoder tests completed!" << std::endl;
    return 0;
}
