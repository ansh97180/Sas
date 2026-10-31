# DP Neural Framework (DPNF)

**Developer:** DP Technologies  
**Version:** `0.1.0-phase0`  
**Primary Language:** C++20 (Core) / Python (Planned Research API)  
**Primary Specialization:** Neural Graphics, Neural Super Resolution, Temporal Reconstruction, Neural Denoising, and Real-Time Neural Rendering.

---

## 1. Mission & Engineering Philosophy

The **DP Neural Framework (DPNF)** is an independent, modular, high-performance deep learning framework purpose-built around the computational demands of neural graphics and real-time rendering intelligence, while maintaining a general-purpose tensor, autograd, and operator architecture.

In accordance with DPNF's strict engineering constitution:
- Every completed module provides real, tested C++20 functionality with explicit ownership semantics (RAII), deterministic error reporting, and unit tests.
- No unimplemented subsystem is represented as complete.
- The framework executes cleanly on CPU-only machines and dynamically probes for GPU compute backends (Vulkan) without hard-coding vendor assumptions.

---

## 2. Implementation Status Matrix (Phase 0 Complete)

| Phase | Subsystem / Module | Directory | Status | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Phase 0** | **Project Foundation & CMake** | `DPNF/` | **Implemented** | CMake 3.20+, C++20, warning discipline |
| **Phase 0** | **Platform & Hardware Probing** | `DPNF/Platform/` | **Implemented** | CPU ISA flags, RAM stats, dynamic Vulkan loader probe |
| **Phase 0** | **Configuration System** | `DPNF/Config/` | **Implemented** | `FrameworkConfig`, validation, key-value serialization |
| **Phase 0** | **Core System, Errors, Logging, Jobs** | `DPNF/Core/` | **Implemented** | All 13 `DPNF_ERROR_*` codes, `Result<T>`, `Logger`, `ThreadPool`, `ResourceTracker` |
| **Phase 0** | **Sandbox Application** | `DPNF/Sandbox/` | **Implemented** | `dpnf_sandbox` CLI & JSON hardware/runtime inspector |
| **Phase 0** | **Unit & Integration Test Suite** | `DPNF/Tests/` | **Implemented** | GoogleTest (`dpnf_tests`) verifying all Phase 0 subsystems |
| **Phase 1** | **CPU Tensor Foundation** | `DPNF/Tensor/`, `DPNF/Memory/` | **Planned** | Multidimensional shapes, strides, views, CPU storage |
| **Phase 2** | **Autodiff & Operator Library** | `DPNF/AutoDiff/`, `DPNF/Operators/` | **Planned** | Reverse-mode tape, numerical gradient checking |
| **Phase 3** | **Neural Network Core** | `DPNF/NeuralCore/` | **Planned** | `Module`, `Parameter`, `Conv2D`, `LayerNorm`, `ResidualBlock` |
| **Phase 4** | **Training Engine & Optimizers** | `DPNF/Training/`, `DPNF/Optimizers/`, `DPNF/Losses/` | **Planned** | Trainer loop, AdamW, checkpoint recovery |
| **Phase 5** | **Vulkan Compute Foundation** | `DPNF/GPU/Vulkan/` | **Planned** | Headless Vulkan instance, compute queue, SPIR-V pipelines |
| **Phase 6** | **GPU Tensor Operations** | `DPNF/GPU/` | **Planned** | Elementwise, reduction, GEMM, activation compute kernels |
| **Phase 7** | **Neural Graphics Foundation** | `DPNF/NeuralGraphics/`, `DPNF/Dataset/` | **Planned** | `NeuralGraphicsFrame`, G-buffer validation, paired datasets |
| **Phase 8** | **Super Resolution Research** | `DPNF/SuperResolution/` | **Planned** | Multi-scale reconstruction, PSNR/SSIM evaluation |
| **Phase 9** | **Temporal Reconstruction** | `DPNF/Temporal/` | **Planned** | Motion-vector warping, disocclusion rejection, history buffers |
| **Phase 10** | **Neural Rendering Runtime** | `DPNF/Runtime/`, `DPNF/Serialization/` | **Planned** | `.dpnf` binary format, real-time C++ inference session |
| **Phase 11** | **Advanced Framework & Studios** | `DPNF/Compiler/`, `DPNF/ModelStudio/` | **Planned** | Graph fusion, mixed precision, desktop & web studios |

---

## 3. Building & Running Phase 0

```bash
# Configure
cmake -S DPNF -B DPNF/build -DCMAKE_BUILD_TYPE=Release

# Build all targets (dpnf_platform, dpnf_config, dpnf_core, dpnf_sandbox, dpnf_tests)
cmake --build DPNF/build --parallel

# Run Sandbox executable
./DPNF/build/Sandbox/dpnf_sandbox

# Run GoogleTest suite
ctest --test-dir DPNF/build --output-on-failure
```
