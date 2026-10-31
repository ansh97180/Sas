import React, { useState } from 'react';
import { DPNF_ERROR_REGISTRY, type DpnfErrorSpec } from '../utils/dpnfLogic';

export const ErrorRegistryView: React.FC = () => {
  const [selectedError, setSelectedError] = useState<DpnfErrorSpec>(DPNF_ERROR_REGISTRY[0]);
  const [verifying, setVerifying] = useState<boolean>(false);
  const [verifiedOutput, setVerifiedOutput] = useState<string>('');
  const [verifiedDurationMs, setVerifiedDurationMs] = useState<number | null>(null);

  const handleVerifyErrorInCpp = async (errSpec: DpnfErrorSpec) => {
    setSelectedError(errSpec);
    setVerifying(true);
    try {
      const response = await fetch('/api/dpnf/sandbox/run', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          logLevel: 'info',
          workerThreads: 2,
          devicePolicy: 'auto',
          simulateErrorCode: errSpec.name
        })
      });
      const data = await response.json();
      if (data.ok && data.report) {
        setVerifiedDurationMs(data.durationMs);
        const errorLogs = (data.report.logs as Array<{ level: string; message: string }>)
          .filter((l) => l.level === 'ERROR')
          .map((l) => l.message)
          .join('\n');
        setVerifiedOutput(errorLogs || data.stdout);
      }
    } catch (err: unknown) {
      setVerifiedOutput(err instanceof Error ? err.message : String(err));
    } finally {
      setVerifying(false);
    }
  };

  return (
    <div className="space-y-8">
      <div className="border-b border-slate-800 pb-6">
        <div className="flex items-center gap-2 text-xs text-slate-400 font-mono mb-2">
          <span>Section 5 Specification</span>
          <span aria-hidden="true">·</span>
          <span>dpnf::core::ErrorCode &amp; Result&lt;T&gt;</span>
        </div>
        <h1 className="text-2xl sm:text-3xl font-semibold text-slate-50">
          Structured Error Registry &amp; Live C++ Diagnostic Verifier
        </h1>
        <p className="text-sm text-slate-400 mt-1 max-w-3xl">
          Every DPNF subsystem uses <code className="text-slate-200">dpnf::core::Error</code> and{' '}
          <code className="text-slate-200">dpnf::core::Result&lt;T&gt;</code>. Select any of the 13 canonical{' '}
          <code className="text-slate-200">DPNF_ERROR_*</code> codes below to verify its C++ formatting and remediation output.
        </p>
      </div>

      {/* Live C++ Verification Panel */}
      <section className="p-6 bg-slate-900/60 border border-slate-800 rounded-xl space-y-5">
        <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4 border-b border-slate-800 pb-4">
          <div>
            <div className="text-xs font-mono text-sky-400">
              Code #{selectedError.code} · {selectedError.subsystem}
            </div>
            <h2 className="text-lg font-semibold text-slate-100 font-mono mt-0.5">
              {selectedError.name}
            </h2>
          </div>
          <button
            type="button"
            onClick={() => handleVerifyErrorInCpp(selectedError)}
            disabled={verifying}
            className="px-4 py-2 text-xs font-medium text-slate-950 bg-sky-400 rounded-lg hover:bg-sky-300 transition-colors whitespace-nowrap shrink-0 disabled:opacity-50"
          >
            {verifying ? 'Verifying in C++...' : `Trigger ${selectedError.name} in C++`}
          </button>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-6 text-xs md:divide-x md:divide-slate-800">
          <div className="space-y-1.5">
            <div className="text-slate-400 font-medium">Trigger Preconditions</div>
            <p className="text-slate-200 leading-relaxed">{selectedError.triggerCondition}</p>
          </div>
          <div className="space-y-1.5 md:pl-6">
            <div className="text-slate-400 font-medium">Documented Remediation Hint</div>
            <p className="text-slate-200 leading-relaxed">{selectedError.remediation}</p>
          </div>
        </div>

        {verifiedOutput && (
          <div className="pt-4 border-t border-slate-800 space-y-1.5">
            <div className="flex items-center justify-between text-xs text-slate-400">
              <span>Live C++ <code className="text-slate-300">Error::format()</code> Output</span>
              {verifiedDurationMs !== null && (
                <span className="font-mono tabular-nums text-emerald-400">{verifiedDurationMs} ms</span>
              )}
            </div>
            <pre className="p-3 bg-slate-950 text-xs font-mono text-rose-300 whitespace-pre-wrap break-words rounded-lg">
              {verifiedOutput}
            </pre>
          </div>
        )}
      </section>

      {/* Complete 13 Error Categories Table */}
      <div className="overflow-x-auto border border-slate-800 rounded-xl bg-slate-900/40">
        <table className="w-full text-left border-collapse text-sm">
          <thead>
            <tr className="border-b border-slate-800 bg-slate-900/80 text-xs text-slate-400">
              <th className="py-3 px-4 font-medium">ID</th>
              <th className="py-3 px-4 font-medium">Canonical Error Code</th>
              <th className="py-3 px-4 font-medium">Owning Subsystem</th>
              <th className="py-3 px-4 font-medium">Diagnostic Meaning &amp; Remediation</th>
              <th className="py-3 px-4 font-medium text-right">Action</th>
            </tr>
          </thead>
          <tbody className="divide-y divide-slate-800/70">
            {DPNF_ERROR_REGISTRY.map((err) => (
              <tr
                key={err.name}
                className={`hover:bg-slate-900/70 transition-colors ${
                  selectedError.name === err.name ? 'bg-sky-500/10' : ''
                }`}
              >
                <td className="py-3 px-4 font-mono text-xs text-slate-400 tabular-nums align-top">
                  {err.code}
                </td>
                <td className="py-3 px-4 font-mono text-xs font-medium text-slate-100 align-top whitespace-nowrap">
                  {err.name}
                </td>
                <td className="py-3 px-4 text-xs text-slate-300 align-top whitespace-nowrap">
                  {err.subsystem}
                </td>
                <td className="py-3 px-4 text-xs text-slate-300 leading-relaxed max-w-lg align-top">
                  <div>{err.description}</div>
                  <div className="text-slate-400 mt-1">Remediation: {err.remediation}</div>
                </td>
                <td className="py-3 px-4 text-right align-top whitespace-nowrap">
                  <button
                    type="button"
                    onClick={() => handleVerifyErrorInCpp(err)}
                    className="px-3 py-1.5 text-xs font-medium text-slate-200 bg-slate-800 hover:bg-slate-700 rounded-md transition-colors whitespace-nowrap"
                  >
                    Verify in C++
                  </button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
};
