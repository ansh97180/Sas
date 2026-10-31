import React, { useEffect, useState } from 'react';
import {
  DPNF_ERROR_REGISTRY,
  validateSandboxRunOptions,
  formatBytes,
  type SandboxRunOptions
} from '../utils/dpnfLogic';
import type { PlatformDiagnosticsPayload } from './OverviewView';

interface GTestCaseResult {
  name: string;
  status: string;
  result: string;
  time: string;
}

interface GTestSuiteResult {
  name: string;
  tests: number;
  failures: number;
  disabled: number;
  errors: number;
  time: string;
  testsuite: GTestCaseResult[];
}

export interface GTestReportPayload {
  tests: number;
  failures: number;
  disabled: number;
  errors: number;
  time: string;
  testsuites: GTestSuiteResult[];
}

interface SandboxTestRunnerViewProps {
  initialDiagnostics: PlatformDiagnosticsPayload | null;
}

export const SandboxTestRunnerView: React.FC<SandboxTestRunnerViewProps> = ({
  initialDiagnostics
}) => {
  const [options, setOptions] = useState<SandboxRunOptions>({
    logLevel: 'info',
    workerThreads: 4,
    devicePolicy: 'auto',
    simulateErrorCode: 'NONE'
  });

  const [sandboxRunning, setSandboxRunning] = useState(false);
  const [sandboxError, setSandboxError] = useState<string | null>(null);
  const [sandboxCommand, setSandboxCommand] = useState<string>(
    './DPNF/build/Sandbox/dpnf_sandbox --log-level info --threads 4 --device-policy auto'
  );
  const [sandboxDurationMs, setSandboxDurationMs] = useState<number | null>(null);
  const [sandboxReport, setSandboxReport] = useState<PlatformDiagnosticsPayload | null>(
    initialDiagnostics
  );
  const [sandboxStdout, setSandboxStdout] = useState<string>('');

  const [testsRunning, setTestsRunning] = useState(false);
  const [testsDurationMs, setTestsDurationMs] = useState<number | null>(null);
  const [gtestReport, setGtestReport] = useState<GTestReportPayload | null>(null);
  const [gtestStdout, setGtestStdout] = useState<string>('');
  const [testsError, setTestsError] = useState<string | null>(null);

  const handleRunSandbox = async (e: React.FormEvent) => {
    e.preventDefault();
    setSandboxError(null);

    const validation = validateSandboxRunOptions(options);
    if (!validation.valid) {
      setSandboxError(validation.error ?? 'Invalid configuration options.');
      return;
    }

    setSandboxRunning(true);
    try {
      const response = await fetch('/api/dpnf/sandbox/run', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(options)
      });
      const data = await response.json();
      if (!response.ok || !data.ok) {
        setSandboxError(data.error ?? 'Failed to execute dpnf_sandbox binary.');
      } else {
        setSandboxCommand(data.command);
        setSandboxDurationMs(data.durationMs);
        setSandboxReport(data.report);
        setSandboxStdout(data.stdout);
      }
    } catch (err: unknown) {
      setSandboxError(err instanceof Error ? err.message : String(err));
    } finally {
      setSandboxRunning(false);
    }
  };

  const handleRunGTestSuite = async () => {
    setTestsError(null);
    setTestsRunning(true);
    try {
      const response = await fetch('/api/dpnf/tests/run', {
        method: 'POST'
      });
      const data = await response.json();
      if (!response.ok || !data.ok) {
        setTestsError(data.error ?? 'Failed to execute dpnf_tests binary.');
      } else {
        setTestsDurationMs(data.durationMs);
        setGtestReport(data.gtestReport);
        setGtestStdout(data.stdout);
      }
    } catch (err: unknown) {
      setTestsError(err instanceof Error ? err.message : String(err));
    } finally {
      setTestsRunning(false);
    }
  };

  useEffect(() => {
    handleRunGTestSuite();
  }, []);

  return (
    <div className="space-y-10">
      <div className="border-b border-slate-800 pb-6">
        <div className="flex items-center gap-2 text-xs text-slate-400 font-mono mb-2">
          <span>Native C++20 Binaries</span>
          <span aria-hidden="true">·</span>
          <span>dpnf_sandbox &amp; dpnf_tests</span>
        </div>
        <h1 className="text-2xl sm:text-3xl font-semibold text-slate-50">
          Live C++20 Sandbox &amp; GoogleTest Verification Runner
        </h1>
        <p className="text-sm text-slate-400 mt-1 max-w-3xl">
          Configure <code className="text-slate-200">dpnf::config::FrameworkConfig</code> parameters and execute the compiled C++20{' '}
          <code className="text-slate-200">dpnf_sandbox</code> and <code className="text-slate-200">dpnf_tests</code> binaries directly on the host.
        </p>
      </div>

      {/* Two-Column Split: Left = Sandbox Runner, Right = GoogleTest Suite */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-8 items-start">
        {/* Left Column: dpnf_sandbox Interactive Runner */}
        <section className="p-6 bg-slate-900/60 border border-slate-800 rounded-xl space-y-6">
          <div className="flex items-center justify-between gap-4 border-b border-slate-800 pb-4">
            <div>
              <h2 className="text-lg font-semibold text-slate-100">
                01. Execute dpnf_sandbox Executable
              </h2>
              <p className="text-xs text-slate-400 mt-0.5">
                Tests CoreSystem lifecycle, ThreadPool dispatch, ResourceTracker, and Logger sinks.
              </p>
            </div>
            {sandboxDurationMs !== null && (
              <span className="text-xs font-mono text-emerald-400 tabular-nums shrink-0">
                Executed in {sandboxDurationMs} ms
              </span>
            )}
          </div>

          <form onSubmit={handleRunSandbox} className="space-y-4">
            <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
              <div>
                <label htmlFor="sandbox-log-level" className="block text-xs font-medium text-slate-300 mb-1.5">
                  Log Severity Filter (LogLevel)
                </label>
                <select
                  id="sandbox-log-level"
                  value={options.logLevel}
                  onChange={(e) =>
                    setOptions((prev) => ({
                      ...prev,
                      logLevel: e.target.value as SandboxRunOptions['logLevel']
                    }))
                  }
                  className="w-full px-3 py-2 text-xs bg-slate-950 border border-slate-800 rounded-lg text-slate-100 focus:outline-none focus:border-sky-500"
                >
                  <option value="trace">trace (Verbose diagnostics)</option>
                  <option value="debug">debug (Include ThreadPool checksum)</option>
                  <option value="info">info (Standard lifecycle)</option>
                  <option value="warn">warn (Warnings &amp; errors only)</option>
                  <option value="error">error (Errors only)</option>
                </select>
              </div>

              <div>
                <label htmlFor="sandbox-threads" className="block text-xs font-medium text-slate-300 mb-1.5">
                  Worker Thread Pool Count (0 = Auto)
                </label>
                <input
                  id="sandbox-threads"
                  type="number"
                  value={options.workerThreads}
                  onChange={(e) =>
                    setOptions((prev) => ({
                      ...prev,
                      workerThreads: Number(e.target.value)
                    }))
                  }
                  className="w-full px-3 py-2 text-xs font-mono tabular-nums bg-slate-950 border border-slate-800 rounded-lg text-slate-100 focus:outline-none focus:border-sky-500"
                />
              </div>

              <div>
                <label htmlFor="sandbox-device-policy" className="block text-xs font-medium text-slate-300 mb-1.5">
                  Device Selection Policy
                </label>
                <select
                  id="sandbox-device-policy"
                  value={options.devicePolicy}
                  onChange={(e) =>
                    setOptions((prev) => ({
                      ...prev,
                      devicePolicy: e.target.value as SandboxRunOptions['devicePolicy']
                    }))
                  }
                  className="w-full px-3 py-2 text-xs bg-slate-950 border border-slate-800 rounded-lg text-slate-100 focus:outline-none focus:border-sky-500"
                >
                  <option value="auto">Auto (Probe Vulkan, fallback to CPU)</option>
                  <option value="cpu_only">CPUOnly (Explicit CPU execution)</option>
                  <option value="prefer_gpu">PreferGPU (Warn on missing Vulkan ICD)</option>
                </select>
              </div>

              <div>
                <label htmlFor="sandbox-error-sim" className="block text-xs font-medium text-slate-300 mb-1.5">
                  Diagnostic Error Verification
                </label>
                <select
                  id="sandbox-error-sim"
                  value={options.simulateErrorCode}
                  onChange={(e) =>
                    setOptions((prev) => ({
                      ...prev,
                      simulateErrorCode: e.target.value
                    }))
                  }
                  className="w-full px-3 py-2 text-xs font-mono bg-slate-950 border border-slate-800 rounded-lg text-slate-100 focus:outline-none focus:border-sky-500"
                >
                  <option value="NONE">None (Nominal Execution)</option>
                  {DPNF_ERROR_REGISTRY.map((err) => (
                    <option key={err.name} value={err.name}>
                      {err.name}
                    </option>
                  ))}
                </select>
              </div>
            </div>

            {sandboxError && (
              <div role="alert" className="py-2.5 px-3 border border-rose-800/80 rounded-lg bg-rose-950/40 text-xs text-rose-200 font-mono">
                Validation Error: {sandboxError}
              </div>
            )}

            <div className="flex items-center justify-between gap-4 pt-2">
              <code className="text-xs text-slate-400 font-mono truncate">{sandboxCommand}</code>
              <button
                type="submit"
                disabled={sandboxRunning}
                className="px-4 py-2 text-xs font-medium text-slate-950 bg-sky-400 rounded-lg hover:bg-sky-300 transition-colors whitespace-nowrap shrink-0 disabled:opacity-50"
              >
                {sandboxRunning ? 'Running C++ Binary...' : 'Run dpnf_sandbox'}
              </button>
            </div>
          </form>

          {/* Captured Ring-Buffer Logs from C++ Logger */}
          {sandboxReport && (
            <div className="space-y-3 pt-4 border-t border-slate-800">
              <div className="flex flex-wrap items-center justify-between gap-2 text-xs text-slate-400">
                <span>Captured C++ Logger Ring Buffer ({sandboxReport.logs.length} entries)</span>
                <span className="font-mono tabular-nums">
                  Backend: {sandboxReport.active_execution_backend} · Workers: {sandboxReport.runtime.worker_thread_count} · Peak RAM: {formatBytes(sandboxReport.runtime.resources.peak_used_bytes)}
                </span>
              </div>
              <div className="bg-slate-950/80 py-3 px-3.5 max-h-64 overflow-y-auto space-y-2 font-mono text-xs border-t border-b border-slate-800">
                {sandboxReport.logs.map((entry) => (
                  <div key={entry.sequence_id} className="leading-relaxed break-words">
                    <span className="text-slate-500">[{entry.timestamp}]</span>{' '}
                    <span
                      className={
                        entry.level === 'ERROR' || entry.level === 'FATAL'
                          ? 'text-rose-400 font-semibold'
                          : entry.level === 'WARN'
                          ? 'text-amber-400 font-semibold'
                          : entry.level === 'DEBUG'
                          ? 'text-sky-400'
                          : 'text-emerald-400'
                      }
                    >
                      [{entry.level}]
                    </span>{' '}
                    <span className="text-slate-300">[{entry.subsystem}]</span>{' '}
                    <span className="text-slate-100">{entry.message}</span>
                  </div>
                ))}
              </div>
            </div>
          )}

          {sandboxStdout && (
            <details className="text-xs">
              <summary className="cursor-pointer text-slate-400 hover:text-slate-200 font-medium">
                View Raw C++ Standard Output Report
              </summary>
              <pre className="mt-2 p-3 bg-slate-950 border-t border-b border-slate-800 text-slate-300 font-mono overflow-x-auto text-[11px] leading-relaxed">
                {sandboxStdout}
              </pre>
            </details>
          )}
        </section>

        {/* Right Column: GoogleTest Suite Runner (dpnf_tests) */}
        <section className="p-6 bg-slate-900/60 border border-slate-800 rounded-xl space-y-6">
          <div className="flex items-center justify-between gap-4 border-b border-slate-800 pb-4">
            <div>
              <h2 className="text-lg font-semibold text-slate-100">
                02. GoogleTest Unit &amp; Integration Suite (dpnf_tests)
              </h2>
              <p className="text-xs text-slate-400 mt-0.5">
                Executes compiled GoogleTest binary verifying Error, Logger, Config, Platform, ResourceTracker, ThreadPool, and CoreSystem.
              </p>
            </div>
            <button
              type="button"
              onClick={handleRunGTestSuite}
              disabled={testsRunning}
              className="px-4 py-2 text-xs font-medium text-slate-100 bg-emerald-600 rounded-lg hover:bg-emerald-500 transition-colors whitespace-nowrap shrink-0 disabled:opacity-50"
            >
              {testsRunning ? 'Executing GoogleTest...' : 'Run GoogleTest Suite'}
            </button>
          </div>

          {testsError && (
            <div role="alert" className="py-2.5 px-3 bg-rose-950/60 text-xs text-rose-200 font-mono">
              {testsError}
            </div>
          )}

          {!gtestReport ? (
            <div className="py-8 text-center space-y-2 border-b border-slate-800">
              <div className="text-sm font-medium text-slate-200">
                Executing C++20 GoogleTest Binary...
              </div>
              <p className="text-xs text-slate-400 max-w-md mx-auto">
                Running <code className="text-slate-300">./DPNF/build/Tests/dpnf_tests</code> to verify all Phase 0 unit assertions.
              </p>
            </div>
          ) : (
            <div className="space-y-4">
              {/* Hairline-divided KPI strip (no nested cards-within-cards) */}
              <div className="grid grid-cols-3 divide-x divide-slate-800 border-t border-b border-slate-800 py-3">
                <div className="pr-4">
                  <div className="text-xs text-slate-400">Total C++ Tests</div>
                  <div className="text-base font-semibold text-slate-100 font-mono tabular-nums mt-0.5">
                    {gtestReport.tests} Assertions
                  </div>
                </div>
                <div className="px-4">
                  <div className="text-xs text-slate-400">Pass / Fail Status</div>
                  <div className="text-base font-semibold text-emerald-400 font-mono tabular-nums mt-0.5">
                    {gtestReport.tests - gtestReport.failures} Passed · {gtestReport.failures} Failed
                  </div>
                </div>
                <div className="pl-4">
                  <div className="text-xs text-slate-400">Execution Time</div>
                  <div className="text-base font-semibold text-slate-100 font-mono tabular-nums mt-0.5">
                    {testsDurationMs ?? gtestReport.time} ms
                  </div>
                </div>
              </div>

              <div className="overflow-x-auto">
                <table className="w-full text-left border-collapse text-xs">
                  <thead>
                    <tr className="border-b border-slate-800 text-slate-400">
                      <th className="py-2.5 pr-3 font-medium">Test Suite &amp; Case</th>
                      <th className="py-2.5 px-3 font-medium whitespace-nowrap">Result</th>
                      <th className="py-2.5 pl-3 font-medium text-right whitespace-nowrap">Time</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-slate-800/70 font-mono">
                    {gtestReport.testsuites.flatMap((suite) =>
                      suite.testsuite.map((tc) => (
                        <tr key={`${suite.name}.${tc.name}`} className="hover:bg-slate-900/80">
                          <td className="py-2.5 pr-3">
                            <div className="text-slate-100 font-medium break-words">{tc.name}</div>
                            <div className="text-[11px] text-slate-400">{suite.name}</div>
                          </td>
                          <td className="py-2.5 px-3 whitespace-nowrap align-top">
                            {tc.result === 'COMPLETED' ? (
                              <span className="text-emerald-400 font-medium">PASSED</span>
                            ) : (
                              <span className="text-rose-400 font-medium">{tc.result}</span>
                            )}
                          </td>
                          <td className="py-2.5 pl-3 text-right text-slate-400 tabular-nums whitespace-nowrap align-top">
                            {tc.time}
                          </td>
                        </tr>
                      ))
                    )}
                  </tbody>
                </table>
              </div>

              {gtestStdout && (
                <details className="text-xs pt-2 border-t border-slate-800">
                  <summary className="cursor-pointer text-slate-400 hover:text-slate-200 font-medium">
                    View Raw GoogleTest Console Output
                  </summary>
                  <pre className="mt-2 p-3 bg-slate-950 border-t border-b border-slate-800 text-slate-300 font-mono overflow-x-auto text-[11px] leading-relaxed">
                    {gtestStdout}
                  </pre>
                </details>
              )}
            </div>
          )}
        </section>
      </div>
    </div>
  );
};

