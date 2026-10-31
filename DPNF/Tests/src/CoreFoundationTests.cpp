#include "dpnf/config/FrameworkConfig.hpp"
#include "dpnf/core/CoreSystem.hpp"
#include "dpnf/core/Error.hpp"
#include "dpnf/core/Logger.hpp"
#include "dpnf/core/ResourceTracker.hpp"
#include "dpnf/core/ThreadPool.hpp"
#include "dpnf/platform/PlatformInfo.hpp"

#include <gtest/gtest.h>
#include <atomic>
#include <string>
#include <vector>

namespace {

using dpnf::config::DevicePolicy;
using dpnf::config::FrameworkConfig;
using dpnf::core::CoreSystem;
using dpnf::core::Error;
using dpnf::core::ErrorCode;
using dpnf::core::Logger;
using dpnf::core::LogLevel;
using dpnf::core::ResourceTracker;
using dpnf::core::Result;
using dpnf::core::ThreadPool;
using dpnf::platform::PlatformDetector;

TEST(ErrorSystemTest, AllErrorCodesHaveCanonicalStringAndRemediation) {
    const std::vector<ErrorCode> all_codes = {
        ErrorCode::DPNF_OK,
        ErrorCode::DPNF_ERROR_INVALID_ARGUMENT,
        ErrorCode::DPNF_ERROR_OUT_OF_MEMORY,
        ErrorCode::DPNF_ERROR_DEVICE_NOT_FOUND,
        ErrorCode::DPNF_ERROR_DEVICE_LOST,
        ErrorCode::DPNF_ERROR_UNSUPPORTED_OPERATION,
        ErrorCode::DPNF_ERROR_INVALID_TENSOR,
        ErrorCode::DPNF_ERROR_INVALID_GRAPH,
        ErrorCode::DPNF_ERROR_COMPILATION_FAILED,
        ErrorCode::DPNF_ERROR_EXECUTION_FAILED,
        ErrorCode::DPNF_ERROR_DATASET_ERROR,
        ErrorCode::DPNF_ERROR_MODEL_LOAD_FAILED,
        ErrorCode::DPNF_ERROR_MODEL_SAVE_FAILED,
        ErrorCode::DPNF_ERROR_TRAINING_FAILED
    };

    for (const auto code : all_codes) {
        const std::string str = dpnf::core::error_code_to_string(code);
        EXPECT_FALSE(str.empty());
        EXPECT_NE(str, "DPNF_ERROR_UNKNOWN");

        const std::string hint = dpnf::core::default_remediation_for_code(code);
        EXPECT_FALSE(hint.empty());
    }
}

TEST(ErrorSystemTest, ResultTemplateHandlesSuccessAndFailure) {
    auto ok_res = Result<int>::success(1024);
    EXPECT_TRUE(ok_res.is_ok());
    EXPECT_FALSE(ok_res.is_error());
    EXPECT_EQ(ok_res.value(), 1024);

    auto fail_res = Result<int>::failure(
        DPNF_MAKE_ERROR(ErrorCode::DPNF_ERROR_INVALID_TENSOR, "Shape mismatch [1,3,64,64]", "Tensor"));
    EXPECT_FALSE(fail_res.is_ok());
    EXPECT_TRUE(fail_res.is_error());
    EXPECT_EQ(fail_res.error().code, ErrorCode::DPNF_ERROR_INVALID_TENSOR);
    EXPECT_NE(fail_res.error().format().find("DPNF_ERROR_INVALID_TENSOR"), std::string::npos);
}

TEST(LoggerTest, LevelFilteringAndRingBufferCapture) {
    auto& logger = Logger::instance();
    logger.clear_entries();
    logger.configure(LogLevel::Warn, false, "", 16);

    logger.log(LogLevel::Debug, "Test", "Ignored debug message");
    logger.log(LogLevel::Info, "Test", "Ignored info message");
    logger.log(LogLevel::Warn, "Test", "Captured warning message");
    logger.log(LogLevel::Error, "Test", "Captured error message");

    const auto entries = logger.recent_entries();
    ASSERT_EQ(entries.size(), 2U);
    EXPECT_EQ(entries[0].level, LogLevel::Warn);
    EXPECT_EQ(entries[1].level, LogLevel::Error);
}

TEST(ConfigTest, ValidationAndKeyValueRoundTrip) {
    FrameworkConfig cfg{};
    cfg.application_name = "DPNF_Unit_Suite";
    cfg.log_level = "debug";
    cfg.worker_threads = 4;
    cfg.random_seed = 1337ULL;
    cfg.device_policy = DevicePolicy::CPUOnly;
    cfg.custom_attributes["graphics_color_space"] = "linear";

    std::string err_msg;
    EXPECT_TRUE(cfg.validate(err_msg));
    EXPECT_TRUE(err_msg.empty());

    const std::string serialized = cfg.serialize_key_value();
    FrameworkConfig parsed{};
    EXPECT_TRUE(FrameworkConfig::parse_key_value(serialized, parsed, err_msg));
    EXPECT_EQ(parsed.application_name, "DPNF_Unit_Suite");
    EXPECT_EQ(parsed.log_level, "debug");
    EXPECT_EQ(parsed.worker_threads, 4U);
    EXPECT_EQ(parsed.random_seed, 1337ULL);
    EXPECT_EQ(parsed.device_policy, DevicePolicy::CPUOnly);
    EXPECT_EQ(parsed.custom_attributes["graphics_color_space"], "linear");
}

TEST(ConfigTest, RejectsInvalidParameters) {
    FrameworkConfig bad_cfg{};
    bad_cfg.application_name = "";
    std::string err_msg;
    EXPECT_FALSE(bad_cfg.validate(err_msg));

    bad_cfg.application_name = "ValidName";
    bad_cfg.log_level = "invalid_level";
    EXPECT_FALSE(bad_cfg.validate(err_msg));

    bad_cfg.log_level = "info";
    bad_cfg.worker_threads = 1024U;
    EXPECT_FALSE(bad_cfg.validate(err_msg));
}

TEST(PlatformDetectorTest, ReportsValidHostCapabilities) {
    const auto report = PlatformDetector::detect();
    EXPECT_NE(report.os_name, "Unknown");
    EXPECT_GE(report.cpu.logical_cores, 1U);
    EXPECT_GE(report.cpp_standard, 202002L);
    EXPECT_FALSE(report.vulkan.diagnostic_note.empty());
}

TEST(ResourceTrackerTest, TracksAllocationsPeakAndDetectsDoubleFree) {
    ResourceTracker tracker{};
    auto a1 = tracker.record_allocation("tensor_buf_0", "Memory", 4096, 16384);
    ASSERT_TRUE(a1.is_ok());

    auto a2 = tracker.record_allocation("tensor_buf_1", "Memory", 8192, 16384);
    ASSERT_TRUE(a2.is_ok());

    // Exceeds 16384 budget
    auto a3 = tracker.record_allocation("tensor_buf_overflow", "Memory", 8192, 16384);
    EXPECT_TRUE(a3.is_error());
    EXPECT_EQ(a3.error().code, ErrorCode::DPNF_ERROR_OUT_OF_MEMORY);

    auto snap1 = tracker.snapshot();
    EXPECT_EQ(snap1.current_used_bytes, 12288ULL);
    EXPECT_EQ(snap1.peak_used_bytes, 12288ULL);
    EXPECT_EQ(snap1.active_allocation_count, 2ULL);

    EXPECT_TRUE(tracker.record_deallocation(a1.value()).is_ok());
    // Double free must fail with DPNF_ERROR_INVALID_ARGUMENT
    const auto double_free = tracker.record_deallocation(a1.value());
    EXPECT_FALSE(double_free.is_ok());
    EXPECT_EQ(double_free.code, ErrorCode::DPNF_ERROR_INVALID_ARGUMENT);

    EXPECT_TRUE(tracker.record_deallocation(a2.value()).is_ok());
    auto snap2 = tracker.snapshot();
    EXPECT_EQ(snap2.current_used_bytes, 0ULL);
    EXPECT_EQ(snap2.peak_used_bytes, 12288ULL);
    EXPECT_EQ(snap2.active_allocation_count, 0ULL);
}

TEST(ThreadPoolTest, ExecutesParallelTasksDeterministically) {
    ThreadPool pool(4);
    EXPECT_EQ(pool.worker_count(), 4U);

    std::atomic<int> sum{0};
    std::vector<std::future<void>> futures;
    for (int i = 1; i <= 32; ++i) {
        futures.push_back(pool.submit([&sum, i]() {
            sum.fetch_add(i, std::memory_order_relaxed);
        }));
    }
    for (auto& f : futures) {
        f.get();
    }
    pool.wait_idle();

    EXPECT_EQ(sum.load(), (32 * 33) / 2);
    EXPECT_EQ(pool.completed_task_count(), 32ULL);
}

TEST(CoreSystemTest, FullLifecycleAndDiagnosticJsonGeneration) {
    auto& core = CoreSystem::instance();
    if (core.is_initialized()) {
        (void)core.shutdown();
    }

    FrameworkConfig cfg{};
    cfg.application_name = "DPNF_Lifecycle_Test";
    cfg.log_level = "info";
    cfg.log_to_console = false;
    cfg.worker_threads = 2;

    const Error init_err = core.initialize(cfg);
    ASSERT_TRUE(init_err.is_ok());
    EXPECT_TRUE(core.is_initialized());

    // Duplicate initialization must be rejected
    const Error dup_err = core.initialize(cfg);
    EXPECT_FALSE(dup_err.is_ok());
    EXPECT_EQ(dup_err.code, ErrorCode::DPNF_ERROR_INVALID_ARGUMENT);

    const auto report = core.generate_diagnostic_report();
    EXPECT_TRUE(report.initialized);
    EXPECT_EQ(report.worker_thread_count, 2U);
    const std::string json = report.to_json();
    EXPECT_NE(json.find("\"framework_version\": \"0.1.0-phase0\""), std::string::npos);

    const Error shut_err = core.shutdown();
    ASSERT_TRUE(shut_err.is_ok());
    EXPECT_FALSE(core.is_initialized());
}

} // namespace
