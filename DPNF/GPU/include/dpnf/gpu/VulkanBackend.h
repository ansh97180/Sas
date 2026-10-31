#pragma once

#include "dpnf/core/Error.h"
#include "dpnf/tensor/Tensor.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

namespace dpnf::gpu {

struct VulkanDispatchMetrics {
    std::string device_name{};
    std::string kernel_name{};
    std::size_t element_count{0};
    std::uint32_t workgroup_count_x{0};
    double gpu_dispatch_ms{0.0};
    double max_abs_diff_vs_cpu{0.0};
    bool validation_passed{false};
};

class VulkanComputeEngine {
public:
    VulkanComputeEngine() = default;
    ~VulkanComputeEngine();

    VulkanComputeEngine(const VulkanComputeEngine&) = delete;
    VulkanComputeEngine& operator=(const VulkanComputeEngine&) = delete;

    core::Error initialize();
    void shutdown();

    [[nodiscard]] bool is_ready() const noexcept { return initialized_; }
    [[nodiscard]] const std::string& device_name() const noexcept { return device_name_; }
    [[nodiscard]] std::uint32_t api_version() const noexcept { return api_version_; }

    // Execute real Vulkan compute shader: out[i] = alpha * a[i] + beta * b[i]
    core::Result<tensor::Tensor> execute_tensor_add(
        const tensor::Tensor& a,
        const tensor::Tensor& b,
        float alpha = 1.0f,
        float beta = 1.0f,
        VulkanDispatchMetrics* out_metrics = nullptr);

    // Execute real Vulkan compute shader: y[i] = LeakyReLU(x[i] * w[i] + bias, leak)
    core::Result<tensor::Tensor> execute_fma_leaky_relu(
        const tensor::Tensor& x,
        const tensor::Tensor& w,
        float bias = 0.0f,
        float leak = 0.01f,
        VulkanDispatchMetrics* out_metrics = nullptr);

private:
    core::Error run_3buffer_compute_shader(
        const std::uint32_t* spv_code,
        std::size_t spv_byte_size,
        const float* in0,
        const float* in1,
        float* out,
        std::size_t count,
        float param0,
        float param1,
        double& out_elapsed_ms);

    std::uint32_t find_memory_type(std::uint32_t type_filter, VkMemoryPropertyFlags props) const;

    bool initialized_{false};
    std::string device_name_{"Uninitialized"};
    std::uint32_t api_version_{0};
    std::uint32_t compute_queue_family_{0};

    VkInstance instance_{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkQueue compute_queue_{VK_NULL_HANDLE};
    VkCommandPool command_pool_{VK_NULL_HANDLE};
};

} // namespace dpnf::gpu
