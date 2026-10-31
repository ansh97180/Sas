import React, { useEffect, useState } from 'react';
import { formatBytes } from '../utils/dpnfLogic';

export interface InspectableFileMeta {
  path: string;
  category: string;
  description: string;
  sizeBytes: number;
  lineCount: number;
}

interface SourceInspectorViewProps {
  selectedFilePath: string;
  onSelectFilePath: (path: string) => void;
}

export const SourceInspectorView: React.FC<SourceInspectorViewProps> = ({
  selectedFilePath,
  onSelectFilePath
}) => {
  const [files, setFiles] = useState<InspectableFileMeta[]>([]);
  const [fileContent, setFileContent] = useState<string>('');
  const [fileDescription, setFileDescription] = useState<string>('');
  const [loadingFile, setLoadingFile] = useState<boolean>(false);
  const [filterQuery, setFilterQuery] = useState<string>('');

  useEffect(() => {
    fetch('/api/dpnf/files')
      .then((res) => res.json())
      .then((data) => {
        if (data.ok && Array.isArray(data.files)) {
          setFiles(data.files);
        }
      })
      .catch(() => {});
  }, []);

  useEffect(() => {
    if (!selectedFilePath) return;
    setLoadingFile(true);
    fetch(`/api/dpnf/file?path=${encodeURIComponent(selectedFilePath)}`)
      .then((res) => res.json())
      .then((data) => {
        if (data.ok && data.file) {
          setFileContent(data.file.content);
          setFileDescription(data.file.description);
        }
      })
      .catch(() => {})
      .finally(() => setLoadingFile(false));
  }, [selectedFilePath]);

  const filteredFiles = files.filter((f) => {
    const q = filterQuery.trim().toLowerCase();
    if (!q) return true;
    return (
      f.path.toLowerCase().includes(q) ||
      f.category.toLowerCase().includes(q) ||
      f.description.toLowerCase().includes(q)
    );
  });

  return (
    <div className="space-y-6">
      <div className="border-b border-slate-800 pb-6">
        <div className="flex items-center gap-2 text-xs text-slate-400 font-mono mb-2">
          <span>DPNF Source Tree</span>
          <span aria-hidden="true">·</span>
          <span>Modern C++20 &amp; CMake</span>
        </div>
        <h1 className="text-2xl sm:text-3xl font-semibold text-slate-50">
          C++20 Source, Header &amp; Build Configuration Inspector
        </h1>
        <p className="text-sm text-slate-400 mt-1 max-w-3xl">
          Inspect the live C++20 headers (<code className="text-slate-200">.hpp</code>), implementation units (<code className="text-slate-200">.cpp</code>), CMake build scripts, and unit tests in the <code className="text-slate-200">DPNF/</code> repository.
        </p>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6 items-start">
        {/* Left Sidebar: File Manifest */}
        <aside className="lg:col-span-4 bg-slate-900/60 border border-slate-800 rounded-xl p-4 space-y-3">
          <div>
            <label htmlFor="source-tree-search" className="sr-only">
              Filter C++20 source files
            </label>
            <input
              id="source-tree-search"
              type="search"
              value={filterQuery}
              onChange={(e) => setFilterQuery(e.target.value)}
              placeholder="Search C++ headers or .cpp files..."
              className="w-full px-3 py-2 text-xs bg-slate-950 border border-slate-800 rounded-lg text-slate-100 placeholder:text-slate-500 focus:outline-none focus:border-sky-500"
            />
          </div>

          <div className="max-h-[540px] overflow-y-auto divide-y divide-slate-800/60">
            {filteredFiles.map((file) => {
              const isSelected = file.path === selectedFilePath;
              return (
                <button
                  key={file.path}
                  type="button"
                  onClick={() => onSelectFilePath(file.path)}
                  className={`w-full text-left py-2.5 px-3 rounded-lg transition-colors ${
                    isSelected
                      ? 'bg-sky-500/15 text-sky-200'
                      : 'text-slate-300 hover:bg-slate-800/60'
                  }`}
                >
                  <div className="text-xs font-mono font-medium truncate">{file.path}</div>
                  <div className="flex items-center gap-2 text-[11px] text-slate-400 mt-0.5 font-mono tabular-nums">
                    <span>{file.category}</span>
                    <span aria-hidden="true">·</span>
                    <span>{file.lineCount} lines</span>
                    <span aria-hidden="true">·</span>
                    <span>{formatBytes(file.sizeBytes)}</span>
                  </div>
                </button>
              );
            })}
          </div>
        </aside>

        {/* Right Code Viewer */}
        <div className="lg:col-span-8 bg-slate-900/60 border border-slate-800 rounded-xl overflow-hidden">
          <div className="px-5 py-3.5 border-b border-slate-800 bg-slate-950/60 flex flex-col sm:flex-row sm:items-center sm:justify-between gap-2">
            <div>
              <div className="text-sm font-mono font-semibold text-slate-100">{selectedFilePath}</div>
              <div className="text-xs text-slate-400">{fileDescription}</div>
            </div>
            <div className="text-xs font-mono text-slate-400 tabular-nums shrink-0">
              {fileContent ? `${fileContent.split('\n').length} lines` : ''}
            </div>
          </div>

          <div className="p-5 bg-slate-950 overflow-x-auto max-h-[600px] overflow-y-auto">
            {loadingFile ? (
              <div className="py-12 text-center text-xs text-slate-400 font-mono">
                Loading {selectedFilePath}...
              </div>
            ) : (
              <pre className="text-xs font-mono text-slate-200 leading-relaxed">
                {fileContent}
              </pre>
            )}
          </div>
        </div>
      </div>
    </div>
  );
};
