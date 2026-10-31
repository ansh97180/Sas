#include "dpnf/core/Error.h"

#include <sstream>

namespace dpnf::core {

std::string error_code_to_string(ErrorCode code) {
    switch (code) {
        case ErrorCode::DPNF_OK:
            return "DPNF_OK";
        case ErrorCode::DPNF_ERROR_INVALID_ARGUMENT:
            return "DPNF_ERROR_INVALID_ARGUMENT";
        case ErrorCode::DPNF_ERROR_OUT_OF_MEMORY:
            return "DPNF_ERROR_OUT_OF_MEMORY";
        case ErrorCode::DPNF_ERROR_DEVICE_NOT_FOUND:
            return "DPNF_ERROR_DEVICE_NOT_FOUND";
        case ErrorCode::DPNF_ERROR_DEVICE_LOST:
            return "DPNF_ERROR_DEVICE_LOST";
        case ErrorCode::DPNF_ERROR_UNSUPPORTED_OPERATION:
            return "DPNF_ERROR_UNSUPPORTED_OPERATION";
        case ErrorCode::DPNF_ERROR_INVALID_TENSOR:
            return "DPNF_ERROR_INVALID_TENSOR";
        case ErrorCode::DPNF_ERROR_INVALID_GRAPH:
            return "DPNF_ERROR_INVALID_GRAPH";
        case ErrorCode::DPNF_ERROR_COMPILATION_FAILED:
            return "DPNF_ERROR_COMPILATION_FAILED";
        case ErrorCode::DPNF_ERROR_EXECUTION_FAILED:
            return "DPNF_ERROR_EXECUTION_FAILED";
        case ErrorCode::DPNF_ERROR_DATASET_ERROR:
            return "DPNF_ERROR_DATASET_ERROR";
        case ErrorCode::DPNF_ERROR_MODEL_LOAD_FAILED:
            return "DPNF_ERROR_MODEL_LOAD_FAILED";
        case ErrorCode::DPNF_ERROR_MODEL_SAVE_FAILED:
            return "DPNF_ERROR_MODEL_SAVE_FAILED";
        case ErrorCode::DPNF_ERROR_TRAINING_FAILED:
            return "DPNF_ERROR_TRAINING_FAILED";
    }
    return "DPNF_ERROR_UNKNOWN";
}

std::string default_remediation_for_code(ErrorCode code) {
    switch (code) {
        case ErrorCode::DPNF_OK:
            return "No action required.";
        case ErrorCode::DPNF_ERROR_INVALID_ARGUMENT:
            return "Verify public API argument ranges, non-empty identifiers, and configuration bounds.";
        case ErrorCode::DPNF_ERROR_OUT_OF_MEMORY:
            return "Reduce batch size, enable tile-based execution, or inspect ResourceTracker peak allocations.";
        case ErrorCode::DPNF_ERROR_DEVICE_NOT_FOUND:
            return "Verify Vulkan ICD installation or configure DevicePolicy::CPUOnly fallback.";
        case ErrorCode::DPNF_ERROR_DEVICE_LOST:
            return "Check GPU command timeout (TDR), driver stability, and buffer synchronization barriers.";
        case ErrorCode::DPNF_ERROR_UNSUPPORTED_OPERATION:
            return "Check operator hardware capability matrix or fallback to reference CPU operator.";
        case ErrorCode::DPNF_ERROR_INVALID_TENSOR:
            return "Verify tensor shape metadata, contiguity, data type compatibility, and storage lifetime.";
        case ErrorCode::DPNF_ERROR_INVALID_GRAPH:
            return "Ensure acyclic topological ordering and connected input/output nodes in computation graph.";
        case ErrorCode::DPNF_ERROR_COMPILATION_FAILED:
            return "Inspect GLSL/SPIR-V shader compilation logs or graph lowering pass diagnostics.";
        case ErrorCode::DPNF_ERROR_EXECUTION_FAILED:
            return "Inspect kernel dispatch bounds, descriptor bindings, and worker thread exceptions.";
        case ErrorCode::DPNF_ERROR_DATASET_ERROR:
            return "Verify dataset manifest paths, paired LR/HR resolution alignment, and frame integrity.";
        case ErrorCode::DPNF_ERROR_MODEL_LOAD_FAILED:
            return "Verify .dpnf file header magic, schema version, and parameter tensor checksums.";
        case ErrorCode::DPNF_ERROR_MODEL_SAVE_FAILED:
            return "Verify output directory write permissions and available disk capacity.";
        case ErrorCode::DPNF_ERROR_TRAINING_FAILED:
            return "Check for NaN/Inf gradients, loss divergence, or invalid checkpoint state.";
    }
    return "Inspect framework diagnostic log for further details.";
}

std::string Error::code_name() const {
    return error_code_to_string(code);
}

std::string Error::format() const {
    if (is_ok()) {
        return "[DPNF_OK] Operation succeeded.";
    }
    std::ostringstream oss;
    oss << "[" << code_name() << "] (" << subsystem << ") " << message;
    if (!file.empty() && line > 0) {
        oss << " [at " << file << ":" << line << "]";
    }
    if (!remediation_hint.empty()) {
        oss << " | Remediation: " << remediation_hint;
    }
    return oss.str();
}

Error Error::ok() {
    return Error{};
}

Error Error::make(
    ErrorCode code,
    std::string message,
    std::string subsystem,
    std::string file,
    std::int32_t line,
    std::string remediation_hint) {
    Error err{};
    err.code = code;
    err.message = std::move(message);
    err.subsystem = std::move(subsystem);
    err.file = std::move(file);
    err.line = line;
    err.remediation_hint = remediation_hint.empty()
                               ? default_remediation_for_code(code)
                               : std::move(remediation_hint);
    return err;
}

} // namespace dpnf::core
