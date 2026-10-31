#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace dpnf::core {

class ThreadPool {
public:
    explicit ThreadPool(std::uint32_t thread_count = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    std::future<void> submit(std::function<void()> task);
    void wait_idle();

    [[nodiscard]] std::uint32_t worker_count() const noexcept {
        return static_cast<std::uint32_t>(workers_.size());
    }

    [[nodiscard]] std::uint64_t completed_task_count() const noexcept {
        return completed_tasks_.load(std::memory_order_relaxed);
    }

private:
    void worker_loop();

    std::vector<std::thread> workers_{};
    std::queue<std::function<void()>> tasks_{};
    mutable std::mutex mutex_{};
    std::condition_variable cv_task_{};
    std::condition_variable cv_idle_{};
    std::uint32_t active_workers_{0};
    bool stop_{false};
    std::atomic<std::uint64_t> completed_tasks_{0};
};

} // namespace dpnf::core
