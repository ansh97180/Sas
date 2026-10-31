#include "dpnf/core/ResourceTracker.h"

#include <algorithm>

namespace dpnf::core {

Result<std::uint64_t> ResourceTracker::record_allocation(
    const std::string& tag,
    const std::string& subsystem,
    std::uint64_t size_bytes,
    std::uint64_t max_budget_bytes) {
    if (size_bytes == 0) {
        return Result<std::uint64_t>::failure(
            DPNF_MAKE_ERROR(
                ErrorCode::DPNF_ERROR_INVALID_ARGUMENT,
                "Cannot record zero-byte resource allocation for tag '" + tag + "'.",
                "ResourceTracker"));
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (max_budget_bytes > 0 && (current_used_bytes_ + size_bytes > max_budget_bytes)) {
        return Result<std::uint64_t>::failure(
            DPNF_MAKE_ERROR(
                ErrorCode::DPNF_ERROR_OUT_OF_MEMORY,
                "Allocation of " + std::to_string(size_bytes) +
                    " bytes for '" + tag + "' exceeds configured memory budget (" +
                    std::to_string(max_budget_bytes) + " bytes).",
                "ResourceTracker"));
    }

    const std::uint64_t id = next_id_++;
    AllocationRecord rec{};
    rec.allocation_id = id;
    rec.tag = tag;
    rec.subsystem = subsystem;
    rec.size_bytes = size_bytes;

    active_.emplace(id, std::move(rec));
    total_allocated_bytes_ += size_bytes;
    current_used_bytes_ += size_bytes;
    if (current_used_bytes_ > peak_used_bytes_) {
        peak_used_bytes_ = current_used_bytes_;
    }
    ++allocation_count_;

    return Result<std::uint64_t>::success(id);
}

Error ResourceTracker::record_deallocation(std::uint64_t allocation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = active_.find(allocation_id);
    if (it == active_.end()) {
        return DPNF_MAKE_ERROR(
            ErrorCode::DPNF_ERROR_INVALID_ARGUMENT,
            "Invalid or double-freed allocation ID " + std::to_string(allocation_id) + ".",
            "ResourceTracker");
    }

    current_used_bytes_ -= it->second.size_bytes;
    active_.erase(it);
    ++deallocation_count_;
    return Error::ok();
}

ResourceSnapshot ResourceTracker::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    ResourceSnapshot snap{};
    snap.total_allocated_bytes = total_allocated_bytes_;
    snap.current_used_bytes = current_used_bytes_;
    snap.peak_used_bytes = peak_used_bytes_;
    snap.allocation_count = allocation_count_;
    snap.deallocation_count = deallocation_count_;
    snap.active_allocation_count = static_cast<std::uint64_t>(active_.size());
    snap.active_allocations.reserve(active_.size());
    for (const auto& [id, rec] : active_) {
        (void)id;
        snap.active_allocations.push_back(rec);
    }
    std::sort(
        snap.active_allocations.begin(),
        snap.active_allocations.end(),
        [](const AllocationRecord& a, const AllocationRecord& b) {
            return a.allocation_id < b.allocation_id;
        });
    return snap;
}

void ResourceTracker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    next_id_ = 1;
    total_allocated_bytes_ = 0;
    current_used_bytes_ = 0;
    peak_used_bytes_ = 0;
    allocation_count_ = 0;
    deallocation_count_ = 0;
    active_.clear();
}

} // namespace dpnf::core
