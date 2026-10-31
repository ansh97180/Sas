#include "dpnf/core/ThreadPool.h"

#include <algorithm>

namespace dpnf::core {

ThreadPool::ThreadPool(std::uint32_t thread_count) {
    if (thread_count == 0) {
        const unsigned int hw = std::thread::hardware_concurrency();
        thread_count = hw > 0U ? hw : 2U;
    }
    thread_count = std::clamp(thread_count, 1U, 256U);

    workers_.reserve(thread_count);
    for (std::uint32_t i = 0; i < thread_count; ++i) {
        workers_.emplace_back([this]() { worker_loop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cv_task_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

std::future<void> ThreadPool::submit(std::function<void()> task) {
    auto packaged = std::make_shared<std::packaged_task<void()>>(std::move(task));
    std::future<void> res = packaged->get_future();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.emplace([packaged]() { (*packaged)(); });
    }
    cv_task_.notify_one();
    return res;
}

void ThreadPool::wait_idle() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_idle_.wait(lock, [this]() {
        return tasks_.empty() && active_workers_ == 0U;
    });
}

void ThreadPool::worker_loop() {
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_task_.wait(lock, [this]() {
                return stop_ || !tasks_.empty();
            });
            if (stop_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
            ++active_workers_;
        }

        task();
        completed_tasks_.fetch_add(1, std::memory_order_relaxed);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            --active_workers_;
            if (tasks_.empty() && active_workers_ == 0U) {
                cv_idle_.notify_all();
            }
        }
    }
}

} // namespace dpnf::core
