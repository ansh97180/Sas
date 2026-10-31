#include "dpnf/gpu/VulkanBackend.h"
#include "dpnf/core/Logger.h"

#include <cstdint>
#include "dpnf/gpu/ShaderTensorAddSpv.h"
#include "dpnf/gpu/ShaderFmaReluSpv.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <vector>

namespace dpnf::gpu {

namespace {

struct PushConstantsPayload {
    std::uint32_t count;
    float param0;
    float param1;
};

} // namespace

VulkanComputeEngine::~VulkanComputeEngine() {
    shutdown();
}

core::Error VulkanComputeEngine::initialize() {
    if (initialized_) {
        return core::Error::ok();
    }

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "DPNF_VulkanComputeEngine";
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.pEngineName = "DPNF";
    app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo inst_info{};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pApplicationInfo = &app_info;

    if (vkCreateInstance(&inst_info, nullptr, &instance_) != VK_SUCCESS) {
        return DPNF_MAKE_ERROR(
            core::ErrorCode::DPNF_ERROR_DEVICE_NOT_FOUND,
            "vkCreateInstance failed when initializing headless Vulkan compute backend.",
            "VulkanComputeEngine");
    }

    std::uint32_t dev_count = 0;
    vkEnumeratePhysicalDevices(instance_, &dev_count, nullptr);
    if (dev_count == 0) {
        shutdown();
        return DPNF_MAKE_ERROR(
            core::ErrorCode::DPNF_ERROR_DEVICE_NOT_FOUND,
            "Zero Vulkan physical devices enumerated.",
            "VulkanComputeEngine");
    }

    std::vector<VkPhysicalDevice> devices(dev_count);
    vkEnumeratePhysicalDevices(instance_, &dev_count, devices.data());
    physical_device_ = devices.front();

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physical_device_, &props);
    device_name_ = props.deviceName;
    api_version_ = props.apiVersion;

    std::uint32_t qf_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &qf_count, nullptr);
    std::vector<VkQueueFamilyProperties> qf_props(qf_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &qf_count, qf_props.data());

    bool found_compute = false;
    for (std::uint32_t i = 0; i < qf_count; ++i) {
        if ((qf_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
            compute_queue_family_ = i;
            found_compute = true;
            break;
        }
    }

    if (!found_compute) {
        shutdown();
        return DPNF_MAKE_ERROR(
            core::ErrorCode::DPNF_ERROR_UNSUPPORTED_OPERATION,
            "Selected Vulkan physical device does not expose a VK_QUEUE_COMPUTE_BIT queue family.",
            "VulkanComputeEngine");
    }

    const float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = compute_queue_family_;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo dev_info{};
    dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_info.queueCreateInfoCount = 1;
    dev_info.pQueueCreateInfos = &queue_info;

    if (vkCreateDevice(physical_device_, &dev_info, nullptr, &device_) != VK_SUCCESS) {
        shutdown();
        return DPNF_MAKE_ERROR(
            core::ErrorCode::DPNF_ERROR_DEVICE_NOT_FOUND,
            "vkCreateDevice failed for Vulkan compute queue.",
            "VulkanComputeEngine");
    }

    vkGetDeviceQueue(device_, compute_queue_family_, 0, &compute_queue_);

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = compute_queue_family_;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
        shutdown();
        return DPNF_MAKE_ERROR(
            core::ErrorCode::DPNF_ERROR_EXECUTION_FAILED,
            "vkCreateCommandPool failed.",
            "VulkanComputeEngine");
    }

    initialized_ = true;
    core::Logger::instance().log(
        core::LogLevel::Info,
        "VulkanCompute",
        "Initialized Vulkan compute device '" + device_name_ + "' (Compute Queue Family " +
            std::to_string(compute_queue_family_) + ").");
    return core::Error::ok();
}

void VulkanComputeEngine::shutdown() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        if (command_pool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, command_pool_, nullptr);
            command_pool_ = VK_NULL_HANDLE;
        }
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
    initialized_ = false;
}

std::uint32_t VulkanComputeEngine::find_memory_type(
    std::uint32_t type_filter,
    VkMemoryPropertyFlags props) const {
    VkPhysicalDeviceMemoryProperties mem_props{};
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_props);
    for (std::uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((type_filter & (1U << i)) != 0U &&
            (mem_props.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    return 0;
}

core::Error VulkanComputeEngine::run_3buffer_compute_shader(
    const std::uint32_t* spv_code,
    std::size_t spv_byte_size,
    const float* in0,
    const float* in1,
    float* out,
    std::size_t count,
    float param0,
    float param1,
    double& out_elapsed_ms) {
    if (!initialized_) {
        const auto err = initialize();
        if (!err.is_ok()) {
            return err;
        }
    }

    const VkDeviceSize buffer_bytes = static_cast<VkDeviceSize>(count * sizeof(float));
    VkBuffer buffers[3] = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceMemory memories[3] = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};

    auto cleanup_buffers = [&]() {
        for (int i = 0; i < 3; ++i) {
            if (buffers[i] != VK_NULL_HANDLE) vkDestroyBuffer(device_, buffers[i], nullptr);
            if (memories[i] != VK_NULL_HANDLE) vkFreeMemory(device_, memories[i], nullptr);
        }
    };

    for (int i = 0; i < 3; ++i) {
        VkBufferCreateInfo buf_info{};
        buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_info.size = buffer_bytes;
        buf_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device_, &buf_info, nullptr, &buffers[i]) != VK_SUCCESS) {
            cleanup_buffers();
            return DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_OUT_OF_MEMORY,
                "vkCreateBuffer failed for storage buffer.",
                "VulkanComputeEngine");
        }

        VkMemoryRequirements mem_req{};
        vkGetBufferMemoryRequirements(device_, buffers[i], &mem_req);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_req.size;
        alloc_info.memoryTypeIndex = find_memory_type(
            mem_req.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device_, &alloc_info, nullptr, &memories[i]) != VK_SUCCESS) {
            cleanup_buffers();
            return DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_OUT_OF_MEMORY,
                "vkAllocateMemory failed for storage buffer.",
                "VulkanComputeEngine");
        }

        vkBindBufferMemory(device_, buffers[i], memories[i], 0);
    }

    // Upload inputs
    void* mapped = nullptr;
    vkMapMemory(device_, memories[0], 0, buffer_bytes, 0, &mapped);
    std::memcpy(mapped, in0, static_cast<std::size_t>(buffer_bytes));
    vkUnmapMemory(device_, memories[0]);

    vkMapMemory(device_, memories[1], 0, buffer_bytes, 0, &mapped);
    std::memcpy(mapped, in1, static_cast<std::size_t>(buffer_bytes));
    vkUnmapMemory(device_, memories[1]);

    // Descriptor Set Layout (3 storage buffers)
    VkDescriptorSetLayoutBinding bindings[3]{};
    for (std::uint32_t b = 0; b < 3; ++b) {
        bindings[b].binding = b;
        bindings[b].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[b].descriptorCount = 1;
        bindings[b].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo dsl_info{};
    dsl_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_info.bindingCount = 3;
    dsl_info.pBindings = bindings;

    VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
    vkCreateDescriptorSetLayout(device_, &dsl_info, nullptr, &dsl);

    VkPushConstantRange pc_range{};
    pc_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pc_range.offset = 0;
    pc_range.size = sizeof(PushConstantsPayload);

    VkPipelineLayoutCreateInfo pl_info{};
    pl_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_info.setLayoutCount = 1;
    pl_info.pSetLayouts = &dsl;
    pl_info.pushConstantRangeCount = 1;
    pl_info.pPushConstantRanges = &pc_range;

    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    vkCreatePipelineLayout(device_, &pl_info, nullptr, &pipeline_layout);

    VkShaderModuleCreateInfo sm_info{};
    sm_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sm_info.codeSize = spv_byte_size;
    sm_info.pCode = spv_code;

    VkShaderModule shader_module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device_, &sm_info, nullptr, &shader_module) != VK_SUCCESS) {
        vkDestroyPipelineLayout(device_, pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(device_, dsl, nullptr);
        cleanup_buffers();
        return DPNF_MAKE_ERROR(
            core::ErrorCode::DPNF_ERROR_COMPILATION_FAILED,
            "vkCreateShaderModule failed on SPIR-V compute shader.",
            "VulkanComputeEngine");
    }

    VkComputePipelineCreateInfo cp_info{};
    cp_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cp_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    cp_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    cp_info.stage.module = shader_module;
    cp_info.stage.pName = "main";
    cp_info.layout = pipeline_layout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &cp_info, nullptr, &pipeline);

    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_size.descriptorCount = 3;

    VkDescriptorPoolCreateInfo dp_info{};
    dp_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dp_info.maxSets = 1;
    dp_info.poolSizeCount = 1;
    dp_info.pPoolSizes = &pool_size;

    VkDescriptorPool desc_pool = VK_NULL_HANDLE;
    vkCreateDescriptorPool(device_, &dp_info, nullptr, &desc_pool);

    VkDescriptorSetAllocateInfo ds_alloc{};
    ds_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ds_alloc.descriptorPool = desc_pool;
    ds_alloc.descriptorSetCount = 1;
    ds_alloc.pSetLayouts = &dsl;

    VkDescriptorSet desc_set = VK_NULL_HANDLE;
    vkAllocateDescriptorSets(device_, &ds_alloc, &desc_set);

    VkDescriptorBufferInfo db_infos[3]{};
    VkWriteDescriptorSet writes[3]{};
    for (std::uint32_t i = 0; i < 3; ++i) {
        db_infos[i].buffer = buffers[i];
        db_infos[i].offset = 0;
        db_infos[i].range = buffer_bytes;

        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = desc_set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &db_infos[i];
    }
    vkUpdateDescriptorSets(device_, 3, writes, 0, nullptr);

    VkCommandBufferAllocateInfo cb_alloc{};
    cb_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cb_alloc.commandPool = command_pool_;
    cb_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cb_alloc.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device_, &cb_alloc, &cmd);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    const auto t0 = std::chrono::high_resolution_clock::now();

    vkBeginCommandBuffer(cmd, &begin_info);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &desc_set, 0, nullptr);

    const PushConstantsPayload pc{static_cast<std::uint32_t>(count), param0, param1};
    vkCmdPushConstants(cmd, pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstantsPayload), &pc);

    const std::uint32_t group_count_x = static_cast<std::uint32_t>((count + 63U) / 64U);
    vkCmdDispatch(cmd, group_count_x, 1, 1);
    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(device_, &fence_info, nullptr, &fence);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    vkQueueSubmit(compute_queue_, 1, &submit_info, fence);
    vkWaitForFences(device_, 1, &fence, VK_TRUE, 5000000000ULL);

    const auto t1 = std::chrono::high_resolution_clock::now();
    out_elapsed_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    vkMapMemory(device_, memories[2], 0, buffer_bytes, 0, &mapped);
    std::memcpy(out, mapped, static_cast<std::size_t>(buffer_bytes));
    vkUnmapMemory(device_, memories[2]);

    vkDestroyFence(device_, fence, nullptr);
    vkFreeCommandBuffers(device_, command_pool_, 1, &cmd);
    vkDestroyDescriptorPool(device_, desc_pool, nullptr);
    vkDestroyPipeline(device_, pipeline, nullptr);
    vkDestroyShaderModule(device_, shader_module, nullptr);
    vkDestroyPipelineLayout(device_, pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(device_, dsl, nullptr);
    cleanup_buffers();

    return core::Error::ok();
}

core::Result<tensor::Tensor> VulkanComputeEngine::execute_tensor_add(
    const tensor::Tensor& a,
    const tensor::Tensor& b,
    float alpha,
    float beta,
    VulkanDispatchMetrics* out_metrics) {
    if (a.numel() != b.numel() || a.numel() == 0) {
        return core::Result<tensor::Tensor>::failure(
            DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
                "Vulkan execute_tensor_add requires non-empty matching tensors.",
                "VulkanComputeEngine"));
    }

    tensor::Tensor result(a.shape(), 0.0f);
    double elapsed_ms = 0.0;
    const auto err = run_3buffer_compute_shader(
        kShaderTensorAddSpv,
        sizeof(kShaderTensorAddSpv),
        a.data(),
        b.data(),
        result.data(),
        a.numel(),
        alpha,
        beta,
        elapsed_ms);

    if (!err.is_ok()) {
        return core::Result<tensor::Tensor>::failure(err);
    }

    // Validate against CPU reference
    double max_diff = 0.0;
    for (std::size_t i = 0; i < a.numel(); ++i) {
        const float cpu_ref = alpha * a.data()[i] + beta * b.data()[i];
        const double diff = std::fabs(static_cast<double>(result.data()[i] - cpu_ref));
        if (diff > max_diff) {
            max_diff = diff;
        }
    }

    if (out_metrics != nullptr) {
        out_metrics->device_name = device_name_;
        out_metrics->kernel_name = "tensor_add.comp (SPIR-V 1.6)";
        out_metrics->element_count = a.numel();
        out_metrics->workgroup_count_x = static_cast<std::uint32_t>((a.numel() + 63U) / 64U);
        out_metrics->gpu_dispatch_ms = elapsed_ms;
        out_metrics->max_abs_diff_vs_cpu = max_diff;
        out_metrics->validation_passed = (max_diff < 1e-5);
    }

    return core::Result<tensor::Tensor>::success(std::move(result));
}

core::Result<tensor::Tensor> VulkanComputeEngine::execute_fma_leaky_relu(
    const tensor::Tensor& x,
    const tensor::Tensor& w,
    float bias,
    float leak,
    VulkanDispatchMetrics* out_metrics) {
    if (x.numel() != w.numel() || x.numel() == 0) {
        return core::Result<tensor::Tensor>::failure(
            DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
                "Vulkan execute_fma_leaky_relu requires non-empty matching tensors.",
                "VulkanComputeEngine"));
    }

    tensor::Tensor result(x.shape(), 0.0f);
    double elapsed_ms = 0.0;
    const auto err = run_3buffer_compute_shader(
        kShaderFmaReluSpv,
        sizeof(kShaderFmaReluSpv),
        x.data(),
        w.data(),
        result.data(),
        x.numel(),
        bias,
        leak,
        elapsed_ms);

    if (!err.is_ok()) {
        return core::Result<tensor::Tensor>::failure(err);
    }

    double max_diff = 0.0;
    for (std::size_t i = 0; i < x.numel(); ++i) {
        const float v = x.data()[i] * w.data()[i] + bias;
        const float cpu_ref = v >= 0.0f ? v : v * leak;
        const double diff = std::fabs(static_cast<double>(result.data()[i] - cpu_ref));
        if (diff > max_diff) {
            max_diff = diff;
        }
    }

    if (out_metrics != nullptr) {
        out_metrics->device_name = device_name_;
        out_metrics->kernel_name = "fma_relu.comp (SPIR-V 1.6)";
        out_metrics->element_count = x.numel();
        out_metrics->workgroup_count_x = static_cast<std::uint32_t>((x.numel() + 63U) / 64U);
        out_metrics->gpu_dispatch_ms = elapsed_ms;
        out_metrics->max_abs_diff_vs_cpu = max_diff;
        out_metrics->validation_passed = (max_diff < 1e-5);
    }

    return core::Result<tensor::Tensor>::success(std::move(result));
}

} // namespace dpnf::gpu
