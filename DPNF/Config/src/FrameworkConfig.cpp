#include "dpnf/config/FrameworkConfig.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace dpnf::config {

namespace {

std::string trim_copy(const std::string& str) {
    const auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string to_lower_copy(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}

} // namespace

bool FrameworkConfig::validate(std::string& error_message) const {
    if (application_name.empty()) {
        error_message = "application_name must not be empty.";
        return false;
    }

    const std::string lvl = to_lower_copy(log_level);
    if (lvl != "trace" && lvl != "debug" && lvl != "info" &&
        lvl != "warn" && lvl != "error" && lvl != "fatal" && lvl != "off") {
        error_message = "Invalid log_level '" + log_level +
                        "'. Expected one of: trace, debug, info, warn, error, fatal, off.";
        return false;
    }

    if (worker_threads > 256U) {
        error_message = "worker_threads (" + std::to_string(worker_threads) +
                        ") exceeds maximum supported Phase 0 pool size (256).";
        return false;
    }

    error_message.clear();
    return true;
}

std::string FrameworkConfig::serialize_key_value() const {
    std::ostringstream oss;
    oss << "application_name=" << application_name << "\n";
    oss << "log_level=" << log_level << "\n";
    oss << "log_file_path=" << log_file_path << "\n";
    oss << "log_to_console=" << (log_to_console ? "true" : "false") << "\n";
    oss << "worker_threads=" << worker_threads << "\n";
    oss << "max_cpu_memory_bytes=" << max_cpu_memory_bytes << "\n";
    oss << "random_seed=" << random_seed << "\n";
    oss << "deterministic_execution=" << (deterministic_execution ? "true" : "false") << "\n";
    oss << "enable_resource_tracking=" << (enable_resource_tracking ? "true" : "false") << "\n";
    oss << "strict_bounds_checking=" << (strict_bounds_checking ? "true" : "false") << "\n";
    oss << "device_policy=" << device_policy_to_string(device_policy) << "\n";
    for (const auto& [k, v] : custom_attributes) {
        oss << "custom." << k << "=" << v << "\n";
    }
    return oss.str();
}

bool FrameworkConfig::parse_key_value(
    const std::string& content,
    FrameworkConfig& out_config,
    std::string& error_message) {
    FrameworkConfig parsed = out_config;
    std::istringstream iss(content);
    std::string line;
    std::size_t line_num = 0;

    while (std::getline(iss, line)) {
        ++line_num;
        const std::string trimmed = trim_copy(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }
        const auto eq = trimmed.find('=');
        if (eq == std::string::npos) {
            error_message = "Syntax error at line " + std::to_string(line_num) + ": missing '=' delimiter.";
            return false;
        }

        const std::string key = trim_copy(trimmed.substr(0, eq));
        const std::string val = trim_copy(trimmed.substr(eq + 1));

        try {
            if (key == "application_name") {
                parsed.application_name = val;
            } else if (key == "log_level") {
                parsed.log_level = val;
            } else if (key == "log_file_path") {
                parsed.log_file_path = val;
            } else if (key == "log_to_console") {
                parsed.log_to_console = (to_lower_copy(val) == "true" || val == "1");
            } else if (key == "worker_threads") {
                parsed.worker_threads = static_cast<std::uint32_t>(std::stoul(val));
            } else if (key == "max_cpu_memory_bytes") {
                parsed.max_cpu_memory_bytes = static_cast<std::uint64_t>(std::stoull(val));
            } else if (key == "random_seed") {
                parsed.random_seed = static_cast<std::uint64_t>(std::stoull(val));
            } else if (key == "deterministic_execution") {
                parsed.deterministic_execution = (to_lower_copy(val) == "true" || val == "1");
            } else if (key == "enable_resource_tracking") {
                parsed.enable_resource_tracking = (to_lower_copy(val) == "true" || val == "1");
            } else if (key == "strict_bounds_checking") {
                parsed.strict_bounds_checking = (to_lower_copy(val) == "true" || val == "1");
            } else if (key == "device_policy") {
                const std::string pol = to_lower_copy(val);
                if (pol == "auto") {
                    parsed.device_policy = DevicePolicy::Auto;
                } else if (pol == "cpu_only" || pol == "cpuonly" || pol == "cpu") {
                    parsed.device_policy = DevicePolicy::CPUOnly;
                } else if (pol == "prefer_gpu" || pol == "prefergpu" || pol == "gpu") {
                    parsed.device_policy = DevicePolicy::PreferGPU;
                } else {
                    error_message = "Unknown device_policy '" + val + "' at line " + std::to_string(line_num);
                    return false;
                }
            } else if (key.rfind("custom.", 0) == 0) {
                parsed.custom_attributes[key.substr(7)] = val;
            } else {
                error_message = "Unknown configuration key '" + key + "' at line " + std::to_string(line_num);
                return false;
            }
        } catch (const std::exception& ex) {
            error_message = "Invalid numeric value for key '" + key + "' at line " +
                            std::to_string(line_num) + ": " + ex.what();
            return false;
        }
    }

    if (!parsed.validate(error_message)) {
        return false;
    }

    out_config = std::move(parsed);
    return true;
}

std::string FrameworkConfig::device_policy_to_string(DevicePolicy policy) {
    switch (policy) {
        case DevicePolicy::Auto: return "auto";
        case DevicePolicy::CPUOnly: return "cpu_only";
        case DevicePolicy::PreferGPU: return "prefer_gpu";
    }
    return "auto";
}

} // namespace dpnf::config
