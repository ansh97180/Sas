# Changelog

All notable changes to the **DP Neural Framework (DPNF)** are documented in this file.

## [0.1.0-phase0] - 2026-10-31

### Added (Phase 0: Project Foundation)
- **Root CMake Build System**: Modern C++20 CMake configuration (`CMakeLists.txt`, `CMakePresets.json`) with strict warning flags (`-Wall -Wextra -Wpedantic -Wconversion`).
- **Platform Module (`DPNF/Platform`)**: OS identification, CPU topology and ISA feature detection (`SSE4.2`, `AVX`, `AVX2`, `FMA`, `AVX512F`, `AVX512VL`, `AVX512BW`, `AVX512_FP16`, `AMX_BF16`, `AMX_INT8`), system RAM reporting, and runtime dynamic library probing for Vulkan loader (`libvulkan.so.1` / `vulkan-1.dll`) with clean CPU-only fallback.
- **Config Module (`DPNF/Config`)**: Strongly typed `FrameworkConfig` supporting key-value config file parsing, programmatic validation, deterministic seed configuration, thread pool sizing, and device policy (`Auto`, `CPUOnly`, `PreferGPU`).
- **Core Module (`DPNF/Core`)**:
  - Complete structured error handling system covering all 13 `DPNF_ERROR_*` categories (`ErrorCode`, `Error`, `Result<T>`), source location capture, and diagnostic context strings.
  - Thread-safe multi-sink `Logger` supporting console output, file output, and in-memory ring-buffer capture with configurable severity filtering (`Trace`, `Debug`, `Info`, `Warn`, `Error`, `Fatal`, `Off`).
  - `ResourceTracker` with atomic allocation tracking, peak memory watermark, active allocation map, and leak detection reports.
  - `ThreadPool` work-stealing/queued task scheduler with future completion synchronization.
  - `CoreSystem` lifecycle manager (`initialize`, `shutdown`, `is_initialized`, `generate_diagnostic_report`).
- **Sandbox Executable (`DPNF/Sandbox`)**: CLI diagnostic tool (`dpnf_sandbox`) supporting `--json`, `--log-level`, `--threads`, `--simulate-error`, and `--config` arguments.
- **GoogleTest Suite (`DPNF/Tests`)**: 24 unit and integration tests validating error codes, `Result<T>` monadic semantics, logger sinks, config validation, hardware capability probing, resource tracking leak detection, thread pool execution, and core lifecycle state transitions.
- **Documentation (`DPNF/Documentation`)**: Architecture Overview, Build Guide, Getting Started Guide, and Phase Implementation Matrix.
