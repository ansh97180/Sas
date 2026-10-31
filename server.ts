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
  { path: 'DPNF/CMakeLists.txt', category: 'Build & Root', description: 'Root C++20 CMake build configuration' },
  { path: 'DPNF/CMakePresets.json', category: 'Build & Root', description: 'Cross-platform CMake configure & test presets' },
  { path: 'DPNF/README.md', category: 'Build & Root', description: 'DPNF Phase 0 overview and status matrix' },
  { path: 'DPNF/CHANGELOG.md', category: 'Build & Root', description: 'Phase 0 release notes and changelog' },
  { path: 'DPNF/Platform/CMakeLists.txt', category: 'Platform Module', description: 'Platform module build target' },
  { path: 'DPNF/Platform/include/dpnf/platform/PlatformInfo.hpp', category: 'Platform Module', description: 'OS, CPU ISA, RAM, and Vulkan probe public API' },
  { path: 'DPNF/Platform/src/PlatformInfo.cpp', category: 'Platform Module', description: 'Hardware capability and dynamic Vulkan loader implementation' },
  { path: 'DPNF/Config/CMakeLists.txt', category: 'Config Module', description: 'Config module build target' },
  { path: 'DPNF/Config/include/dpnf/config/FrameworkConfig.hpp', category: 'Config Module', description: 'FrameworkConfig struct and validation API' },
  { path: 'DPNF/Config/src/FrameworkConfig.cpp', category: 'Config Module', description: 'Configuration parser, validator, and serializer' },
  { path: 'DPNF/Core/CMakeLists.txt', category: 'Core Module', description: 'Core module build target' },
  { path: 'DPNF/Core/include/dpnf/core/Error.hpp', category: 'Core Module', description: '13 DPNF_ERROR_* codes, Error struct, and Result<T> monad' },
  { path: 'DPNF/Core/src/Error.cpp', category: 'Core Module', description: 'Error formatting and remediation lookup table' },
  { path: 'DPNF/Core/include/dpnf/core/Logger.hpp', category: 'Core Module', description: 'Thread-safe multi-sink Logger interface' },
  { path: 'DPNF/Core/src/Logger.cpp', category: 'Core Module', description: 'UTC ISO-8601 logging and ring buffer implementation' },
  { path: 'DPNF/Core/include/dpnf/core/ResourceTracker.hpp', category: 'Core Module', description: 'Memory allocation & leak detection interface' },
  { path: 'DPNF/Core/src/ResourceTracker.cpp', category: 'Core Module', description: 'Allocation tracking, budget enforcement & double-free detection' },
  { path: 'DPNF/Core/include/dpnf/core/ThreadPool.hpp', category: 'Core Module', description: 'Multi-worker job scheduling interface' },
  { path: 'DPNF/Core/src/ThreadPool.cpp', category: 'Core Module', description: 'Worker thread pool and future completion barriers' },
  { path: 'DPNF/Core/include/dpnf/core/CoreSystem.hpp', category: 'Core Module', description: 'Framework lifecycle & DiagnosticReport interface' },
  { path: 'DPNF/Core/src/CoreSystem.cpp', category: 'Core Module', description: 'CoreSystem initialize, shutdown & JSON report builder' },
  { path: 'DPNF/Sandbox/src/main.cpp', category: 'Sandbox & Tests', description: 'dpnf_sandbox CLI diagnostic application' },
  { path: 'DPNF/Tests/src/CoreFoundationTests.cpp', category: 'Sandbox & Tests', description: 'GoogleTest unit & integration test suite (dpnf_tests)' },
  { path: 'DPNF/Documentation/GettingStarted_and_Architecture.md', category: 'Documentation', description: 'Phase 0 Getting Started, Build Guide & Architecture reference' }
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
