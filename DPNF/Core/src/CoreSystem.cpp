#include "dpnf/core/CoreSystem.hpp"

#include <sstream>

namespace dpnf::core {

namespace {

std::string json_escape(const std::string& input) {
    std::ostringstream oss;
    for (char c : input) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << c; break;
        }
    }
    return oss.str();
}

} // namespace

std::string DiagnosticReport::to_human_readable() const {
    std::ostringstream oss;
    oss << "=======================================================================\n";
    oss << "                 DP NEURAL FRAMEWORK (DPNF) DIAGNOSTICS                \n";
    oss << "=======================================================================\n";
    oss << "Framework Version      : " << framework_version << "\n";
    oss << "Lifecycle State        : " << (initialized ? "INITIALIZED" : "SHUTDOWN") << "\n";
    oss << "Application Name       : " << config.application_name << "\n";
    oss << "Active Backend         : " << active_execution_backend << "\n";
    oss << "Device Policy          : " << config::FrameworkConfig::device_policy_to_string(config.device_policy) << "\n";
    oss << "Operating System       : " << platform.os_name << " (" << platform.kernel_version << ")\n";
    oss << "Architecture           : " << platform::PlatformDetector::arch_to_string(platform.architecture) << "\n";
    oss << "Compiler               : " << platform.compiler_name << " (C++ std " << platform.cpp_standard << ")\n";
    oss << "CPU Model              : " << platform.cpu.model_name << " (" << platform.cpu.vendor_id << ")\n";
    oss << "CPU Topology           : " << platform.cpu.physical_cores << " physical / "
        << platform.cpu.logical_cores << " logical cores\n";

    const auto isas = platform::PlatformDetector::supported_isa_list(platform.cpu);
    oss << "Supported CPU ISAs     : ";
    if (isas.empty()) {
        oss << "Baseline scalar";
    } else {
        for (std::size_t i = 0; i < isas.size(); ++i) {
            oss << isas[i] << (i + 1 < isas.size() ? ", " : "");
        }
    }
    oss << "\n";

    oss << "System Memory (Total)  : " << (platform.memory.total_physical_bytes / (1024ULL * 1024ULL)) << " MiB\n";
    oss << "System Memory (Avail)  : " << (platform.memory.available_physical_bytes / (1024ULL * 1024ULL)) << " MiB\n";
    oss << "Vulkan Loader Present  : " << (platform.vulkan.loader_present ? "YES" : "NO") << "\n";
    oss << "Vulkan Status Note     : " << platform.vulkan.diagnostic_note << "\n";
    oss << "Worker Thread Pool     : " << worker_thread_count << " workers (" << completed_jobs << " jobs completed)\n";
    oss << "Resource Allocations   : " << resources.allocation_count << " allocs / "
        << resources.deallocation_count << " deallocs ("
        << resources.active_allocation_count << " active)\n";
    oss << "Tracked Memory (Peak)  : " << resources.peak_used_bytes << " bytes (current: "
        << resources.current_used_bytes << " bytes)\n";
    oss << "=======================================================================\n";
    return oss.str();
}

std::string DiagnosticReport::to_json() const {
    const auto isas = platform::PlatformDetector::supported_isa_list(platform.cpu);
    const auto logs = Logger::instance().recent_entries();

    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"framework_version\": \"" << json_escape(framework_version) << "\",\n";
    oss << "  \"initialized\": " << (initialized ? "true" : "false") << ",\n";
    oss << "  \"active_execution_backend\": \"" << json_escape(active_execution_backend) << "\",\n";
    oss << "  \"config\": {\n";
    oss << "    \"application_name\": \"" << json_escape(config.application_name) << "\",\n";
    oss << "    \"log_level\": \"" << json_escape(config.log_level) << "\",\n";
    oss << "    \"worker_threads\": " << config.worker_threads << ",\n";
    oss << "    \"random_seed\": " << config.random_seed << ",\n";
    oss << "    \"deterministic_execution\": " << (config.deterministic_execution ? "true" : "false") << ",\n";
    oss << "    \"strict_bounds_checking\": " << (config.strict_bounds_checking ? "true" : "false") << ",\n";
    oss << "    \"device_policy\": \"" << json_escape(config::FrameworkConfig::device_policy_to_string(config.device_policy)) << "\"\n";
    oss << "  },\n";
    oss << "  \"platform\": {\n";
    oss << "    \"os_name\": \"" << json_escape(platform.os_name) << "\",\n";
    oss << "    \"kernel_version\": \"" << json_escape(platform.kernel_version) << "\",\n";
    oss << "    \"architecture\": \"" << json_escape(platform::PlatformDetector::arch_to_string(platform.architecture)) << "\",\n";
    oss << "    \"compiler_name\": \"" << json_escape(platform.compiler_name) << "\",\n";
    oss << "    \"cpp_standard\": " << platform.cpp_standard << ",\n";
    oss << "    \"cpu\": {\n";
    oss << "      \"vendor_id\": \"" << json_escape(platform.cpu.vendor_id) << "\",\n";
    oss << "      \"model_name\": \"" << json_escape(platform.cpu.model_name) << "\",\n";
    oss << "      \"physical_cores\": " << platform.cpu.physical_cores << ",\n";
    oss << "      \"logical_cores\": " << platform.cpu.logical_cores << ",\n";
    oss << "      \"supported_isas\": [";
    for (std::size_t i = 0; i < isas.size(); ++i) {
        oss << "\"" << json_escape(isas[i]) << "\"" << (i + 1 < isas.size() ? ", " : "");
    }
    oss << "]\n";
    oss << "    },\n";
    oss << "    \"memory\": {\n";
    oss << "      \"total_physical_bytes\": " << platform.memory.total_physical_bytes << ",\n";
    oss << "      \"available_physical_bytes\": " << platform.memory.available_physical_bytes << ",\n";
    oss << "      \"page_size_bytes\": " << platform.memory.page_size_bytes << "\n";
    oss << "    },\n";
    oss << "    \"vulkan\": {\n";
    oss << "      \"loader_present\": " << (platform.vulkan.loader_present ? "true" : "false") << ",\n";
    oss << "      \"loader_library_path\": \"" << json_escape(platform.vulkan.loader_library_path) << "\",\n";
    oss << "      \"diagnostic_note\": \"" << json_escape(platform.vulkan.diagnostic_note) << "\"\n";
    oss << "    }\n";
    oss << "  },\n";
    oss << "  \"runtime\": {\n";
    oss << "    \"worker_thread_count\": " << worker_thread_count << ",\n";
    oss << "    \"completed_jobs\": " << completed_jobs << ",\n";
    oss << "    \"resources\": {\n";
    oss << "      \"total_allocated_bytes\": " << resources.total_allocated_bytes << ",\n";
    oss << "      \"current_used_bytes\": " << resources.current_used_bytes << ",\n";
    oss << "      \"peak_used_bytes\": " << resources.peak_used_bytes << ",\n";
    oss << "      \"allocation_count\": " << resources.allocation_count << ",\n";
    oss << "      \"deallocation_count\": " << resources.deallocation_count << ",\n";
    oss << "      \"active_allocation_count\": " << resources.active_allocation_count << "\n";
    oss << "    }\n";
    oss << "  },\n";
    oss << "  \"logs\": [\n";
    for (std::size_t i = 0; i < logs.size(); ++i) {
        const auto& entry = logs[i];
        oss << "    {\n";
        oss << "      \"sequence_id\": " << entry.sequence_id << ",\n";
        oss << "      \"timestamp\": \"" << json_escape(entry.timestamp_iso) << "\",\n";
        oss << "      \"level\": \"" << json_escape(Logger::level_to_string(entry.level)) << "\",\n";
        oss << "      \"subsystem\": \"" << json_escape(entry.subsystem) << "\",\n";
        oss << "      \"message\": \"" << json_escape(entry.message) << "\"\n";
        oss << "    }" << (i + 1 < logs.size() ? "," : "") << "\n";
    }
    oss << "  ]\n";
    oss << "}\n";
    return oss.str();
}

CoreSystem& CoreSystem::instance() {
    static CoreSystem sys;
    return sys;
}

Error CoreSystem::initialize(const config::FrameworkConfig& cfg) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        return DPNF_MAKE_ERROR(
            ErrorCode::DPNF_ERROR_INVALID_ARGUMENT,
            "CoreSystem::initialize called while framework is already initialized.",
            "CoreSystem");
    }

    std::string validation_error;
    if (!cfg.validate(validation_error)) {
        return DPNF_MAKE_ERROR(
            ErrorCode::DPNF_ERROR_INVALID_ARGUMENT,
            "FrameworkConfig validation failed: " + validation_error,
            "CoreSystem");
    }

    config_ = cfg;
    Logger::instance().configure(
        Logger::string_to_level(config_.log_level),
        config_.log_to_console,
        config_.log_file_path);

    Logger::instance().log(
        LogLevel::Info,
        "CoreSystem",
        "Initializing DP Neural Framework (DPNF) v" + std::string(version()) +
            " for application '" + config_.application_name + "'...");

    platform_ = platform::PlatformDetector::detect();
    Logger::instance().log(
        LogLevel::Info,
        "Platform",
        "Detected OS: " + platform_.os_name + " | Arch: " +
            platform::PlatformDetector::arch_to_string(platform_.architecture) +
            " | Logical CPUs: " + std::to_string(platform_.cpu.logical_cores));

    if (config_.device_policy == config::DevicePolicy::PreferGPU && !platform_.vulkan.loader_present) {
        Logger::instance().log(
            LogLevel::Warn,
            "Platform",
            "DevicePolicy::PreferGPU requested, but Vulkan loader is unavailable. Falling back to CPU execution.");
        active_backend_ = "CPU (Fallback)";
    } else if (config_.device_policy == config::DevicePolicy::CPUOnly) {
        active_backend_ = "CPU (Explicit)";
    } else {
        active_backend_ = "CPU";
    }

    resource_tracker_.reset();
    thread_pool_ = std::make_unique<ThreadPool>(config_.worker_threads);
    Logger::instance().log(
        LogLevel::Info,
        "ThreadPool",
        "Initialized worker thread pool with " + std::to_string(thread_pool_->worker_count()) + " threads.");

    initialized_ = true;
    Logger::instance().log(LogLevel::Info, "CoreSystem", "DPNF Phase 0 initialization complete.");
    return Error::ok();
}

Error CoreSystem::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        return DPNF_MAKE_ERROR(
            ErrorCode::DPNF_ERROR_INVALID_ARGUMENT,
            "CoreSystem::shutdown called when framework is not initialized.",
            "CoreSystem");
    }

    if (thread_pool_) {
        thread_pool_->wait_idle();
        thread_pool_.reset();
    }

    const auto snap = resource_tracker_.snapshot();
    if (snap.active_allocation_count > 0) {
        Logger::instance().log(
            LogLevel::Warn,
            "ResourceTracker",
            "Shutdown detected " + std::to_string(snap.active_allocation_count) +
                " unreleased resource allocation(s) totaling " +
                std::to_string(snap.current_used_bytes) + " bytes.");
    } else {
        Logger::instance().log(
            LogLevel::Info,
            "ResourceTracker",
            "All tracked resources cleanly released (" +
                std::to_string(snap.allocation_count) + " total allocations).");
    }

    initialized_ = false;
    Logger::instance().log(LogLevel::Info, "CoreSystem", "DPNF shutdown complete.");
    return Error::ok();
}

bool CoreSystem::is_initialized() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_;
}

const config::FrameworkConfig& CoreSystem::config() const {
    return config_;
}

const platform::PlatformReport& CoreSystem::platform_report() const {
    return platform_;
}

ResourceTracker& CoreSystem::resource_tracker() {
    return resource_tracker_;
}

ThreadPool* CoreSystem::thread_pool() {
    return thread_pool_.get();
}

DiagnosticReport CoreSystem::generate_diagnostic_report() const {
    std::lock_guard<std::mutex> lock(mutex_);
    DiagnosticReport report{};
    report.framework_version = version();
    report.initialized = initialized_;
    report.active_execution_backend = active_backend_;
    report.config = config_;
    report.platform = platform_;
    report.resources = resource_tracker_.snapshot();
    if (thread_pool_) {
        report.worker_thread_count = thread_pool_->worker_count();
        report.completed_jobs = thread_pool_->completed_task_count();
    }
    return report;
}

} // namespace dpnf::core
