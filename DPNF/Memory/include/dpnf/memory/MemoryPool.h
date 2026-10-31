#pragma once

#include "dpnf/core/Error.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace dpnf::memory {

struct MemoryPoolStats {
    std::uint64_t total_allocated_bytes{0};
    std::uint64_t currently_used_bytes{0};
    std::uint64_t cached_free_bytes{0};
    std::uint64_t peak_used_bytes{0};
    std::uint64_t cache_hits{0};
    std::uint64_t cache_misses{0};
};

class PooledBuffer {
public:
    PooledBuffer(std::size_t size_bytes, const std::string& tag = "tensor_storage");
    ~PooledBuffer();

    PooledBuffer(const PooledBuffer&) = delete;
    PooledBuffer& operator=(const PooledBuffer&) = delete;

    [[nodiscard]] void* data() noexcept { return data_; }
    [[nodiscard]] const void* data() const noexcept { return data_; }
    [[nodiscard]] std::size_t size_bytes() const noexcept { return size_bytes_; }

private:
    void* data_{nullptr};
    std::size_t size_bytes_{0};
    std::uint64_t tracker_id_{0};
};

class MemoryPool {
public:
    static MemoryPool& instance();

    void* allocate(std::size_t size_bytes);
    void deallocate(void* ptr, std::size_t size_bytes);
    void release_cached_blocks();
    [[nodiscard]] MemoryPoolStats stats() const;

private:
    MemoryPool() = default;
    ~MemoryPool();

    mutable std::mutex mutex_{};
    std::unordered_map<std::size_t, std::vector<void*>> free_blocks_{};
    MemoryPoolStats stats_{};
};

} // namespace dpnf::memory
