import express from 'express';
import { createServer as createViteServer } from 'vite';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import fs from 'node:fs/promises';
import path from 'node:path';
import {
  validateSandboxRunOptions,
  buildSandboxCliArgs,
  type SandboxRunOptions
} from './src/utils/dpnfLogic';

const execFileAsync = promisify(execFile);
const WORKSPACE_ROOT = process.cwd();
const DPNF_ROOT = path.join(WORKSPACE_ROOT, 'DPNF');
const SANDBOX_BIN = path.join(DPNF_ROOT, 'build', 'Sandbox', 'dpnf_sandbox');
const TESTS_BIN = path.join(DPNF_ROOT, 'build', 'Tests', 'dpnf_tests');

async function ensureBuilt(): Promise<void> {
  try {
    await fs.access(SANDBOX_BIN);
    await fs.access(TESTS_BIN);
  } catch {
    await execFileAsync('cmake', ['-S', 'DPNF', '-B', 'DPNF/build', '-DCMAKE_BUILD_TYPE=Release'], {
      cwd: WORKSPACE_ROOT
    });
    await execFileAsync('cmake', ['--build', 'DPNF/build', '--parallel'], {
      cwd: WORKSPACE_ROOT
    });
  }
}

const INSPECTABLE_FILES = [
  { path: 'DPNF/CMakeLists.txt', category: 'Build & Root', description: 'Root C++20 + Vulkan 1.3 CMake build configuration' },
  { path: 'DPNF/README.md', category: 'Build & Root', description: 'DPNF C++20 + Vulkan 1.3 architecture and module matrix' },
  { path: 'DPNF/GPU/include/dpnf/gpu/VulkanBackend.h', category: 'GPU / Vulkan 1.3', description: 'Headless Vulkan 1.3 compute engine and SPIR-V kernel dispatch API' },
  { path: 'DPNF/GPU/src/VulkanBackend.cpp', category: 'GPU / Vulkan 1.3', description: 'VkInstance, VkDevice, VkQueue, descriptor set, and SPIR-V pipeline execution' },
  { path: 'DPNF/GPU/shaders/tensor_add.comp', category: 'GPU / Vulkan 1.3', description: 'GLSL 450 Vulkan compute shader for linear combination (alpha*A + beta*B)' },
  { path: 'DPNF/GPU/shaders/fma_relu.comp', category: 'GPU / Vulkan 1.3', description: 'GLSL 450 Vulkan compute shader for fused multiply-add + LeakyReLU' },
  { path: 'DPNF/GPU/include/dpnf/gpu/ShaderTensorAddSpv.h', category: 'GPU / Vulkan 1.3', description: 'Compiled SPIR-V 1.6 binary words for tensor_add.comp' },
  { path: 'DPNF/GPU/include/dpnf/gpu/ShaderFmaReluSpv.h', category: 'GPU / Vulkan 1.3', description: 'Compiled SPIR-V 1.6 binary words for fma_relu.comp' },
  { path: 'DPNF/Tensor/include/dpnf/tensor/Tensor.h', category: 'Tensor & Memory', description: 'Multidimensional Tensor class with shapes, strides, matmul, and reductions' },
  { path: 'DPNF/Tensor/src/Tensor.cpp', category: 'Tensor & Memory', description: 'Tensor arithmetic, 2D/batched matmul, transpose, slicing, and norms' },
  { path: 'DPNF/Memory/include/dpnf/memory/MemoryPool.h', category: 'Tensor & Memory', description: '64-byte aligned CPU memory pool and PooledBuffer API' },
  { path: 'DPNF/Memory/src/MemoryPool.cpp', category: 'Tensor & Memory', description: 'Block caching allocator with hit/miss stats and ResourceTracker integration' },
  { path: 'DPNF/AutoDiff/include/dpnf/autodiff/Variable.h', category: 'AutoDiff & NeuralCore', description: 'Reverse-mode automatic differentiation Variable & finite-difference checker' },
  { path: 'DPNF/AutoDiff/src/Variable.cpp', category: 'AutoDiff & NeuralCore', description: 'Topological backpropagation tape and analytical gradient kernels' },
  { path: 'DPNF/NeuralCore/include/dpnf/neural/Module.h', category: 'AutoDiff & NeuralCore', description: 'Composable Module, Linear layer, and AdamWOptimizer API' },
  { path: 'DPNF/NeuralCore/src/Module.cpp', category: 'AutoDiff & NeuralCore', description: 'Xavier initialization, AdamW step update, and supervised training loop' },
  { path: 'DPNF/NeuralGraphics/include/dpnf/graphics/NeuralGraphicsPipeline.h', category: 'Neural Graphics', description: 'NeuralGraphicsFrame, 2x Super Resolution, Temporal Reconstruction, Denoising & .dpnf IO' },
  { path: 'DPNF/NeuralGraphics/src/NeuralGraphicsPipeline.cpp', category: 'Neural Graphics', description: 'Edge-preserving 2x upsampling, motion-vector warping, bilateral denoiser & PSNR/SSIM' },
  { path: 'DPNF/Platform/include/dpnf/platform/PlatformInfo.h', category: 'Platform & Core', description: 'OS, CPU ISA, RAM, and Vulkan 1.3 physical device probe API' },
  { path: 'DPNF/Platform/src/PlatformInfo.cpp', category: 'Platform & Core', description: 'Native vkEnumeratePhysicalDevices probe and hardware capability detector' },
  { path: 'DPNF/Config/include/dpnf/config/FrameworkConfig.h', category: 'Platform & Core', description: 'FrameworkConfig struct and validation API' },
  { path: 'DPNF/Config/src/FrameworkConfig.cpp', category: 'Platform & Core', description: 'Configuration parser, validator, and serializer' },
  { path: 'DPNF/Core/include/dpnf/core/Error.h', category: 'Platform & Core', description: '13 DPNF_ERROR_* codes, Error struct, and Result<T> monad' },
  { path: 'DPNF/Core/src/Error.cpp', category: 'Platform & Core', description: 'Error formatting and remediation lookup table' },
  { path: 'DPNF/Core/include/dpnf/core/Logger.h', category: 'Platform & Core', description: 'Thread-safe multi-sink Logger interface' },
  { path: 'DPNF/Core/src/Logger.cpp', category: 'Platform & Core', description: 'UTC ISO-8601 logging and ring buffer implementation' },
  { path: 'DPNF/Core/include/dpnf/core/ResourceTracker.h', category: 'Platform & Core', description: 'Memory allocation & leak detection interface' },
  { path: 'DPNF/Core/src/ResourceTracker.cpp', category: 'Platform & Core', description: 'Allocation tracking, budget enforcement & double-free detection' },
  { path: 'DPNF/Core/include/dpnf/core/ThreadPool.h', category: 'Platform & Core', description: 'Multi-worker job scheduling interface' },
  { path: 'DPNF/Core/src/ThreadPool.cpp', category: 'Platform & Core', description: 'Worker thread pool and future completion barriers' },
  { path: 'DPNF/Core/include/dpnf/core/CoreSystem.h', category: 'Platform & Core', description: 'Framework lifecycle & DiagnosticReport interface' },
  { path: 'DPNF/Core/src/CoreSystem.cpp', category: 'Platform & Core', description: 'CoreSystem initialize, shutdown & JSON report builder' },
  { path: 'DPNF/Sandbox/src/main.cpp', category: 'Sandbox & Tests', description: 'dpnf_sandbox CLI executing Vulkan 1.3, Neural Training & Neural Graphics' },
  { path: 'DPNF/Tests/src/CoreFoundationTests.cpp', category: 'Sandbox & Tests', description: 'GoogleTest unit & integration suite (11 test suites in dpnf_tests)' }
];

async function startServer() {
  const app = express();
  app.use(express.json());

  app.get('/api/dpnf/diagnostics', async (_req, res) => {
    try {
      await ensureBuilt();
      const startMs = performance.now();
      const { stdout: jsonOut } = await execFileAsync(SANDBOX_BIN, ['--json'], {
        cwd: WORKSPACE_ROOT
      });
      const { stdout: textOut } = await execFileAsync(SANDBOX_BIN, [], {
        cwd: WORKSPACE_ROOT
      });
      const durationMs = Number((performance.now() - startMs).toFixed(2));
      const parsed = JSON.parse(jsonOut);
      res.json({
        ok: true,
        durationMs,
        report: parsed,
        humanReadable: textOut
      });
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      res.status(500).json({ ok: false, error: message });
    }
  });

  app.post('/api/dpnf/sandbox/run', async (req, res) => {
    try {
      const options: SandboxRunOptions = {
        logLevel: req.body?.logLevel ?? 'info',
        workerThreads: Number(req.body?.workerThreads ?? 0),
        devicePolicy: req.body?.devicePolicy ?? 'auto',
        simulateErrorCode: req.body?.simulateErrorCode ?? 'NONE'
      };

      const validation = validateSandboxRunOptions(options);
      if (!validation.valid) {
        res.status(400).json({ ok: false, error: validation.error });
        return;
      }

      await ensureBuilt();
      const jsonArgs = buildSandboxCliArgs(options, true);
      const textArgs = buildSandboxCliArgs(options, false);

      const startMs = performance.now();
      const { stdout: jsonOut } = await execFileAsync(SANDBOX_BIN, jsonArgs, {
        cwd: WORKSPACE_ROOT
      });
      const { stdout: textOut, stderr: textErr } = await execFileAsync(SANDBOX_BIN, textArgs, {
        cwd: WORKSPACE_ROOT
      });
      const durationMs = Number((performance.now() - startMs).toFixed(2));

      const report = JSON.parse(jsonOut);
      res.json({
        ok: true,
        command: `./DPNF/build/Sandbox/dpnf_sandbox ${textArgs.join(' ')}`,
        durationMs,
        report,
        stdout: [textErr, textOut].filter(Boolean).join('\n')
      });
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      res.status(500).json({ ok: false, error: message });
    }
  });

  app.post('/api/dpnf/tests/run', async (_req, res) => {
    try {
      await ensureBuilt();
      const jsonPath = '/tmp/dpnf_gtest_results.json';
      const startMs = performance.now();
      const { stdout, stderr } = await execFileAsync(
        TESTS_BIN,
        [`--gtest_output=json:${jsonPath}`],
        { cwd: WORKSPACE_ROOT }
      );
      const durationMs = Number((performance.now() - startMs).toFixed(2));
      const rawJson = await fs.readFile(jsonPath, 'utf-8');
      const gtestReport = JSON.parse(rawJson);

      res.json({
        ok: true,
        command: './DPNF/build/Tests/dpnf_tests --gtest_output=json',
        durationMs,
        stdout: [stdout, stderr].filter(Boolean).join('\n'),
        gtestReport
      });
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      res.status(500).json({ ok: false, error: message });
    }
  });

  app.get('/api/dpnf/files', async (_req, res) => {
    try {
      const fileEntries = await Promise.all(
        INSPECTABLE_FILES.map(async (item) => {
          const absPath = path.join(WORKSPACE_ROOT, item.path);
          const stat = await fs.stat(absPath);
          const content = await fs.readFile(absPath, 'utf-8');
          const lineCount = content.split('\n').length;
          return {
            ...item,
            sizeBytes: stat.size,
            lineCount
          };
        })
      );
      res.json({ ok: true, files: fileEntries });
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      res.status(500).json({ ok: false, error: message });
    }
  });

  app.get('/api/dpnf/file', async (req, res) => {
    try {
      const requestedPath = String(req.query.path ?? '');
      const allowed = INSPECTABLE_FILES.find((f) => f.path === requestedPath);
      if (!allowed) {
        res.status(404).json({ ok: false, error: `File '${requestedPath}' is not in the inspectable DPNF manifest.` });
        return;
      }
      const absPath = path.join(WORKSPACE_ROOT, allowed.path);
      const content = await fs.readFile(absPath, 'utf-8');
      res.json({
        ok: true,
        file: {
          ...allowed,
          content
        }
      });
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      res.status(500).json({ ok: false, error: message });
    }
  });

  if (process.env.NODE_ENV !== 'production') {
    const vite = await createViteServer({
      server: { middlewareMode: true },
      appType: 'spa'
    });
    app.use(vite.middlewares);
  } else {
    const distPath = path.join(WORKSPACE_ROOT, 'dist');
    app.use(express.static(distPath));
    app.get('*', (_req, res) => {
      res.sendFile(path.join(distPath, 'index.html'));
    });
  }

  const port = 3000;
  app.listen(port, '0.0.0.0', () => {
    console.log(`DPNF Engineering Console server listening on http://0.0.0.0:${port}`);
  });
}

startServer();
