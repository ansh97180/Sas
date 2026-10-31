#pragma once

#include "dpnf/config/FrameworkConfig.hpp"
#include "dpnf/core/Error.hpp"
#include "dpnf/core/Logger.hpp"
#include "dpnf/core/ResourceTracker.hpp"
#include "dpnf/core/ThreadPool.hpp"
#include "dpnf/platform/PlatformInfo.hpp"

#include <memory>
#include <mutex>
#include <string>

namespace dpnf::core {

struct DiagnosticReport {
    std::string framework_version{"0.1.0-phase0"};
    bool initialized{false};
    std::string active_execution_backend{"CPU"};
    config::FrameworkConfig config{};
    platform::PlatformReport platform{};
    ResourceSnapshot resources{};
    std::uint32_t worker_thread_count{0};
    std::uint64_t completed_jobs{0};

    [[nodiscard]] std::string to_human_readable() const;
    [[nodiscard]] std::string to_json() const;
};

class CoreSystem {
public:
    static CoreSystem& instance();

    Error initialize(const config::FrameworkConfig& cfg = config::FrameworkConfig{});
    Error shutdown();

    [[nodiscard]] bool is_initialized() const noexcept;
    [[nodiscard]] const config::FrameworkConfig& config() const;
    [[nodiscard]] const platform::PlatformReport& platform_report() const;
    [[nodiscard]] ResourceTracker& resource_tracker();
    [[nodiscard]] ThreadPool* thread_pool();

    [[nodiscard]] DiagnosticReport generate_diagnostic_report() const;

    static constexpr const char* version() noexcept {
        return "0.1.0-phase0";
    }

private:
    CoreSystem() = default;

    mutable std::mutex mutex_{};
    bool initialized_{false};
    std::string active_backend_{"CPU"};
    config::FrameworkConfig config_{};
    platform::PlatformReport platform_{};
    ResourceTracker resource_tracker_{};
    std::unique_ptr<ThreadPool> thread_pool_{};
};

} // namespace dpnf::core
