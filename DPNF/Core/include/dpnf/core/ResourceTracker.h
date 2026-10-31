#pragma once

#include "dpnf/core/Error.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace dpnf::core {

struct AllocationRecord {
    std::uint64_t allocation_id{0};
    std::string tag{};
    std::string subsystem{};
    std::uint64_t size_bytes{0};
};

struct ResourceSnapshot {
    std::uint64_t total_allocated_bytes{0};
    std::uint64_t current_used_bytes{0};
    std::uint64_t peak_used_bytes{0};
    std::uint64_t allocation_count{0};
    std::uint64_t deallocation_count{0};
    std::uint64_t active_allocation_count{0};
    std::vector<AllocationRecord> active_allocations{};
};

class ResourceTracker {
public:
    ResourceTracker() = default;

    Result<std::uint64_t> record_allocation(
        const std::string& tag,
        const std::string& subsystem,
        std::uint64_t size_bytes,
        std::uint64_t max_budget_bytes = 0);

    Error record_deallocation(std::uint64_t allocation_id);

    [[nodiscard]] ResourceSnapshot snapshot() const;
    void reset();

private:
    mutable std::mutex mutex_{};
    std::uint64_t next_id_{1};
    std::uint64_t total_allocated_bytes_{0};
    std::uint64_t current_used_bytes_{0};
    std::uint64_t peak_used_bytes_{0};
    std::uint64_t allocation_count_{0};
    std::uint64_t deallocation_count_{0};
    std::unordered_map<std::uint64_t, AllocationRecord> active_{};
};

} // namespace dpnf::core
