#include "dpnf/memory/MemoryPool.h"
#include "dpnf/core/CoreSystem.h"

#include <cstdlib>
#include <cstring>

namespace dpnf::memory {

namespace {

std::size_t round_up_alignment(std::size_t bytes, std::size_t align = 64) {
    if (bytes == 0) {
        return align;
    }
    return ((bytes + align - 1) / align) * align;
}

} // namespace

PooledBuffer::PooledBuffer(std::size_t size_bytes, const std::string& tag)
    : size_bytes_(round_up_alignment(size_bytes, 64)) {
    data_ = MemoryPool::instance().allocate(size_bytes_);
    if (data_ != nullptr) {
        std::memset(data_, 0, size_bytes_);
        auto& core = core::CoreSystem::instance();
        if (core.is_initialized()) {
            auto res = core.resource_tracker().record_allocation(tag, "MemoryPool", size_bytes_);
            if (res.is_ok()) {
                tracker_id_ = res.value();
            }
        }
    }
}

PooledBuffer::~PooledBuffer() {
    if (data_ != nullptr) {
        auto& core = core::CoreSystem::instance();
        if (core.is_initialized() && tracker_id_ != 0) {
            (void)core.resource_tracker().record_deallocation(tracker_id_);
        }
        MemoryPool::instance().deallocate(data_, size_bytes_);
        data_ = nullptr;
    }
}

MemoryPool& MemoryPool::instance() {
    static MemoryPool pool;
    return pool;
}

MemoryPool::~MemoryPool() {
    release_cached_blocks();
}

void* MemoryPool::allocate(std::size_t size_bytes) {
    const std::size_t aligned_sz = round_up_alignment(size_bytes, 64);
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = free_blocks_.find(aligned_sz);
    if (it != free_blocks_.end() && !it->second.empty()) {
        void* ptr = it->second.back();
        it->second.pop_back();
        stats_.cached_free_bytes -= aligned_sz;
        stats_.currently_used_bytes += aligned_sz;
        if (stats_.currently_used_bytes > stats_.peak_used_bytes) {
            stats_.peak_used_bytes = stats_.currently_used_bytes;
        }
        ++stats_.cache_hits;
        return ptr;
    }

    void* ptr = std::aligned_alloc(64, aligned_sz);
    if (ptr != nullptr) {
        stats_.total_allocated_bytes += aligned_sz;
        stats_.currently_used_bytes += aligned_sz;
        if (stats_.currently_used_bytes > stats_.peak_used_bytes) {
            stats_.peak_used_bytes = stats_.currently_used_bytes;
        }
        ++stats_.cache_misses;
    }
    return ptr;
}

void MemoryPool::deallocate(void* ptr, std::size_t size_bytes) {
    if (ptr == nullptr) {
        return;
    }
    const std::size_t aligned_sz = round_up_alignment(size_bytes, 64);
    std::lock_guard<std::mutex> lock(mutex_);
    if (stats_.currently_used_bytes >= aligned_sz) {
        stats_.currently_used_bytes -= aligned_sz;
    }
    stats_.cached_free_bytes += aligned_sz;
    free_blocks_[aligned_sz].push_back(ptr);
}

void MemoryPool::release_cached_blocks() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [sz, vec] : free_blocks_) {
        (void)sz;
        for (void* ptr : vec) {
            std::free(ptr);
        }
        vec.clear();
    }
    free_blocks_.clear();
    stats_.cached_free_bytes = 0;
}

MemoryPoolStats MemoryPool::stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

} // namespace dpnf::memory
