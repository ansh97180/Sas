#include "dpnf/autodiff/Variable.h"
#include "dpnf/config/FrameworkConfig.h"
#include "dpnf/core/CoreSystem.h"
#include "dpnf/core/Error.h"
#include "dpnf/core/Logger.h"
#include "dpnf/gpu/VulkanBackend.h"
#include "dpnf/graphics/NeuralGraphicsPipeline.h"
#include "dpnf/neural/Module.h"
#include "dpnf/tensor/Tensor.h"

#include <atomic>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
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
    cfg.application_name = "DPNF_Vulkan_NeuralGraphics_Runtime";
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

    // 1. Execute Vulkan 1.3 SPIR-V Compute Shaders on GPU/Lavapipe
    if (cfg.device_policy != dpnf::config::DevicePolicy::CPUOnly) {
        dpnf::gpu::VulkanComputeEngine vk_engine;
        if (vk_engine.initialize().is_ok()) {
            auto a = dpnf::tensor::Tensor::arange(1024, 1.0f, 0.5f);
            auto b = dpnf::tensor::Tensor::arange(1024, 2.0f, 0.25f);
            dpnf::gpu::VulkanDispatchMetrics add_metrics{};
            auto gpu_res = vk_engine.execute_tensor_add(a, b, 1.5f, -0.5f, &add_metrics);
            if (gpu_res.is_ok()) {
                std::ostringstream msg;
                msg << "Dispatched SPIR-V kernel '" << add_metrics.kernel_name
                    << "' on '" << add_metrics.device_name << "' (" << add_metrics.element_count
                    << " floats, " << std::fixed << std::setprecision(2) << add_metrics.gpu_dispatch_ms
                    << " ms, max diff vs CPU = " << std::scientific << add_metrics.max_abs_diff_vs_cpu << ")";
                dpnf::core::Logger::instance().log(dpnf::core::LogLevel::Info, "VulkanCompute", msg.str());
            }

            dpnf::gpu::VulkanDispatchMetrics fma_metrics{};
            auto fma_res = vk_engine.execute_fma_leaky_relu(a, b, -10.0f, 0.05f, &fma_metrics);
            if (fma_res.is_ok()) {
                std::ostringstream msg;
                msg << "Dispatched SPIR-V kernel '" << fma_metrics.kernel_name
                    << "' (" << std::fixed << std::setprecision(2) << fma_metrics.gpu_dispatch_ms
                    << " ms, validation=" << (fma_metrics.validation_passed ? "PASSED" : "FAILED") << ")";
                dpnf::core::Logger::instance().log(dpnf::core::LogLevel::Info, "VulkanCompute", msg.str());
            }
            vk_engine.shutdown();
        }
    }

    // 2. Execute Autodiff + NeuralCore + AdamW Training Loop
    {
        const auto train_summary = dpnf::neural::run_supervised_training_demo(25);
        std::ostringstream msg;
        msg << "Trained 2-layer MLP with AdamW for " << train_summary.epochs
            << " epochs: initial_loss=" << std::fixed << std::setprecision(5) << train_summary.initial_loss
            << " -> final_loss=" << train_summary.final_loss
            << " (converged=" << (train_summary.converged ? "true" : "false") << ")";
        dpnf::core::Logger::instance().log(dpnf::core::LogLevel::Info, "NeuralTraining", msg.str());
    }

    // 3. Execute NeuralGraphics 2x Super Resolution, Temporal Reconstruction & Denoising
    {
        dpnf::graphics::NeuralGraphicsFrame frame{};
        frame.color = dpnf::tensor::Tensor({3, 16, 16}, 0.65f);
        frame.depth = dpnf::tensor::Tensor({1, 16, 16}, 0.4f);
        frame.motion_vectors = dpnf::tensor::Tensor({2, 16, 16}, 1.0f);
        frame.exposure = 1.0f;

        auto sr_out = dpnf::graphics::NeuralGraphicsPipeline::super_resolve_2x(frame.color);
        auto denoised = dpnf::graphics::NeuralGraphicsPipeline::denoise_gbuffer(frame);
        auto temporal = dpnf::graphics::NeuralGraphicsPipeline::temporal_reconstruct(
            frame, frame.color, frame.depth, 0.85f);

        if (sr_out.is_ok() && denoised.is_ok() && temporal.is_ok()) {
            const auto metrics = dpnf::graphics::NeuralGraphicsPipeline::evaluate_quality(
                temporal.value(), frame.color);
            std::ostringstream msg;
            msg << "Executed 2x SuperResolution (" << frame.color.shape_string() << " -> "
                << sr_out.value().shape_string() << "), TemporalReconstruct & GBufferDenoise (PSNR="
                << std::fixed << std::setprecision(1) << metrics.psnr_db << " dB, SSIM="
                << std::setprecision(4) << metrics.ssim << ")";
            dpnf::core::Logger::instance().log(dpnf::core::LogLevel::Info, "NeuralGraphics", msg.str());
        }
    }

    // 4. Exercise ThreadPool
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

    dpnf::memory::MemoryPool::instance().release_cached_blocks();
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
