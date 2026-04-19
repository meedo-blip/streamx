#pragma once

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <vector>

namespace streamx {

// Thread pool for async tasks
class ThreadPool {
public:
    explicit ThreadPool(uint32_t num_threads = 4);
    ~ThreadPool();

    // Delete copy operations
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Submit a task
    template<typename Func, typename... Args>
    void Submit(Func&& func, Args&&... args) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (shutdown_) {
                return;
            }

            tasks_.emplace(std::bind(std::forward<Func>(func), 
                                    std::forward<Args>(args)...));
        }
        cv_.notify_one();
    }

    // Wait for all tasks to complete
    void WaitAll();

    // Shutdown thread pool
    void Shutdown();

    // Get number of threads
    uint32_t GetThreadCount() const { return num_threads_; }

    // Get number of pending tasks
    uint32_t GetPendingTasks() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return tasks_.size();
    }

private:
    uint32_t num_threads_;
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable idle_cv_;
    bool shutdown_ = false;
    uint32_t active_tasks_ = 0;

    void WorkerThread();
};

}  // namespace streamx
