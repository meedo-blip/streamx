#include <iostream>
#include <cassert>
#include <thread>
#include "streamx/core/buffer.h"
#include "streamx/utils/logger.h"

using namespace streamx;

void TestFrameBuffer() {
    std::cout << "Testing FrameBuffer..." << std::endl;

    FrameBuffer<Frame> buffer(10);

    // Test push and pop
    auto frame = std::make_shared<Frame>(1920, 1080, PixelFormat::YUV420P);
    assert(buffer.TryPush(frame) == true);
    assert(buffer.GetSize() == 1);

    auto popped = buffer.Pop(std::chrono::milliseconds(1000));
    assert(popped != nullptr);
    assert(buffer.GetSize() == 0);

    std::cout << "  ✓ FrameBuffer push/pop succeeded" << std::endl;
}

void TestThreadSafety() {
    std::cout << "Testing FrameBuffer thread safety..." << std::endl;

    FrameBuffer<Frame> buffer(100);
    bool producer_done = false;
    bool consumer_done = false;

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < 50; ++i) {
            auto frame = std::make_shared<Frame>(640, 480, PixelFormat::YUV420P);
            buffer.Push(frame, std::chrono::milliseconds(5000));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        producer_done = true;
    });

    // Consumer thread
    std::thread consumer([&]() {
        int count = 0;
        while (count < 50 || !producer_done) {
            auto frame = buffer.Pop(std::chrono::milliseconds(100));
            if (frame) {
                count++;
            }
        }
        consumer_done = true;
    });

    producer.join();
    consumer.join();

    assert(producer_done && consumer_done);
    std::cout << "  ✓ Thread safety test succeeded" << std::endl;
}

int main() {
    Logger::Instance().Initialize("");

    std::cout << "\n=== FrameBuffer Tests ===" << std::endl;

    TestFrameBuffer();
    TestThreadSafety();

    std::cout << "\n✓ All buffer tests passed!" << std::endl;
    return 0;
}
