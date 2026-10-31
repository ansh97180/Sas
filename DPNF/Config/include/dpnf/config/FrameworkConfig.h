#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace dpnf::config {

enum class DevicePolicy {
    Auto,
    CPUOnly,
    PreferGPU
};

struct FrameworkConfig {
    std::string application_name{"DPNFApplication"};
    std::string log_level{"info"};
    std::string log_file_path{};
    bool log_to_console{true};
    std::uint32_t worker_threads{0};
    std::uint64_t max_cpu_memory_bytes{0};
    std::uint64_t random_seed{42ULL};
    bool deterministic_execution{true};
    bool enable_resource_tracking{true};
    bool strict_bounds_checking{true};
    DevicePolicy device_policy{DevicePolicy::Auto};
    std::map<std::string, std::string> custom_attributes{};

    bool validate(std::string& error_message) const;
    std::string serialize_key_value() const;
    static bool parse_key_value(const std::string& content, FrameworkConfig& out_config, std::string& error_message);
    static std::string device_policy_to_string(DevicePolicy policy);
};

} // namespace dpnf::config
