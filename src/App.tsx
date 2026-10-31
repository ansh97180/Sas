/**
 * @license
 * SPDX-License-Identifier: Apache-2.0
 */

import React, { useEffect, useState } from 'react';
import { OverviewView, type PlatformDiagnosticsPayload } from './components/OverviewView';
import { SandboxTestRunnerView } from './components/SandboxTestRunnerView';
import { SourceInspectorView } from './components/SourceInspectorView';
import { ErrorRegistryView } from './components/ErrorRegistryView';

type ActiveTab = 'overview' | 'runner' | 'source' | 'errors';

export default function App() {
  const [activeTab, setActiveTab] = useState<ActiveTab>('overview');
  const [diagnostics, setDiagnostics] = useState<PlatformDiagnosticsPayload | null>(null);
  const [durationMs, setDurationMs] = useState<number | null>(null);
  const [loadingDiagnostics, setLoadingDiagnostics] = useState<boolean>(true);
  const [selectedFilePath, setSelectedFilePath] = useState<string>(
    'DPNF/GPU/include/dpnf/gpu/VulkanBackend.h'
  );

  const fetchDiagnostics = async () => {
    setLoadingDiagnostics(true);
    try {
      const res = await fetch('/api/dpnf/diagnostics');
      const data = await res.json();
      if (data.ok && data.report) {
        setDiagnostics(data.report);
        setDurationMs(data.durationMs);
      }
    } catch {
      // Keep previous diagnostics if transient error
    } finally {
      setLoadingDiagnostics(false);
    }
  };

  useEffect(() => {
    fetchDiagnostics();
  }, []);

  const handleOpenFileInInspector = (filePath: string) => {
    setSelectedFilePath(filePath);
    setActiveTab('source');
  };

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 flex flex-col">
      {/* Strict 3-Zone Top Bar Contract */}
      <header className="flex items-center justify-between gap-8 px-6 py-4 border-b border-slate-800 bg-slate-950">
        {/* Zone 1: Single text element wordmark */}
        <a
          href="#overview"
          onClick={(e) => {
            e.preventDefault();
            setActiveTab('overview');
          }}
          className="text-lg font-bold tracking-tight text-slate-50 font-display whitespace-nowrap shrink-0"
        >
          DP Neural Framework
        </a>

        {/* Zone 2: 4 concise single-line text navigation links */}
        <nav aria-label="Primary Navigation" className="flex items-center gap-6 text-sm font-medium text-slate-300 overflow-x-auto">
          <button
            type="button"
            onClick={() => setActiveTab('overview')}
            className={`hover:text-slate-50 transition-colors whitespace-nowrap shrink-0 pb-0.5 ${
              activeTab === 'overview' ? 'text-sky-400 border-b-2 border-sky-400' : ''
            }`}
          >
            Architecture &amp; Vulkan
          </button>
          <button
            type="button"
            onClick={() => setActiveTab('runner')}
            className={`hover:text-slate-50 transition-colors whitespace-nowrap shrink-0 pb-0.5 ${
              activeTab === 'runner' ? 'text-sky-400 border-b-2 border-sky-400' : ''
            }`}
          >
            Sandbox &amp; Tests
          </button>
          <button
            type="button"
            onClick={() => setActiveTab('source')}
            className={`hover:text-slate-50 transition-colors whitespace-nowrap shrink-0 pb-0.5 ${
              activeTab === 'source' ? 'text-sky-400 border-b-2 border-sky-400' : ''
            }`}
          >
            C++20 Source (.h / .cpp)
          </button>
          <button
            type="button"
            onClick={() => setActiveTab('errors')}
            className={`hover:text-slate-50 transition-colors whitespace-nowrap shrink-0 pb-0.5 ${
              activeTab === 'errors' ? 'text-sky-400 border-b-2 border-sky-400' : ''
            }`}
          >
            Error Registry
          </button>
        </nav>

        {/* Zone 3: 1 primary action */}
        <div className="flex items-center shrink-0">
          <button
            type="button"
            onClick={() => setActiveTab('runner')}
            className="px-4 py-2 text-xs font-medium text-slate-950 bg-sky-400 rounded-lg hover:bg-sky-300 transition-colors whitespace-nowrap shrink-0"
          >
            Run C++ Verification
          </button>
        </div>
      </header>

      {/* Main Content Container */}
      <main className="flex-1 max-w-7xl w-full mx-auto px-6 py-8">
        {activeTab === 'overview' && (
          <OverviewView
            diagnostics={diagnostics}
            durationMs={durationMs}
            loading={loadingDiagnostics}
            onRefreshDiagnostics={fetchDiagnostics}
            onOpenFileInInspector={handleOpenFileInInspector}
            onNavigateToRunner={() => setActiveTab('runner')}
          />
        )}

        {activeTab === 'runner' && (
          <SandboxTestRunnerView initialDiagnostics={diagnostics} />
        )}

        {activeTab === 'source' && (
          <SourceInspectorView
            selectedFilePath={selectedFilePath}
            onSelectFilePath={setSelectedFilePath}
          />
        )}

        {activeTab === 'errors' && <ErrorRegistryView />}
      </main>

      {/* Quiet Footer */}
      <footer className="border-t border-slate-800/80 px-6 py-5 text-xs text-slate-400">
        <div className="max-w-7xl mx-auto flex flex-col sm:flex-row sm:items-center sm:justify-between gap-2">
          <div>DP Neural Framework (DPNF) · DP Technologies · Apache-2.0 License</div>
          <div className="font-mono tabular-nums">
            C++20 (.h / .cpp) · Vulkan 1.3 SPIR-V Compute · Neural Graphics · GoogleTest
          </div>
        </div>
      </footer>
    </div>
  );
}
