#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dpnf::platform {

enum class OperatingSystem {
    Windows,
    Linux,
    MacOS,
    Unknown
};

enum class CpuArchitecture {
    X86_64,
    ARM64,
    Unknown
};

struct CpuCapabilities {
    std::string vendor_id{"Unknown"};
    std::string model_name{"Unknown"};
    std::uint32_t logical_cores{1};
    std::uint32_t physical_cores{1};
    bool sse4_2{false};
    bool avx{false};
    bool avx2{false};
    bool fma{false};
    bool f16c{false};
    bool avx512f{false};
    bool avx512vl{false};
    bool avx512bw{false};
    bool avx512_vnni{false};
    bool avx512_fp16{false};
    bool amx_bf16{false};
    bool amx_int8{false};
    bool neon{false};
};

struct MemoryCapabilities {
    std::uint64_t total_physical_bytes{0};
    std::uint64_t available_physical_bytes{0};
    std::uint64_t page_size_bytes{4096};
};

struct VulkanProbeStatus {
    bool loader_present{false};
    std::string loader_library_path{};
    std::uint32_t api_version_major{0};
    std::uint32_t api_version_minor{0};
    std::uint32_t api_version_patch{0};
    std::uint32_t discovered_device_count{0};
    std::vector<std::string> device_names{};
    std::string diagnostic_note{};
};

struct PlatformReport {
    OperatingSystem os{OperatingSystem::Unknown};
    std::string os_name{"Unknown"};
    std::string kernel_version{"Unknown"};
    CpuArchitecture architecture{CpuArchitecture::Unknown};
    std::string compiler_name{"Unknown"};
    std::int64_t cpp_standard{0};
    CpuCapabilities cpu{};
    MemoryCapabilities memory{};
    VulkanProbeStatus vulkan{};
};

class PlatformDetector {
public:
    static PlatformReport detect();
    static std::string os_to_string(OperatingSystem os);
    static std::string arch_to_string(CpuArchitecture arch);
    static std::vector<std::string> supported_isa_list(const CpuCapabilities& cpu);
};

} // namespace dpnf::platform
