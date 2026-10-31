export type ImplementationStatus = 'Implemented' | 'Planned';

export interface DpnfModuleEntry {
  id: string;
  name: string;
  directory: string;
  phase: number;
  phaseTitle: string;
  status: ImplementationStatus;
  category: 'Foundation' | 'Compute & Memory' | 'Autodiff & Neural' | 'Neural Graphics' | 'Runtime & Tools';
  responsibility: string;
  publicHeaders: string[];
  sourceFiles: string[];
  unitTestSuite: string;
}

export interface DpnfErrorSpec {
  code: number;
  name: string;
  subsystem: string;
  description: string;
  remediation: string;
  triggerCondition: string;
}

export interface SandboxRunOptions {
  logLevel: 'trace' | 'debug' | 'info' | 'warn' | 'error' | 'fatal' | 'off';
  workerThreads: number;
  devicePolicy: 'auto' | 'cpu_only' | 'prefer_gpu';
  simulateErrorCode: string;
}

export interface ValidationResult {
  valid: boolean;
  error?: string;
}

export const DPNF_ERROR_REGISTRY: DpnfErrorSpec[] = [
  {
    code: 1,
    name: 'DPNF_ERROR_INVALID_ARGUMENT',
    subsystem: 'Core / Config / Public API',
    description: 'An argument passed to a public DPNF API violates documented domain, range, or state preconditions.',
    remediation: 'Verify public API argument ranges, non-empty identifiers, and configuration bounds.',
    triggerCondition: 'Empty application_name, worker_threads > 256, double-free allocation ID, or duplicate CoreSystem::initialize().'
  },
  {
    code: 2,
    name: 'DPNF_ERROR_OUT_OF_MEMORY',
    subsystem: 'Memory / ResourceTracker',
    description: 'Requested host CPU or device GPU memory allocation exceeds physical capacity or configured ResourceTracker budget.',
    remediation: 'Reduce batch size, enable tile-based execution, or inspect ResourceTracker peak allocations.',
    triggerCondition: 'ResourceTracker::record_allocation() exceeds max_budget_bytes or host allocation returns nullptr.'
  },
  {
    code: 3,
    name: 'DPNF_ERROR_DEVICE_NOT_FOUND',
    subsystem: 'Platform / GPU Backend',
    description: 'Requested compute accelerator or Vulkan physical device could not be enumerated on the host system.',
    remediation: 'Verify Vulkan ICD installation or configure DevicePolicy::CPUOnly fallback.',
    triggerCondition: 'Strict GPU device request when libvulkan.so.1 / vulkan-1.dll or compatible physical GPU is absent.'
  },
  {
    code: 4,
    name: 'DPNF_ERROR_DEVICE_LOST',
    subsystem: 'GPU / Vulkan Queue',
    description: 'Logical compute device became unresponsive or terminated during command buffer execution (VK_ERROR_DEVICE_LOST).',
    remediation: 'Check GPU command timeout (TDR), driver stability, and buffer synchronization barriers.',
    triggerCondition: 'Long-running compute shader exceeds OS watchdog timeout or driver reset occurs.'
  },
  {
    code: 5,
    name: 'DPNF_ERROR_UNSUPPORTED_OPERATION',
    subsystem: 'Operators / Precision',
    description: 'Requested operator, data type (e.g. FP16/BF16), or tensor layout is not supported by the target execution backend.',
    remediation: 'Check operator hardware capability matrix or fallback to reference CPU operator.',
    triggerCondition: 'Dispatching an unimplemented precision or hardware-specific kernel without CPU fallback.'
  },
  {
    code: 6,
    name: 'DPNF_ERROR_INVALID_TENSOR',
    subsystem: 'Tensor Engine',
    description: 'Tensor metadata, shape dimensions, stride layout, or underlying storage handle is invalid or unallocated.',
    remediation: 'Verify tensor shape metadata, contiguity, data type compatibility, and storage lifetime.',
    triggerCondition: 'Incompatible broadcast dimensions, negative strides where disallowed, or out-of-bounds slice.'
  },
  {
    code: 7,
    name: 'DPNF_ERROR_INVALID_GRAPH',
    subsystem: 'Graph / AutoDiff',
    description: 'Computation graph contains cycles, disconnected input/output bindings, or unresolved topological dependencies.',
    remediation: 'Ensure acyclic topological ordering and connected input/output nodes in computation graph.',
    triggerCondition: 'Cyclic dependency detected during topological sort or backward pass traversal.'
  },
  {
    code: 8,
    name: 'DPNF_ERROR_COMPILATION_FAILED',
    subsystem: 'Compiler / SPIR-V',
    description: 'Graph compiler lowering pass or GLSL-to-SPIR-V compute shader compilation failed.',
    remediation: 'Inspect GLSL/SPIR-V shader compilation logs or graph lowering pass diagnostics.',
    triggerCondition: 'Shader syntax error, unsupported SPIR-V capability, or invalid kernel specialization constant.'
  },
  {
    code: 9,
    name: 'DPNF_ERROR_EXECUTION_FAILED',
    subsystem: 'Execution / Runtime',
    description: 'Runtime operator dispatch, thread pool task execution, or command buffer submission failed.',
    remediation: 'Inspect kernel dispatch bounds, descriptor bindings, and worker thread exceptions.',
    triggerCondition: 'Unhandled kernel runtime failure or synchronization fence failure.'
  },
  {
    code: 10,
    name: 'DPNF_ERROR_DATASET_ERROR',
    subsystem: 'Dataset / Preprocessing',
    description: 'Dataset manifest parsing, G-buffer frame alignment validation, or sample decoding failed.',
    remediation: 'Verify dataset manifest paths, paired LR/HR resolution alignment, and frame integrity.',
    triggerCondition: 'Mismatched resolution scale between low-res input and high-res reference or missing motion buffer.'
  },
  {
    code: 11,
    name: 'DPNF_ERROR_MODEL_LOAD_FAILED',
    subsystem: 'Serialization / Runtime',
    description: 'Failed to deserialize a .dpnf model archive or training checkpoint from storage.',
    remediation: 'Verify .dpnf file header magic, schema version, and parameter tensor checksums.',
    triggerCondition: 'Corrupted .dpnf file header, incompatible format version, or missing parameter tensor.'
  },
  {
    code: 12,
    name: 'DPNF_ERROR_MODEL_SAVE_FAILED',
    subsystem: 'Serialization / Checkpointing',
    description: 'Failed to write serialized .dpnf model or atomic training checkpoint to disk.',
    remediation: 'Verify output directory write permissions and available disk capacity.',
    triggerCondition: 'Read-only target path, disk full, or interrupted atomic rename.'
  },
  {
    code: 13,
    name: 'DPNF_ERROR_TRAINING_FAILED',
    subsystem: 'Training Engine',
    description: 'Training loop terminated due to numerical instability (NaN/Inf loss), optimizer failure, or invalid configuration.',
    remediation: 'Check for NaN/Inf gradients, loss divergence, or invalid checkpoint state.',
    triggerCondition: 'Non-finite loss value detected during backpropagation or gradient update.'
  }
];

export const DPNF_MODULES: DpnfModuleEntry[] = [
  {
    id: 'platform',
    name: 'Platform & Hardware Detection',
    directory: 'DPNF/Platform',
    phase: 0,
    phaseTitle: 'Phase 0: Project Foundation',
    status: 'Implemented',
    category: 'Foundation',
    responsibility: 'OS identification, CPU topology & SIMD/AMX instruction set detection, system RAM query, and dynamic Vulkan loader probing with guaranteed CPU fallback.',
    publicHeaders: ['DPNF/Platform/include/dpnf/platform/PlatformInfo.hpp'],
    sourceFiles: ['DPNF/Platform/src/PlatformInfo.cpp'],
    unitTestSuite: 'PlatformDetectorTest.ReportsValidHostCapabilities'
  },
  {
    id: 'config',
    name: 'Configuration Management',
    directory: 'DPNF/Config',
    phase: 0,
    phaseTitle: 'Phase 0: Project Foundation',
    status: 'Implemented',
    category: 'Foundation',
    responsibility: 'Strongly typed FrameworkConfig, parameter validation, deterministic seed configuration, device policy selection, and key-value serialization.',
    publicHeaders: ['DPNF/Config/include/dpnf/config/FrameworkConfig.hpp'],
    sourceFiles: ['DPNF/Config/src/FrameworkConfig.cpp'],
    unitTestSuite: 'ConfigTest.ValidationAndKeyValueRoundTrip, ConfigTest.RejectsInvalidParameters'
  },
  {
    id: 'core',
    name: 'Core System, Error, Logger, ResourceTracker & ThreadPool',
    directory: 'DPNF/Core',
    phase: 0,
    phaseTitle: 'Phase 0: Project Foundation',
    status: 'Implemented',
    category: 'Foundation',
    responsibility: 'Framework lifecycle (initialize/shutdown), 13 DPNF_ERROR_* structured error types & Result<T>, multi-sink thread-safe Logger, ResourceTracker with budget & double-free detection, and worker ThreadPool.',
    publicHeaders: [
      'DPNF/Core/include/dpnf/core/Error.hpp',
      'DPNF/Core/include/dpnf/core/Logger.hpp',
      'DPNF/Core/include/dpnf/core/ResourceTracker.hpp',
      'DPNF/Core/include/dpnf/core/ThreadPool.hpp',
      'DPNF/Core/include/dpnf/core/CoreSystem.hpp'
    ],
    sourceFiles: [
      'DPNF/Core/src/Error.cpp',
      'DPNF/Core/src/Logger.cpp',
      'DPNF/Core/src/ResourceTracker.cpp',
      'DPNF/Core/src/ThreadPool.cpp',
      'DPNF/Core/src/CoreSystem.cpp'
    ],
    unitTestSuite: 'ErrorSystemTest.*, LoggerTest.*, ResourceTrackerTest.*, ThreadPoolTest.*, CoreSystemTest.*'
  },
  {
    id: 'sandbox',
    name: 'Sandbox Diagnostic Application',
    directory: 'DPNF/Sandbox',
    phase: 0,
    phaseTitle: 'Phase 0: Project Foundation',
    status: 'Implemented',
    category: 'Foundation',
    responsibility: 'CLI diagnostic executable (dpnf_sandbox) that initializes DPNF, exercises ResourceTracker and ThreadPool, tests structured error reporting, and outputs JSON or human-readable reports.',
    publicHeaders: [],
    sourceFiles: ['DPNF/Sandbox/src/main.cpp'],
    unitTestSuite: 'CLI Smoke & Integration Verification'
  },
  {
    id: 'tests',
    name: 'GoogleTest Unit & Integration Suite',
    directory: 'DPNF/Tests',
    phase: 0,
    phaseTitle: 'Phase 0: Project Foundation',
    status: 'Implemented',
    category: 'Foundation',
    responsibility: 'Automated C++20 unit and integration tests (dpnf_tests) validating every Phase 0 subsystem via CTest and GoogleTest.',
    publicHeaders: [],
    sourceFiles: ['DPNF/Tests/src/CoreFoundationTests.cpp'],
    unitTestSuite: 'DPNF_Phase0_UnitTests (8 test cases)'
  },
  {
    id: 'tensor',
    name: 'Multidimensional Tensor Engine & Memory Subsystem',
    directory: 'DPNF/Tensor, DPNF/Memory',
    phase: 1,
    phaseTitle: 'Phase 1: CPU Tensor Foundation',
    status: 'Planned',
    category: 'Compute & Memory',
    responsibility: 'N-dimensional shapes, explicit strides, views, slicing, broadcasting, CPU storage ownership, memory pooling, and basic arithmetic/reductions independent of Vulkan.',
    publicHeaders: [],
    sourceFiles: [],
    unitTestSuite: 'Scheduled for Phase 1'
  },
  {
    id: 'autodiff',
    name: 'Reverse-Mode Automatic Differentiation & Operator Library',
    directory: 'DPNF/AutoDiff, DPNF/Operators',
    phase: 2,
    phaseTitle: 'Phase 2: Autodiff and Operators',
    status: 'Planned',
    category: 'Autodiff & Neural',
    responsibility: 'Dynamic computation graph recording, backward propagation, gradient accumulation, operator registry, and numerical finite-difference gradient checking.',
    publicHeaders: [],
    sourceFiles: [],
    unitTestSuite: 'Scheduled for Phase 2'
  },
  {
    id: 'neural_core',
    name: 'Neural Network Core (Modules & Layers)',
    directory: 'DPNF/NeuralCore',
    phase: 3,
    phaseTitle: 'Phase 3: Neural Network Core',
    status: 'Planned',
    category: 'Autodiff & Neural',
    responsibility: 'Composable Module hierarchy, Parameter/Buffer registration, Linear, Conv2D/3D, Depthwise/Transposed Conv, LayerNorm, GroupNorm, ResidualBlock, and AttentionBlock.',
    publicHeaders: [],
    sourceFiles: [],
    unitTestSuite: 'Scheduled for Phase 3'
  },
  {
    id: 'training',
    name: 'Training Engine, Optimizers & Losses',
    directory: 'DPNF/Training, DPNF/Optimizers, DPNF/Losses',
    phase: 4,
    phaseTitle: 'Phase 4: Training System',
    status: 'Planned',
    category: 'Autodiff & Neural',
    responsibility: 'Trainer loop, SGD/Adam/AdamW/RMSprop optimizers, L1/L2/SSIM/edge/temporal losses, deterministic seeding, and atomic checkpoint recovery.',
    publicHeaders: [],
    sourceFiles: [],
    unitTestSuite: 'Scheduled for Phase 4'
  },
  {
    id: 'vulkan_compute',
    name: 'Headless Vulkan Compute Backend & GPU Kernels',
    directory: 'DPNF/GPU/Vulkan',
    phase: 5,
    phaseTitle: 'Phase 5 & 6: Vulkan Compute & GPU Tensor Operations',
    status: 'Planned',
    category: 'Compute & Memory',
    responsibility: 'Headless VkInstance, physical device discovery, compute queues, SPIR-V shader pipelines, descriptor sets, and validated GPU tensor kernels.',
    publicHeaders: [],
    sourceFiles: [],
    unitTestSuite: 'Scheduled for Phases 5 & 6'
  },
  {
    id: 'neural_graphics',
    name: 'Neural Graphics, Super Resolution & Temporal Reconstruction',
    directory: 'DPNF/NeuralGraphics, DPNF/SuperResolution, DPNF/Temporal, DPNF/Denoising',
    phase: 7,
    phaseTitle: 'Phases 7–9: Neural Graphics, Super Resolution & Temporal',
    status: 'Planned',
    category: 'Neural Graphics',
    responsibility: 'NeuralGraphicsFrame (color, albedo, normal, depth, motion vectors, exposure, jitter), multi-scale upsampling, motion-vector warping, history rejection, and PSNR/SSIM evaluation.',
    publicHeaders: [],
    sourceFiles: [],
    unitTestSuite: 'Scheduled for Phases 7–9'
  },
  {
    id: 'runtime',
    name: 'Real-Time C++ Inference Runtime & .dpnf Serialization',
    directory: 'DPNF/Runtime, DPNF/Serialization, DPNF/Compiler',
    phase: 10,
    phaseTitle: 'Phases 10–11: Real-Time Rendering Runtime & Advanced Tools',
    status: 'Planned',
    category: 'Runtime & Tools',
    responsibility: 'Standalone C++ InferenceSession for DP Engine integration without Python runtime dependency, .dpnf binary model serialization, and graph optimization.',
    publicHeaders: [],
    sourceFiles: [],
    unitTestSuite: 'Scheduled for Phases 10–11'
  }
];

export function validateSandboxRunOptions(options: SandboxRunOptions): ValidationResult {
  const validLevels = ['trace', 'debug', 'info', 'warn', 'error', 'fatal', 'off'];
  if (!validLevels.includes(options.logLevel)) {
    return { valid: false, error: `Invalid logLevel '${options.logLevel}'.` };
  }

  if (!Number.isInteger(options.workerThreads) || options.workerThreads < 0 || options.workerThreads > 256) {
    return {
      valid: false,
      error: `workerThreads must be an integer between 0 (auto) and 256. Received: ${options.workerThreads}`
    };
  }

  const validPolicies = ['auto', 'cpu_only', 'prefer_gpu'];
  if (!validPolicies.includes(options.devicePolicy)) {
    return { valid: false, error: `Invalid devicePolicy '${options.devicePolicy}'.` };
  }

  if (options.simulateErrorCode && options.simulateErrorCode !== 'NONE') {
    const exists = DPNF_ERROR_REGISTRY.some((e) => e.name === options.simulateErrorCode);
    if (!exists) {
      return {
        valid: false,
        error: `Unknown DPNF error code '${options.simulateErrorCode}'.`
      };
    }
  }

  return { valid: true };
}

export function buildSandboxCliArgs(options: SandboxRunOptions, jsonMode: boolean): string[] {
  const args: string[] = [];
  if (jsonMode) {
    args.push('--json');
  }
  args.push('--log-level', options.logLevel);
  args.push('--threads', String(options.workerThreads));
  args.push('--device-policy', options.devicePolicy);
  if (options.simulateErrorCode && options.simulateErrorCode !== 'NONE') {
    args.push('--simulate-error', options.simulateErrorCode);
  }
  return args;
}

export function filterDpnfModules(
  modules: DpnfModuleEntry[],
  statusFilter: 'ALL' | ImplementationStatus,
  searchQuery: string
): DpnfModuleEntry[] {
  const q = searchQuery.trim().toLowerCase();
  return modules.filter((m) => {
    if (statusFilter !== 'ALL' && m.status !== statusFilter) {
      return false;
    }
    if (!q) {
      return true;
    }
    return (
      m.name.toLowerCase().includes(q) ||
      m.directory.toLowerCase().includes(q) ||
      m.responsibility.toLowerCase().includes(q) ||
      m.phaseTitle.toLowerCase().includes(q)
    );
  });
}

export function formatBytes(bytes: number): string {
  if (!Number.isFinite(bytes) || bytes < 0) {
    return '0 B';
  }
  if (bytes === 0) {
    return '0 B';
  }
  const units = ['B', 'KiB', 'MiB', 'GiB', 'TiB'];
  const idx = Math.min(Math.floor(Math.log(bytes) / Math.log(1024)), units.length - 1);
  const value = bytes / Math.pow(1024, idx);
  return `${idx === 0 ? value.toFixed(0) : value.toFixed(2)} ${units[idx]}`;
}
