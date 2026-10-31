#include "dpnf/core/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace dpnf::core {

namespace {

std::string current_utc_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time_t_now = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()) %
                    1000;

    std::tm tm_utc{};
#if defined(_WIN32)
    gmtime_s(&tm_utc, &time_t_now);
#else
    gmtime_r(&time_t_now, &tm_utc);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count() << "Z";
    return oss.str();
}

} // namespace

Logger& Logger::instance() {
    static Logger logger_instance;
    return logger_instance;
}

void Logger::configure(
    LogLevel min_level,
    bool console_output,
    const std::string& file_path,
    std::size_t max_ring_entries) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = min_level;
    console_output_ = console_output;
    max_ring_entries_ = max_ring_entries > 0 ? max_ring_entries : 128;

    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    if (!file_path.empty()) {
        file_stream_.open(file_path, std::ios::out | std::ios::app);
    }
}

void Logger::set_level(LogLevel min_level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = min_level;
}

LogLevel Logger::level() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return min_level_;
}

void Logger::log(LogLevel lvl, const std::string& subsystem, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (lvl < min_level_ || min_level_ == LogLevel::Off) {
        return;
    }

    LogEntry entry{};
    entry.sequence_id = next_sequence_++;
    entry.timestamp_iso = current_utc_timestamp();
    entry.level = lvl;
    entry.subsystem = subsystem;
    entry.message = message;

    if (entries_.size() >= max_ring_entries_) {
        entries_.erase(entries_.begin());
    }
    entries_.push_back(entry);

    const std::string formatted =
        "[" + entry.timestamp_iso + "] [" + level_to_string(lvl) + "] [" + subsystem + "] " + message;

    if (console_output_) {
        if (lvl >= LogLevel::Error) {
            std::cerr << formatted << "\n";
        } else {
            std::cout << formatted << "\n";
        }
    }

    if (file_stream_.is_open()) {
        file_stream_ << formatted << "\n";
        file_stream_.flush();
    }
}

std::vector<LogEntry> Logger::recent_entries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
}

void Logger::clear_entries() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

std::string Logger::level_to_string(LogLevel lvl) {
    switch (lvl) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        case LogLevel::Off:   return "OFF";
    }
    return "INFO";
}

LogLevel Logger::string_to_level(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (lower == "trace") return LogLevel::Trace;
    if (lower == "debug") return LogLevel::Debug;
    if (lower == "info")  return LogLevel::Info;
    if (lower == "warn" || lower == "warning") return LogLevel::Warn;
    if (lower == "error") return LogLevel::Error;
    if (lower == "fatal") return LogLevel::Fatal;
    if (lower == "off")   return LogLevel::Off;
    return LogLevel::Info;
}

} // namespace dpnf::core
