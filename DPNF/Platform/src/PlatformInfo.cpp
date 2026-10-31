#include "dpnf/platform/PlatformInfo.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#if defined(__linux__)
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>
#elif defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

namespace dpnf::platform {

namespace {

std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

bool contains_flag(const std::string& flags_line, const std::string& token) {
    std::istringstream iss(flags_line);
    std::string item;
    while (iss >> item) {
        if (item == token) {
            return true;
        }
    }
    return false;
}

void detect_linux_cpu_info(CpuCapabilities& cpu) {
#if defined(__linux__)
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        return;
    }

    std::string line;
    std::uint32_t processor_count = 0;
    std::uint32_t core_count = 0;

    while (std::getline(cpuinfo, line)) {
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        const std::string key = trim(line.substr(0, colon));
        const std::string val = trim(line.substr(colon + 1));

        if (key == "processor") {
            ++processor_count;
        } else if (key == "vendor_id" && cpu.vendor_id == "Unknown") {
            cpu.vendor_id = val;
        } else if (key == "model name" && cpu.model_name == "Unknown") {
            cpu.model_name = val;
        } else if (key == "cpu cores" && core_count == 0) {
            try {
                core_count = static_cast<std::uint32_t>(std::stoul(val));
            } catch (...) {
                core_count = 0;
            }
        } else if (key == "flags") {
            cpu.sse4_2 = contains_flag(val, "sse4_2");
            cpu.avx = contains_flag(val, "avx");
            cpu.avx2 = contains_flag(val, "avx2");
            cpu.fma = contains_flag(val, "fma");
            cpu.f16c = contains_flag(val, "f16c");
            cpu.avx512f = contains_flag(val, "avx512f");
            cpu.avx512vl = contains_flag(val, "avx512vl");
            cpu.avx512bw = contains_flag(val, "avx512bw");
            cpu.avx512_vnni = contains_flag(val, "avx512_vnni");
            cpu.avx512_fp16 = contains_flag(val, "avx512_fp16");
            cpu.amx_bf16 = contains_flag(val, "amx_bf16");
            cpu.amx_int8 = contains_flag(val, "amx_int8");
        }
    }

    if (processor_count > 0) {
        cpu.logical_cores = processor_count;
    }
    if (core_count > 0) {
        cpu.physical_cores = core_count;
    } else {
        cpu.physical_cores = cpu.logical_cores;
    }
#else
    (void)cpu;
#endif
}

VulkanProbeStatus probe_vulkan_runtime() {
    VulkanProbeStatus status{};
    std::uint32_t instance_version = VK_API_VERSION_1_0;
    if (vkEnumerateInstanceVersion(&instance_version) == VK_SUCCESS) {
        status.loader_present = true;
        status.loader_library_path = "libvulkan.so.1";
        status.api_version_major = VK_API_VERSION_MAJOR(instance_version);
        status.api_version_minor = VK_API_VERSION_MINOR(instance_version);
        status.api_version_patch = VK_API_VERSION_PATCH(instance_version);
    }

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "DPNF_PlatformProbe";
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.pEngineName = "DPNF";
    app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    VkInstance instance = VK_NULL_HANDLE;
    const VkResult res = vkCreateInstance(&create_info, nullptr, &instance);
    if (res != VK_SUCCESS || instance == VK_NULL_HANDLE) {
        status.diagnostic_note = "Vulkan loader present, but vkCreateInstance returned " +
                                 std::to_string(static_cast<int>(res)) + "; CPU fallback active.";
        return status;
    }

    std::uint32_t dev_count = 0;
    if (vkEnumeratePhysicalDevices(instance, &dev_count, nullptr) == VK_SUCCESS && dev_count > 0) {
        status.discovered_device_count = dev_count;
        std::vector<VkPhysicalDevice> devices(dev_count);
        if (vkEnumeratePhysicalDevices(instance, &dev_count, devices.data()) == VK_SUCCESS) {
            for (const auto& phys : devices) {
                VkPhysicalDeviceProperties props{};
                vkGetPhysicalDeviceProperties(phys, &props);
                status.device_names.emplace_back(props.deviceName);
            }
        }
    }

    vkDestroyInstance(instance, nullptr);

    if (status.discovered_device_count > 0) {
        status.diagnostic_note = "Vulkan " + std::to_string(status.api_version_major) + "." +
                                 std::to_string(status.api_version_minor) + "." +
                                 std::to_string(status.api_version_patch) + " active (" +
                                 std::to_string(status.discovered_device_count) + " compute device: " +
                                 status.device_names.front() + ").";
    } else {
        status.diagnostic_note = "Vulkan instance created, 0 physical devices enumerated; CPU fallback active.";
    }
    return status;
}

} // namespace

PlatformReport PlatformDetector::detect() {
    PlatformReport report{};

#if defined(_WIN32)
    report.os = OperatingSystem::Windows;
    report.os_name = "Windows";
#elif defined(__linux__)
    report.os = OperatingSystem::Linux;
    report.os_name = "Linux";
    struct utsname uts {};
    if (uname(&uts) == 0) {
        report.kernel_version = std::string(uts.sysname) + " " + uts.release + " (" + uts.machine + ")";
    }
#elif defined(__APPLE__)
    report.os = OperatingSystem::MacOS;
    report.os_name = "macOS";
#else
    report.os = OperatingSystem::Unknown;
    report.os_name = "Unknown";
#endif

#if defined(__x86_64__) || defined(_M_X64)
    report.architecture = CpuArchitecture::X86_64;
#elif defined(__aarch64__) || defined(_M_ARM64)
    report.architecture = CpuArchitecture::ARM64;
#else
    report.architecture = CpuArchitecture::Unknown;
#endif

#if defined(__clang__)
    report.compiler_name = "Clang " + std::string(__clang_version__);
#elif defined(__GNUC__)
    report.compiler_name = "GCC " + std::to_string(__GNUC__) + "." +
                           std::to_string(__GNUC_MINOR__) + "." +
                           std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    report.compiler_name = "MSVC " + std::to_string(_MSC_VER);
#else
    report.compiler_name = "Unknown C++ Compiler";
#endif

    report.cpp_standard = static_cast<std::int64_t>(__cplusplus);

    const unsigned int hw_threads = std::thread::hardware_concurrency();
    report.cpu.logical_cores = hw_threads > 0U ? hw_threads : 1U;
    report.cpu.physical_cores = report.cpu.logical_cores;
    detect_linux_cpu_info(report.cpu);

#if defined(__linux__)
    struct sysinfo mem_info {};
    if (sysinfo(&mem_info) == 0) {
        const std::uint64_t unit = static_cast<std::uint64_t>(mem_info.mem_unit);
        report.memory.total_physical_bytes = static_cast<std::uint64_t>(mem_info.totalram) * unit;
        report.memory.available_physical_bytes = static_cast<std::uint64_t>(mem_info.freeram + mem_info.bufferram) * unit;
    }
    const long page_sz = sysconf(_SC_PAGESIZE);
    if (page_sz > 0) {
        report.memory.page_size_bytes = static_cast<std::uint64_t>(page_sz);
    }
#endif

    report.vulkan = probe_vulkan_runtime();
    return report;
}

std::string PlatformDetector::os_to_string(OperatingSystem os) {
    switch (os) {
        case OperatingSystem::Windows: return "Windows";
        case OperatingSystem::Linux: return "Linux";
        case OperatingSystem::MacOS: return "macOS";
        case OperatingSystem::Unknown: return "Unknown";
    }
    return "Unknown";
}

std::string PlatformDetector::arch_to_string(CpuArchitecture arch) {
    switch (arch) {
        case CpuArchitecture::X86_64: return "x86_64";
        case CpuArchitecture::ARM64: return "ARM64";
        case CpuArchitecture::Unknown: return "Unknown";
    }
    return "Unknown";
}

std::vector<std::string> PlatformDetector::supported_isa_list(const CpuCapabilities& cpu) {
    std::vector<std::string> list;
    if (cpu.sse4_2) list.emplace_back("SSE4.2");
    if (cpu.avx) list.emplace_back("AVX");
    if (cpu.avx2) list.emplace_back("AVX2");
    if (cpu.fma) list.emplace_back("FMA");
    if (cpu.f16c) list.emplace_back("F16C");
    if (cpu.avx512f) list.emplace_back("AVX512F");
    if (cpu.avx512vl) list.emplace_back("AVX512VL");
    if (cpu.avx512bw) list.emplace_back("AVX512BW");
    if (cpu.avx512_vnni) list.emplace_back("AVX512_VNNI");
    if (cpu.avx512_fp16) list.emplace_back("AVX512_FP16");
    if (cpu.amx_bf16) list.emplace_back("AMX_BF16");
    if (cpu.amx_int8) list.emplace_back("AMX_INT8");
    if (cpu.neon) list.emplace_back("NEON");
    return list;
}

} // namespace dpnf::platform
