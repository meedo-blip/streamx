#include "streamx/utils/thread_pool.h"
#include "streamx/utils/logger.h"

namespace streamx {

ThreadPool::ThreadPool(uint32_t num_threads)
    : num_threads_(num_threads), shutdown_(false), active_tasks_(0) {
    workers_.reserve(num_threads);

    for (uint32_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::WorkerThread, this);
    }

    STREAMX_INFO("ThreadPool created with " + std::to_string(num_threads) + " threads");
}

ThreadPool::~ThreadPool() {
    Shutdown();
}

void ThreadPool::WorkerThread() {
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);

        // Wait for task or shutdown
        cv_.wait(lock, [this] { return !tasks_.empty() || shutdown_; });

        if (shutdown_ && tasks_.empty()) {
            break;
        }

        if (tasks_.empty()) {
            continue;
        }

        auto task = std::move(tasks_.front());
        tasks_.pop();
        active_tasks_++;
        lock.unlock();

        try {
            task();
        } catch (const std::exception& e) {
            STREAMX_ERROR("Exception in thread pool task: " + std::string(e.what()));
        }

        lock.lock();
        active_tasks_--;
        idle_cv_.notify_all();
    }
}

void ThreadPool::WaitAll() {
    std::unique_lock<std::mutex> lock(mutex_);
    idle_cv_.wait(lock, [this] { return tasks_.empty() && active_tasks_ == 0; });
}

void ThreadPool::Shutdown() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        shutdown_ = true;
    }

    cv_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();
    STREAMX_INFO("ThreadPool shut down");
}

}  // namespace streamx
