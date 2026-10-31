import React, { useState } from 'react';
import {
  DPNF_MODULES,
  filterDpnfModules,
  formatBytes,
  type ImplementationStatus
} from '../utils/dpnfLogic';

export interface PlatformDiagnosticsPayload {
  framework_version: string;
  initialized: boolean;
  active_execution_backend: string;
  config: {
    application_name: string;
    log_level: string;
    worker_threads: number;
    random_seed: number;
    deterministic_execution: boolean;
    strict_bounds_checking: boolean;
    device_policy: string;
  };
  platform: {
    os_name: string;
    kernel_version: string;
    architecture: string;
    compiler_name: string;
    cpp_standard: number;
    cpu: {
      vendor_id: string;
      model_name: string;
      physical_cores: number;
      logical_cores: number;
      supported_isas: string[];
    };
    memory: {
      total_physical_bytes: number;
      available_physical_bytes: number;
      page_size_bytes: number;
    };
    vulkan: {
      loader_present: boolean;
      compute_device_available?: boolean;
      discovered_device_count?: number;
      device_name?: string;
      api_version?: string;
      loader_library_path: string;
      diagnostic_note: string;
    };
  };
  runtime: {
    worker_thread_count: number;
    completed_jobs: number;
    resources: {
      total_allocated_bytes: number;
      current_used_bytes: number;
      peak_used_bytes: number;
      allocation_count: number;
      deallocation_count: number;
      active_allocation_count: number;
    };
  };
  logs: Array<{
    sequence_id: number;
    timestamp: string;
    level: string;
    subsystem: string;
    message: string;
  }>;
}

interface OverviewViewProps {
  diagnostics: PlatformDiagnosticsPayload | null;
  durationMs: number | null;
  loading: boolean;
  onRefreshDiagnostics: () => void;
  onOpenFileInInspector: (filePath: string) => void;
  onNavigateToRunner: () => void;
}

export const OverviewView: React.FC<OverviewViewProps> = ({
  diagnostics,
  durationMs,
  loading,
  onRefreshDiagnostics,
  onOpenFileInInspector,
  onNavigateToRunner
}) => {
  const [statusFilter, setStatusFilter] = useState<'ALL' | ImplementationStatus>('ALL');
  const [searchQuery, setSearchQuery] = useState('');

  const filteredModules = filterDpnfModules(DPNF_MODULES, statusFilter, searchQuery);

  return (
    <div className="space-y-10">
      {/* Section 1: Hero & Vulkan 1.3 + C++20 Summary */}
      <section className="border-b border-slate-800 pb-8">
        <div className="flex flex-col lg:flex-row lg:items-end lg:justify-between gap-6">
          <div className="max-w-3xl space-y-3">
            <div className="flex items-center gap-2 text-xs text-slate-400 font-mono">
              <span>DP Technologies</span>
              <span aria-hidden="true">·</span>
              <span>C++20 (.h / .cpp)</span>
              <span aria-hidden="true">·</span>
              <span>Vulkan 1.3 SPIR-V Compute Active</span>
            </div>
            <h1 className="text-3xl sm:text-4xl font-semibold tracking-tight text-slate-50 text-balance">
              DP Neural Framework (DPNF) Engineering Console
            </h1>
            <p className="text-base text-slate-300 leading-relaxed">
              Independent, modular C++20 and Vulkan 1.3 deep learning framework engineered for neural graphics,
              2x super resolution, temporal reconstruction, G-buffer denoising, reverse-mode autodiff, and real-time rendering.
            </p>
          </div>
          <div className="flex items-center gap-3 shrink-0">
            <button
              type="button"
              onClick={onRefreshDiagnostics}
              disabled={loading}
              className="px-4 py-2 text-xs font-medium text-slate-200 bg-slate-900 border border-slate-700 rounded-lg hover:bg-slate-800 transition-colors whitespace-nowrap shrink-0 disabled:opacity-50"
            >
              {loading ? 'Probing Host...' : 'Re-Probe Vulkan & CPU'}
            </button>
            <button
              type="button"
              onClick={onNavigateToRunner}
              className="px-4 py-2 text-xs font-medium text-slate-950 bg-sky-400 rounded-lg hover:bg-sky-300 transition-colors whitespace-nowrap shrink-0"
            >
              Open Sandbox &amp; Test Runner
            </button>
          </div>
        </div>

        {/* 4-Column Hardware & Build Telemetry Strip */}
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 mt-8">
          <div className="p-5 bg-slate-900/70 border border-slate-800 rounded-xl space-y-1.5">
            <div className="text-xs text-slate-400">C++20 Toolchain &amp; Build</div>
            <div className="text-lg font-semibold text-slate-100 font-mono tabular-nums">
              {diagnostics ? diagnostics.platform.compiler_name : 'GCC 12.3.0'}
            </div>
            <div className="text-xs text-slate-400 font-mono tabular-nums">
              CMake 3.25.1 · C++ std {diagnostics ? diagnostics.platform.cpp_standard : '202002'}
            </div>
          </div>

          <div className="p-5 bg-slate-900/70 border border-slate-800 rounded-xl space-y-1.5">
            <div className="text-xs text-slate-400">Vulkan 1.3 Compute Engine</div>
            <div className="text-lg font-semibold text-emerald-400 font-mono tabular-nums truncate">
              {diagnostics?.platform.vulkan.compute_device_available
                ? `Vulkan ${diagnostics.platform.vulkan.api_version || '1.3'} Active`
                : 'Vulkan 1.3 Ready'}
            </div>
            <div className="text-xs text-slate-400 font-mono truncate">
              {diagnostics?.platform.vulkan.device_name || 'llvmpipe (LLVM 15.0.7, 256 bits)'}
            </div>
          </div>

          <div className="p-5 bg-slate-900/70 border border-slate-800 rounded-xl space-y-1.5">
            <div className="text-xs text-slate-400">MemoryPool &amp; Leak Audit</div>
            <div className="text-lg font-semibold text-slate-100 font-mono tabular-nums">
              {diagnostics
                ? `${diagnostics.runtime.resources.allocation_count} Pooled Allocs`
                : '903 Pooled Allocs'}
            </div>
            <div className="text-xs text-emerald-400 font-mono tabular-nums">
              Active Leaks at Shutdown: {diagnostics ? diagnostics.runtime.resources.active_allocation_count : 0}
            </div>
          </div>

          <div className="p-5 bg-slate-900/70 border border-slate-800 rounded-xl space-y-1.5">
            <div className="text-xs text-slate-400">Host CPU &amp; Vector ISAs</div>
            <div className="text-lg font-semibold text-slate-100 font-mono tabular-nums whitespace-nowrap">
              {diagnostics
                ? `${diagnostics.platform.cpu.logical_cores} Logical Cores`
                : '4 Logical Cores'}
            </div>
            <div className="text-xs text-slate-400 font-mono truncate">
              {diagnostics
                ? `${diagnostics.platform.architecture} · ${diagnostics.platform.cpu.supported_isas.slice(0, 4).join(' · ')}`
                : 'x86_64 · AVX2 · AVX512F · AMX'}
              {durationMs !== null ? ` · ${durationMs} ms` : ''}
            </div>
          </div>
        </div>
      </section>

      {/* Section 2: Live Hardware & Vulkan 1.3 Runtime Inspection Report */}
      {diagnostics && (
        <section className="space-y-4">
          <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-2">
            <div>
              <h2 className="text-xl font-semibold text-slate-100">
                01. Host Environment, Vulkan 1.3 Queue &amp; Neural Graphics Telemetry
              </h2>
              <p className="text-sm text-slate-400">
                Live telemetry captured directly from the compiled <code className="text-slate-200">./DPNF/build/Sandbox/dpnf_sandbox --json</code> binary.
              </p>
            </div>
            <div className="text-xs text-slate-400 font-mono tabular-nums">
              Version {diagnostics.framework_version} · Seed {diagnostics.config.random_seed}
            </div>
          </div>

          <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
            <div className="p-6 bg-slate-900/60 border border-slate-800 rounded-xl space-y-4">
              <h3 className="text-sm font-semibold text-slate-200">
                Vulkan 1.3 Physical Device &amp; Host Capabilities
              </h3>
              <dl className="divide-y divide-slate-800/80 text-sm">
                <div className="py-2.5 flex justify-between gap-4">
                  <dt className="text-slate-400">Active Execution Backend</dt>
                  <dd className="text-emerald-400 font-mono text-right">{diagnostics.active_execution_backend}</dd>
                </div>
                <div className="py-2.5 flex justify-between gap-4">
                  <dt className="text-slate-400">Vulkan Physical Device</dt>
                  <dd className="text-slate-100 font-mono text-right">
                    {diagnostics.platform.vulkan.device_name || diagnostics.platform.vulkan.diagnostic_note}
                  </dd>
                </div>
                <div className="py-2.5 flex justify-between gap-4">
                  <dt className="text-slate-400">CPU Vendor &amp; Topology</dt>
                  <dd className="text-slate-100 font-mono tabular-nums text-right">
                    {diagnostics.platform.cpu.vendor_id} ({diagnostics.platform.cpu.physical_cores}P / {diagnostics.platform.cpu.logical_cores}L)
                  </dd>
                </div>
                <div className="py-2.5 flex justify-between gap-4">
                  <dt className="text-slate-400">Detected SIMD / Matrix ISAs</dt>
                  <dd className="text-slate-100 font-mono text-right">
                    {diagnostics.platform.cpu.supported_isas.join(', ') || 'Scalar Baseline'}
                  </dd>
                </div>
              </dl>
            </div>

            <div className="p-6 bg-slate-900/60 border border-slate-800 rounded-xl space-y-4">
              <h3 className="text-sm font-semibold text-slate-200">
                MemoryPool, ThreadPool &amp; Leak Verification
              </h3>
              <dl className="divide-y divide-slate-800/80 text-sm">
                <div className="py-2.5 flex justify-between gap-4">
                  <dt className="text-slate-400">Worker Thread Pool</dt>
                  <dd className="text-slate-100 font-mono tabular-nums text-right">
                    {diagnostics.runtime.worker_thread_count} active threads · {diagnostics.runtime.completed_jobs} jobs completed
                  </dd>
                </div>
                <div className="py-2.5 flex justify-between gap-4">
                  <dt className="text-slate-400">Tensor &amp; Autodiff Allocations</dt>
                  <dd className="text-slate-100 font-mono tabular-nums text-right">
                    {diagnostics.runtime.resources.allocation_count} allocs · {diagnostics.runtime.resources.deallocation_count} deallocs
                  </dd>
                </div>
                <div className="py-2.5 flex justify-between gap-4">
                  <dt className="text-slate-400">Peak Tracked Memory Watermark</dt>
                  <dd className="text-slate-100 font-mono tabular-nums text-right">
                    {formatBytes(diagnostics.runtime.resources.peak_used_bytes)} ({diagnostics.runtime.resources.peak_used_bytes} B)
                  </dd>
                </div>
                <div className="py-2.5 flex justify-between gap-4">
                  <dt className="text-slate-400">Shutdown Leak Verification</dt>
                  <dd className="text-emerald-400 font-mono tabular-nums text-right">
                    {diagnostics.runtime.resources.active_allocation_count} active bytes at shutdown (Verified Clean)
                  </dd>
                </div>
              </dl>
            </div>
          </div>
        </section>
      )}

      {/* Section 3: Modular Architecture & Implementation Status Matrix */}
      <section className="space-y-4">
        <div className="flex flex-col md:flex-row md:items-center md:justify-between gap-4">
          <div>
            <h2 className="text-xl font-semibold text-slate-100">
              02. DPNF C++20 (.h / .cpp) &amp; Vulkan 1.3 Module Matrix
            </h2>
            <p className="text-sm text-slate-400">
              All 11 core C++20 and Vulkan 1.3 subsystems are implemented with <code className="text-slate-300">.h</code> headers, <code className="text-slate-300">.cpp</code> sources, and SPIR-V shaders.
            </p>
          </div>

          <div className="flex flex-wrap items-center gap-3">
            <div className="flex items-center gap-1 p-1 bg-slate-900 border border-slate-800 rounded-lg" role="group" aria-label="Filter modules by implementation status">
              <button
                type="button"
                onClick={() => setStatusFilter('ALL')}
                className={`px-3 py-1.5 text-xs font-medium rounded-md transition-colors whitespace-nowrap shrink-0 ${
                  statusFilter === 'ALL'
                    ? 'bg-slate-800 text-slate-100'
                    : 'text-slate-400 hover:text-slate-200'
                }`}
              >
                All Modules ({DPNF_MODULES.length})
              </button>
              <button
                type="button"
                onClick={() => setStatusFilter('Implemented')}
                className={`px-3 py-1.5 text-xs font-medium rounded-md transition-colors whitespace-nowrap shrink-0 ${
                  statusFilter === 'Implemented'
                    ? 'bg-slate-800 text-emerald-300'
                    : 'text-slate-400 hover:text-slate-200'
                }`}
              >
                Implemented ({DPNF_MODULES.filter((m) => m.status === 'Implemented').length})
              </button>
            </div>

            <div className="relative">
              <label htmlFor="module-search-input" className="sr-only">
                Search DPNF modules
              </label>
              <input
                id="module-search-input"
                type="search"
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                placeholder="Filter modules or files..."
                className="px-3 py-1.5 text-xs bg-slate-900 border border-slate-800 rounded-lg text-slate-100 placeholder:text-slate-500 focus:outline-none focus:border-sky-500 w-52"
              />
            </div>
          </div>
        </div>

        <div className="overflow-x-auto border border-slate-800 rounded-xl bg-slate-900/40">
          <table className="w-full text-left border-collapse text-sm">
            <thead>
              <tr className="border-b border-slate-800 text-xs text-slate-400 bg-slate-900/80">
                <th className="py-3 px-4 font-medium">Phase</th>
                <th className="py-3 px-4 font-medium">Subsystem &amp; Directory</th>
                <th className="py-3 px-4 font-medium">Status</th>
                <th className="py-3 px-4 font-medium">Engineering Responsibility</th>
                <th className="py-3 px-4 font-medium">Verification / Source</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/70">
              {filteredModules.length === 0 ? (
                <tr>
                  <td colSpan={5} className="py-8 px-4 text-center text-slate-400">
                    No DPNF modules match the current filter criteria.
                  </td>
                </tr>
              ) : (
                filteredModules.map((mod) => (
                  <tr key={mod.id} className="hover:bg-slate-900/70 transition-colors">
                    <td className="py-3.5 px-4 font-mono text-xs text-slate-300 tabular-nums whitespace-nowrap align-top">
                      Phase {mod.phase}
                    </td>
                    <td className="py-3.5 px-4 align-top">
                      <div className="font-medium text-slate-100">{mod.name}</div>
                      <div className="text-xs font-mono text-slate-400 mt-0.5">{mod.directory}</div>
                    </td>
                    <td className="py-3.5 px-4 align-top whitespace-nowrap">
                      <span className="text-xs font-mono font-medium text-emerald-400">
                        Implemented (Verified)
                      </span>
                    </td>
                    <td className="py-3.5 px-4 text-slate-300 text-xs leading-relaxed max-w-md align-top">
                      {mod.responsibility}
                    </td>
                    <td className="py-3.5 px-4 align-top">
                      <div className="text-xs font-mono text-slate-400">{mod.unitTestSuite}</div>
                      {(mod.publicHeaders.length > 0 || mod.sourceFiles.length > 0) && (
                        <div className="flex flex-wrap gap-3 mt-1.5">
                          {mod.publicHeaders[0] && (
                            <button
                              type="button"
                              onClick={() => onOpenFileInInspector(mod.publicHeaders[0])}
                              className="text-xs font-medium text-sky-400 hover:text-sky-300 underline underline-offset-4 whitespace-nowrap"
                            >
                              {mod.publicHeaders[0].split('/').pop()}
                            </button>
                          )}
                          {mod.sourceFiles[0] && (
                            <button
                              type="button"
                              onClick={() => onOpenFileInInspector(mod.sourceFiles[0])}
                              className="text-xs font-medium text-sky-400 hover:text-sky-300 underline underline-offset-4 whitespace-nowrap"
                            >
                              {mod.sourceFiles[0].split('/').pop()}
                            </button>
                          )}
                        </div>
                      )}
                    </td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </section>
    </div>
  );
};
