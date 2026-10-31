# DP Neural Framework (DPNF)

**Developer:** DP Technologies  
**Category:** Advanced Deep Learning & Neural Graphics Framework  
**Language & Compute:** C++20 (`-std=c++20`) + Vulkan 1.3 Compute (`Vulkan::Vulkan`, SPIR-V 1.6)  

---

## 1. Executive Summary

**DP Neural Framework (DPNF)** is an independent, modular, high-performance C++20 and Vulkan 1.3 deep learning and neural graphics framework engineered for:
- **Neural Super Resolution & Temporal Reconstruction** (`DPNF/NeuralGraphics`)
- **Intelligent G-Buffer Denoising & Image Reconstruction** (`DPNF/NeuralGraphics`)
- **Headless Vulkan 1.3 SPIR-V Compute Shaders** (`DPNF/GPU`)
- **Multidimensional Tensor Engine & 64-Byte Aligned Pooled Allocator** (`DPNF/Tensor`, `DPNF/Memory`)
- **Reverse-Mode Automatic Differentiation & Analytical Gradient Verification** (`DPNF/AutoDiff`)
- **Neural Network Modules, Linear Layers & AdamW Optimizer** (`DPNF/NeuralCore`)
- **Binary `.dpnf` Model Serialization & Standalone C++ Runtime** (`DPNF/NeuralGraphics`)

All headers and implementation files strictly use `.h` and `.cpp` extensions (`DPNF/<Module>/include/dpnf/<module>/*.h` and `DPNF/<Module>/src/*.cpp`), with GLSL compute shaders in `DPNF/GPU/shaders/*.comp` compiled into SPIR-V via `glslangValidator`.

---

## 2. Implemented & Verified Modules (`v1.0.0`)

| Module | Directory | Public Header (`.h`) | Implementation (`.cpp`) | Verification Suite |
| :--- | :--- | :--- | :--- | :--- |
| **Platform** | `DPNF/Platform` | `PlatformInfo.h` | `PlatformInfo.cpp` | `PlatformVulkanTest.DetectsHostAndVulkan13ComputeDevice` |
| **Config** | `DPNF/Config` | `FrameworkConfig.h` | `FrameworkConfig.cpp` | `ConfigTest.ValidationAndRoundTrip` |
| **Core** | `DPNF/Core` | `Error.h`, `Logger.h`, `ResourceTracker.h`, `ThreadPool.h`, `CoreSystem.h` | `Error.cpp`, `Logger.cpp`, `ResourceTracker.cpp`, `ThreadPool.cpp`, `CoreSystem.cpp` | `ErrorSystemTest.*`, `ResourceTrackerTest.*`, `ThreadPoolTest.*` |
| **Memory** | `DPNF/Memory` | `MemoryPool.h` | `MemoryPool.cpp` | `MemoryPoolTest.ReusesAlignedBlocksAndTracksZeroLeaks` |
| **Tensor** | `DPNF/Tensor` | `Tensor.h` | `Tensor.cpp` | `TensorTest.ArithmeticMatmulReshapeAndTranspose` |
| **GPU (Vulkan)** | `DPNF/GPU` | `VulkanBackend.h`, `ShaderTensorAddSpv.h`, `ShaderFmaReluSpv.h` | `VulkanBackend.cpp` (`tensor_add.comp`, `fma_relu.comp`) | `VulkanComputeTest.ExecutesRealSpirvShadersOnVulkanQueue` |
| **AutoDiff** | `DPNF/AutoDiff` | `Variable.h` | `Variable.cpp` | `AutoDiffTest.ReverseModeMatchesFiniteDifferenceGradients` |
| **NeuralCore** | `DPNF/NeuralCore` | `Module.h` | `Module.cpp` | `NeuralTrainingTest.MlpWithAdamWConvergesOnTarget` |
| **NeuralGraphics** | `DPNF/NeuralGraphics` | `NeuralGraphicsPipeline.h` | `NeuralGraphicsPipeline.cpp` | `NeuralGraphicsTest.SuperResolutionTemporalDenoisingAndDpnfSerialization` |
| **Sandbox** | `DPNF/Sandbox` | — | `main.cpp` | `dpnf_sandbox` CLI & JSON diagnostics |
| **Tests** | `DPNF/Tests` | — | `CoreFoundationTests.cpp` | `ctest` (11 GoogleTest suites, 100% passing) |

---

## 3. Build & Test Instructions

```bash
cmake -S DPNF -B DPNF/build -DCMAKE_BUILD_TYPE=Release
cmake --build DPNF/build --parallel
ctest --test-dir DPNF/build --output-on-failure
./DPNF/build/Sandbox/dpnf_sandbox
```
