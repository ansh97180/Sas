#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace dpnf::core {

enum class ErrorCode : std::int32_t {
    DPNF_OK = 0,
    DPNF_ERROR_INVALID_ARGUMENT = 1,
    DPNF_ERROR_OUT_OF_MEMORY = 2,
    DPNF_ERROR_DEVICE_NOT_FOUND = 3,
    DPNF_ERROR_DEVICE_LOST = 4,
    DPNF_ERROR_UNSUPPORTED_OPERATION = 5,
    DPNF_ERROR_INVALID_TENSOR = 6,
    DPNF_ERROR_INVALID_GRAPH = 7,
    DPNF_ERROR_COMPILATION_FAILED = 8,
    DPNF_ERROR_EXECUTION_FAILED = 9,
    DPNF_ERROR_DATASET_ERROR = 10,
    DPNF_ERROR_MODEL_LOAD_FAILED = 11,
    DPNF_ERROR_MODEL_SAVE_FAILED = 12,
    DPNF_ERROR_TRAINING_FAILED = 13
};

struct Error {
    ErrorCode code{ErrorCode::DPNF_OK};
    std::string message{};
    std::string subsystem{"Core"};
    std::string file{};
    std::int32_t line{0};
    std::string remediation_hint{};

    [[nodiscard]] bool is_ok() const noexcept {
        return code == ErrorCode::DPNF_OK;
    }

    [[nodiscard]] std::string code_name() const;
    [[nodiscard]] std::string format() const;

    static Error ok();
    static Error make(
        ErrorCode code,
        std::string message,
        std::string subsystem = "Core",
        std::string file = "",
        std::int32_t line = 0,
        std::string remediation_hint = "");
};

std::string error_code_to_string(ErrorCode code);
std::string default_remediation_for_code(ErrorCode code);

template <typename T>
class Result {
public:
    static Result<T> success(T value) {
        return Result<T>(std::move(value));
    }

    static Result<T> failure(Error err) {
        return Result<T>(std::move(err));
    }

    [[nodiscard]] bool is_ok() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    [[nodiscard]] bool is_error() const noexcept {
        return std::holds_alternative<Error>(storage_);
    }

    [[nodiscard]] const T& value() const& {
        return std::get<T>(storage_);
    }

    [[nodiscard]] T& value() & {
        return std::get<T>(storage_);
    }

    [[nodiscard]] T&& value() && {
        return std::get<T>(std::move(storage_));
    }

    [[nodiscard]] const Error& error() const& {
        return std::get<Error>(storage_);
    }

private:
    explicit Result(T val) : storage_(std::move(val)) {}
    explicit Result(Error err) : storage_(std::move(err)) {}

    std::variant<T, Error> storage_;
};

#define DPNF_MAKE_ERROR(err_code, msg, subsys) \
    ::dpnf::core::Error::make((err_code), (msg), (subsys), __FILE__, static_cast<std::int32_t>(__LINE__))

} // namespace dpnf::core
