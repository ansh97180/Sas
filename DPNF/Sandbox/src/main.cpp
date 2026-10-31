#include "dpnf/config/FrameworkConfig.hpp"
#include "dpnf/core/CoreSystem.hpp"
#include "dpnf/core/Error.hpp"
#include "dpnf/core/Logger.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

dpnf::core::ErrorCode parse_error_code_name(const std::string& name) {
    using dpnf::core::ErrorCode;
    if (name == "DPNF_ERROR_INVALID_ARGUMENT") return ErrorCode::DPNF_ERROR_INVALID_ARGUMENT;
    if (name == "DPNF_ERROR_OUT_OF_MEMORY") return ErrorCode::DPNF_ERROR_OUT_OF_MEMORY;
    if (name == "DPNF_ERROR_DEVICE_NOT_FOUND") return ErrorCode::DPNF_ERROR_DEVICE_NOT_FOUND;
    if (name == "DPNF_ERROR_DEVICE_LOST") return ErrorCode::DPNF_ERROR_DEVICE_LOST;
    if (name == "DPNF_ERROR_UNSUPPORTED_OPERATION") return ErrorCode::DPNF_ERROR_UNSUPPORTED_OPERATION;
    if (name == "DPNF_ERROR_INVALID_TENSOR") return ErrorCode::DPNF_ERROR_INVALID_TENSOR;
    if (name == "DPNF_ERROR_INVALID_GRAPH") return ErrorCode::DPNF_ERROR_INVALID_GRAPH;
    if (name == "DPNF_ERROR_COMPILATION_FAILED") return ErrorCode::DPNF_ERROR_COMPILATION_FAILED;
    if (name == "DPNF_ERROR_EXECUTION_FAILED") return ErrorCode::DPNF_ERROR_EXECUTION_FAILED;
    if (name == "DPNF_ERROR_DATASET_ERROR") return ErrorCode::DPNF_ERROR_DATASET_ERROR;
    if (name == "DPNF_ERROR_MODEL_LOAD_FAILED") return ErrorCode::DPNF_ERROR_MODEL_LOAD_FAILED;
    if (name == "DPNF_ERROR_MODEL_SAVE_FAILED") return ErrorCode::DPNF_ERROR_MODEL_SAVE_FAILED;
    if (name == "DPNF_ERROR_TRAINING_FAILED") return ErrorCode::DPNF_ERROR_TRAINING_FAILED;
    return ErrorCode::DPNF_OK;
}

} // namespace

int main(int argc, char** argv) {
    bool json_output = false;
    std::string log_level = "info";
    std::uint32_t threads = 0;
    std::string device_policy_str = "auto";
    std::string simulate_error_code{};

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--json") {
            json_output = true;
        } else if (arg == "--log-level" && i + 1 < argc) {
            log_level = argv[++i];
        } else if (arg == "--threads" && i + 1 < argc) {
            threads = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--device-policy" && i + 1 < argc) {
            device_policy_str = argv[++i];
        } else if (arg == "--simulate-error" && i + 1 < argc) {
            simulate_error_code = argv[++i];
        }
    }

    dpnf::config::FrameworkConfig cfg{};
    cfg.application_name = "DPNF_Phase0_Sandbox";
    cfg.log_level = log_level;
    cfg.log_to_console = !json_output;
    cfg.worker_threads = threads;
    if (device_policy_str == "cpu_only") {
        cfg.device_policy = dpnf::config::DevicePolicy::CPUOnly;
    } else if (device_policy_str == "prefer_gpu") {
        cfg.device_policy = dpnf::config::DevicePolicy::PreferGPU;
    } else {
        cfg.device_policy = dpnf::config::DevicePolicy::Auto;
    }

    dpnf::core::Logger::instance().clear_entries();
    auto& core = dpnf::core::CoreSystem::instance();

    const dpnf::core::Error init_err = core.initialize(cfg);
    if (!init_err.is_ok()) {
        std::cerr << init_err.format() << "\n";
        return EXIT_FAILURE;
    }

    // Exercise ResourceTracker with deterministic scratch allocations
    auto& tracker = core.resource_tracker();
    auto alloc1 = tracker.record_allocation("phase0_scratch_buffer_a", "Sandbox", 65536);
    auto alloc2 = tracker.record_allocation("phase0_diagnostic_workspace", "Sandbox", 131072);

    // Exercise ThreadPool with parallel diagnostic tasks
    if (auto* pool = core.thread_pool()) {
        std::atomic<std::uint32_t> accumulator{0};
        std::vector<std::future<void>> futures;
        futures.reserve(8);
        for (std::uint32_t task_idx = 0; task_idx < 8; ++task_idx) {
            futures.push_back(pool->submit([&accumulator, task_idx]() {
                accumulator.fetch_add(task_idx + 1U, std::memory_order_relaxed);
            }));
        }
        for (auto& f : futures) {
            f.get();
        }
        pool->wait_idle();
        dpnf::core::Logger::instance().log(
            dpnf::core::LogLevel::Debug,
            "Sandbox",
            "ThreadPool executed 8 verification tasks; checksum=" + std::to_string(accumulator.load()));
    }

    if (alloc1.is_ok()) {
        (void)tracker.record_deallocation(alloc1.value());
    }
    if (alloc2.is_ok()) {
        (void)tracker.record_deallocation(alloc2.value());
    }

    if (!simulate_error_code.empty()) {
        const auto code = parse_error_code_name(simulate_error_code);
        if (code != dpnf::core::ErrorCode::DPNF_OK) {
            const auto simulated = DPNF_MAKE_ERROR(
                code,
                "Diagnostic verification of structured error code " + simulate_error_code,
                "SandboxVerifier");
            dpnf::core::Logger::instance().log(
                dpnf::core::LogLevel::Error,
                "SandboxVerifier",
                simulated.format());
        }
    }

    const auto report = core.generate_diagnostic_report();
    const dpnf::core::Error shutdown_err = core.shutdown();
    if (!shutdown_err.is_ok()) {
        std::cerr << shutdown_err.format() << "\n";
        return EXIT_FAILURE;
    }

    if (json_output) {
        std::cout << report.to_json();
    } else {
        std::cout << "\n" << report.to_human_readable() << "\n";
    }

    return EXIT_SUCCESS;
}
