#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace dpnf::core {

enum class LogLevel : std::int32_t {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Fatal = 5,
    Off = 6
};

struct LogEntry {
    std::uint64_t sequence_id{0};
    std::string timestamp_iso{};
    LogLevel level{LogLevel::Info};
    std::string subsystem{};
    std::string message{};
};

class Logger {
public:
    static Logger& instance();

    void configure(LogLevel min_level, bool console_output, const std::string& file_path = "", std::size_t max_ring_entries = 512);
    void set_level(LogLevel min_level);
    [[nodiscard]] LogLevel level() const;

    void log(LogLevel lvl, const std::string& subsystem, const std::string& message);

    [[nodiscard]] std::vector<LogEntry> recent_entries() const;
    void clear_entries();

    static std::string level_to_string(LogLevel lvl);
    static LogLevel string_to_level(const std::string& str);

private:
    Logger() = default;

    mutable std::mutex mutex_{};
    LogLevel min_level_{LogLevel::Info};
    bool console_output_{true};
    std::ofstream file_stream_{};
    std::size_t max_ring_entries_{512};
    std::uint64_t next_sequence_{1};
    std::vector<LogEntry> entries_{};
};

} // namespace dpnf::core
