# DPNF Getting Started & Build Guide (Phase 0)

## 1. Prerequisites

- **C++ Compiler:** C++20 compliant compiler (`GCC >= 11`, `Clang >= 14`, or `MSVC >= 19.30`).
- **Build System:** `CMake >= 3.20` and `Make` or `Ninja`.
- **Testing Dependency:** `GoogleTest` (`libgtest-dev`).
- **GPU Compute (Optional in Phase 0, used in Phase 5+):** Vulkan Loader (`libvulkan.so.1` / `vulkan-1.dll`). When no Vulkan ICD or physical GPU is available, DPNF automatically runs in deterministic CPU execution mode.

---

## 2. Building DPNF from Source

```bash
# 1. Configure CMake build directory
cmake -S DPNF -B DPNF/build -DCMAKE_BUILD_TYPE=Release

# 2. Compile libraries, Sandbox executable, and GoogleTest binary
cmake --build DPNF/build --parallel

# 3. Run unit & integration tests
ctest --test-dir DPNF/build --output-on-failure

# 4. Run the DPNF Sandbox diagnostic utility
./DPNF/build/Sandbox/dpnf_sandbox
./DPNF/build/Sandbox/dpnf_sandbox --json
```

---

## 3. Architectural Overview (Phase 0)

DPNF is structured into decoupled modules with strict header/implementation separation:

1. **`DPNF/Platform` (`libdpnf_platform.a`)**:
   - Queries OS kernel metadata, CPU physical/logical topology, vector/matrix instruction extensions (`SSE4.2`, `AVX2`, `FMA`, `AVX512F`, `AVX512_FP16`, `AMX_BF16`, `AMX_INT8`), and physical memory bounds.
   - Dynamically probes for `libvulkan.so.1` / `vulkan-1.dll` at runtime without linking against a static Vulkan library, ensuring 100% portability on headless CPU nodes.
2. **`DPNF/Config` (`libdpnf_config.a`)**:
   - Defines `FrameworkConfig` with validation rules and deterministic key-value configuration serialization.
3. **`DPNF/Core` (`libdpnf_core.a`)**:
   - `Error` & `Result<T>`: Structured error propagation across all 13 `DPNF_ERROR_*` categories with file/line diagnostics and remediation guidance.
   - `Logger`: Thread-safe multi-sink logger with UTC ISO-8601 timestamps and ring-buffer inspection.
   - `ResourceTracker`: Tracks allocations, enforces optional memory budgets, records peak watermarks, and detects double-frees or shutdown leaks.
   - `ThreadPool`: Multi-worker job queue with `std::future<void>` completion handles and `wait_idle()` barriers.
   - `CoreSystem`: Central singleton managing initialization, hardware discovery, resource tracking, and diagnostic report generation.
