import test from 'node:test';
import assert from 'node:assert/strict';
import {
  DPNF_ERROR_REGISTRY,
  DPNF_MODULES,
  validateSandboxRunOptions,
  buildSandboxCliArgs,
  filterDpnfModules,
  formatBytes
} from './dpnfLogic';

test('DPNF_ERROR_REGISTRY contains all 13 canonical DPNF_ERROR_* codes', () => {
  assert.equal(DPNF_ERROR_REGISTRY.length, 13);
  const names = DPNF_ERROR_REGISTRY.map((e) => e.name);
  assert.ok(names.includes('DPNF_ERROR_INVALID_ARGUMENT'));
  assert.ok(names.includes('DPNF_ERROR_OUT_OF_MEMORY'));
  assert.ok(names.includes('DPNF_ERROR_INVALID_TENSOR'));
  assert.ok(names.includes('DPNF_ERROR_TRAINING_FAILED'));
});

test('DPNF_MODULES strictly separates Implemented Phase 0 modules from Planned Phases 1-11', () => {
  const implemented = DPNF_MODULES.filter((m) => m.status === 'Implemented');
  const planned = DPNF_MODULES.filter((m) => m.status === 'Planned');
  assert.equal(implemented.length, 5);
  assert.ok(implemented.every((m) => m.phase === 0));
  assert.ok(planned.length >= 7);
  assert.ok(planned.every((m) => m.phase >= 1));
});

test('validateSandboxRunOptions validates thread count, log level, and error codes', () => {
  assert.deepEqual(
    validateSandboxRunOptions({
      logLevel: 'info',
      workerThreads: 4,
      devicePolicy: 'auto',
      simulateErrorCode: 'NONE'
    }),
    { valid: true }
  );

  const invalidThreads = validateSandboxRunOptions({
    logLevel: 'info',
    workerThreads: 512,
    devicePolicy: 'auto',
    simulateErrorCode: 'NONE'
  });
  assert.equal(invalidThreads.valid, false);

  const invalidError = validateSandboxRunOptions({
    logLevel: 'debug',
    workerThreads: 2,
    devicePolicy: 'cpu_only',
    simulateErrorCode: 'DPNF_ERROR_FAKE_CODE'
  });
  assert.equal(invalidError.valid, false);
});

test('buildSandboxCliArgs constructs deterministic CLI arguments for dpnf_sandbox', () => {
  const args = buildSandboxCliArgs(
    {
      logLevel: 'debug',
      workerThreads: 4,
      devicePolicy: 'prefer_gpu',
      simulateErrorCode: 'DPNF_ERROR_OUT_OF_MEMORY'
    },
    true
  );
  assert.deepEqual(args, [
    '--json',
    '--log-level',
    'debug',
    '--threads',
    '4',
    '--device-policy',
    'prefer_gpu',
    '--simulate-error',
    'DPNF_ERROR_OUT_OF_MEMORY'
  ]);
});

test('filterDpnfModules filters by status and search query accurately', () => {
  const phase0Only = filterDpnfModules(DPNF_MODULES, 'Implemented', '');
  assert.equal(phase0Only.length, 5);

  const vulkanSearch = filterDpnfModules(DPNF_MODULES, 'ALL', 'vulkan');
  assert.ok(vulkanSearch.length >= 2);
});

test('formatBytes formats byte counts cleanly with binary units', () => {
  assert.equal(formatBytes(0), '0 B');
  assert.equal(formatBytes(1024), '1.00 KiB');
  assert.equal(formatBytes(1048576), '1.00 MiB');
});
