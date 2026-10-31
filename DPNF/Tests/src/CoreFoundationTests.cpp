#include "dpnf/autodiff/Variable.h"
#include "dpnf/config/FrameworkConfig.h"
#include "dpnf/core/CoreSystem.h"
#include "dpnf/core/Error.h"
#include "dpnf/core/Logger.h"
#include "dpnf/core/ResourceTracker.h"
#include "dpnf/core/ThreadPool.h"
#include "dpnf/gpu/VulkanBackend.h"
#include "dpnf/graphics/NeuralGraphicsPipeline.h"
#include "dpnf/memory/MemoryPool.h"
#include "dpnf/neural/Module.h"
#include "dpnf/platform/PlatformInfo.h"
#include "dpnf/tensor/Tensor.h"

#include <gtest/gtest.h>
#include <atomic>
#include <cmath>
#include <string>
#include <vector>

namespace {

using dpnf::autodiff::Variable;
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
using dpnf::gpu::VulkanComputeEngine;
using dpnf::gpu::VulkanDispatchMetrics;
using dpnf::graphics::NeuralGraphicsFrame;
using dpnf::graphics::NeuralGraphicsPipeline;
using dpnf::memory::MemoryPool;
using dpnf::neural::run_supervised_training_demo;
using dpnf::platform::PlatformDetector;
using dpnf::tensor::Tensor;

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
    EXPECT_EQ(ok_res.value(), 1024);

    auto fail_res = Result<int>::failure(
        DPNF_MAKE_ERROR(ErrorCode::DPNF_ERROR_INVALID_TENSOR, "Shape mismatch [1,3,64,64]", "Tensor"));
    EXPECT_TRUE(fail_res.is_error());
    EXPECT_EQ(fail_res.error().code, ErrorCode::DPNF_ERROR_INVALID_TENSOR);
}

TEST(LoggerTest, LevelFilteringAndRingBufferCapture) {
    auto& logger = Logger::instance();
    logger.clear_entries();
    logger.configure(LogLevel::Warn, false, "", 16);

    logger.log(LogLevel::Debug, "Test", "Ignored debug message");
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

    std::string err_msg;
    EXPECT_TRUE(cfg.validate(err_msg));

    const std::string serialized = cfg.serialize_key_value();
    FrameworkConfig parsed{};
    EXPECT_TRUE(FrameworkConfig::parse_key_value(serialized, parsed, err_msg));
    EXPECT_EQ(parsed.application_name, "DPNF_Unit_Suite");
    EXPECT_EQ(parsed.worker_threads, 4U);
}

TEST(PlatformDetectorTest, EnumeratesHostAndVulkanCapabilities) {
    const auto report = PlatformDetector::detect();
    EXPECT_NE(report.os_name, "Unknown");
    EXPECT_GE(report.cpu.logical_cores, 1U);
    EXPECT_GE(report.cpp_standard, 202002L);
    EXPECT_TRUE(report.vulkan.loader_present);
    EXPECT_GE(report.vulkan.discovered_device_count, 1U);
}

TEST(MemoryPoolTest, ReusesAlignedBlocksOnDeallocation) {
    auto& pool = MemoryPool::instance();
    pool.release_cached_blocks();
    void* p1 = pool.allocate(4096);
    ASSERT_NE(p1, nullptr);
    pool.deallocate(p1, 4096);

    const auto hits_before = pool.stats().cache_hits;
    void* p2 = pool.allocate(4096);
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(pool.stats().cache_hits, hits_before + 1);
    pool.deallocate(p2, 4096);
}

TEST(TensorEngineTest, ArithmeticReshapeTransposeAndMatmul) {
    Tensor a({2, 3}, {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f});
    Tensor b({2, 3}, {10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f});

    auto sum_res = a.add(b);
    ASSERT_TRUE(sum_res.is_ok());
    EXPECT_FLOAT_EQ(sum_res.value().at({0, 0}), 11.0f);
    EXPECT_FLOAT_EQ(sum_res.value().at({1, 2}), 66.0f);

    auto bt = b.transpose_2d();
    ASSERT_TRUE(bt.is_ok());
    EXPECT_EQ(bt.value().shape()[0], 3U);
    EXPECT_EQ(bt.value().shape()[1], 2U);

    auto mm = a.matmul(bt.value());
    ASSERT_TRUE(mm.is_ok());
    EXPECT_EQ(mm.value().shape()[0], 2U);
    EXPECT_EQ(mm.value().shape()[1], 2U);
    EXPECT_FLOAT_EQ(mm.value().at({0, 0}), 140.0f);
}

TEST(VulkanComputeTest, ExecutesSpirvTensorAddAndFmaReluMatchingCpu) {
    VulkanComputeEngine vk;
    ASSERT_TRUE(vk.initialize().is_ok());
    EXPECT_TRUE(vk.is_ready());

    auto a = Tensor::arange(256, 0.5f, 0.25f);
    auto b = Tensor::arange(256, -1.0f, 0.5f);

    VulkanDispatchMetrics add_metrics{};
    auto add_out = vk.execute_tensor_add(a, b, 2.0f, 3.0f, &add_metrics);
    ASSERT_TRUE(add_out.is_ok());
    EXPECT_TRUE(add_metrics.validation_passed);
    EXPECT_LT(add_metrics.max_abs_diff_vs_cpu, 1e-5);

    VulkanDispatchMetrics fma_metrics{};
    auto fma_out = vk.execute_fma_leaky_relu(a, b, 0.25f, 0.1f, &fma_metrics);
    ASSERT_TRUE(fma_out.is_ok());
    EXPECT_TRUE(fma_metrics.validation_passed);
    EXPECT_LT(fma_metrics.max_abs_diff_vs_cpu, 1e-5);
}

TEST(AutoDiffTest, AnalyticalGradientsMatchNumericalFiniteDifferences) {
    Variable x(Tensor({2, 2}, {1.5f, -0.8f, 2.2f, 0.4f}), true);
    Variable w(Tensor({2, 2}, {0.7f, -1.2f, 0.5f, 0.9f}), false);
    Variable target(Tensor({2, 2}, {1.0f, 0.0f, 0.5f, -0.5f}), false);

    auto fn = [&](const Variable& in_var) {
        auto h = Variable::relu(Variable::matmul(in_var, w));
        return Variable::mse_loss(h, target);
    };

    const double max_err = Variable::max_numerical_gradient_error(fn, x, 1e-3f);
    EXPECT_LT(max_err, 2e-3);
}

TEST(NeuralCoreTrainingTest, TwoLayerMlpConvergesWithAdamW) {
    const auto summary = run_supervised_training_demo(25);
    EXPECT_TRUE(summary.converged);
    EXPECT_LT(summary.final_loss, summary.initial_loss * 0.1f);
}

TEST(NeuralGraphicsTest, SuperResolutionTemporalReconstructionAndSerialization) {
    NeuralGraphicsFrame frame{};
    frame.color = Tensor({3, 8, 8}, 0.5f);
    frame.depth = Tensor({1, 8, 8}, 0.25f);
    frame.motion_vectors = Tensor({2, 8, 8}, 0.0f);
    frame.exposure = 1.0f;

    ASSERT_TRUE(frame.validate().is_ok());

    auto sr = NeuralGraphicsPipeline::super_resolve_2x(frame.color);
    ASSERT_TRUE(sr.is_ok());
    EXPECT_EQ(sr.value().shape()[1], 16U);
    EXPECT_EQ(sr.value().shape()[2], 16U);

    auto denoised = NeuralGraphicsPipeline::denoise_gbuffer(frame);
    ASSERT_TRUE(denoised.is_ok());

    auto temp = NeuralGraphicsPipeline::temporal_reconstruct(frame, frame.color, frame.depth, 0.85f);
    ASSERT_TRUE(temp.is_ok());

    const auto qual = NeuralGraphicsPipeline::evaluate_quality(temp.value(), frame.color);
    EXPECT_GT(qual.psnr_db, 60.0);
    EXPECT_GT(qual.ssim, 0.99);

    const std::string model_path = "/tmp/test_model.dpnf";
    ASSERT_TRUE(NeuralGraphicsPipeline::save_dpnf_model(model_path, "DPSR_v1", sr.value()).is_ok());

    std::string loaded_name;
    auto loaded = NeuralGraphicsPipeline::load_dpnf_model(model_path, loaded_name);
    ASSERT_TRUE(loaded.is_ok());
    EXPECT_EQ(loaded_name, "DPSR_v1");
    EXPECT_EQ(loaded.value().shape(), sr.value().shape());
}

} // namespace
