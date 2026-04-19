#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>
#include "frame.h"

namespace streamx {

// Thread-safe ring buffer for media frames
template<typename T>
class FrameBuffer {
public:
    explicit FrameBuffer(uint32_t capacity = 120)
        : capacity_(capacity), read_index_(0), write_index_(0), frame_count_(0) {
        buffer_.reserve(capacity);
        for (uint32_t i = 0; i < capacity; ++i) {
            buffer_.emplace_back(nullptr);
        }
    }

    // Try to push a frame (non-blocking, drops frames if full)
    bool TryPush(std::shared_ptr<T> frame) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (frame_count_ >= capacity_) {
            // Buffer full, drop oldest frame (read oldest frame first)
            read_index_ = (read_index_ + 1) % capacity_;
            frame_count_--;
        }

        buffer_[write_index_] = frame;
        write_index_ = (write_index_ + 1) % capacity_;
        frame_count_++;

        cv_.notify_one();
        return true;
    }

    // Push with timeout
    bool Push(std::shared_ptr<T> frame, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        std::unique_lock<std::mutex> lock(mutex_);

        // Wait for space or timeout
        while (frame_count_ >= capacity_) {
            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now());
            
            if (remaining.count() <= 0) {
                return false;  // Timeout
            }

            if (!cv_.wait_for(lock, remaining, [this] { return frame_count_ < capacity_; })) {
                return false;
            }
        }

        buffer_[write_index_] = frame;
        write_index_ = (write_index_ + 1) % capacity_;
        frame_count_++;

        cv_.notify_one();
        return true;
    }

    // Pop frame (blocking with timeout)
    std::shared_ptr<T> Pop(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        std::unique_lock<std::mutex> lock(mutex_);

        while (frame_count_ == 0) {
            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now());
            
            if (remaining.count() <= 0) {
                return nullptr;  // Timeout
            }

            if (!cv_.wait_for(lock, remaining, [this] { return frame_count_ > 0; })) {
                return nullptr;
            }
        }

        auto frame = buffer_[read_index_];
        read_index_ = (read_index_ + 1) % capacity_;
        frame_count_--;

        cv_.notify_one();
        return frame;
    }

    // Try pop (non-blocking)
    std::shared_ptr<T> TryPop() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (frame_count_ == 0) {
            return nullptr;
        }

        auto frame = buffer_[read_index_];
        read_index_ = (read_index_ + 1) % capacity_;
        frame_count_--;

        cv_.notify_one();
        return frame;
    }

    uint32_t GetSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return frame_count_;
    }

    uint32_t GetCapacity() const { return capacity_; }

    bool Empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return frame_count_ == 0;
    }

    bool Full() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return frame_count_ >= capacity_;
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        read_index_ = 0;
        write_index_ = 0;
        frame_count_ = 0;
    }

private:
    std::vector<std::shared_ptr<T>> buffer_;
    uint32_t capacity_;
    uint32_t read_index_;
    uint32_t write_index_;
    uint32_t frame_count_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

}  // namespace streamx
