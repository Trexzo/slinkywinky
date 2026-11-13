# R198 — Actual compiled native manager fault and lifecycle acceptance

Date: 2026-11-13. Repo: Trexzo/slinkywinky, PR #2,
branch integration/openslinky-fd9c592-reference.
This is separate from PR #1's Minecraft runtime reconstruction.

## Evidence boundary

R198 links the **actual corrected 66-entry module_manager.cpp.obj**
from the independently compiled pinned public OpenSlinky source into
an independently authored, isolated Windows x64 C++ test harness.

The compiled public reference source remains pinned at
fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4.
Compiled-object SHA-256 verification is enforced by the new
tools/run_r198_native_manager_resilience.ps1:

- Corrected 66-entry manager COFF:
  632d706ec121d4809a744876f4006c0e4ece19d7af0ad4994def704a77e516ae.
- Unmodified 67-entry manager COFF:
  40573226782654580a89504ecbb7ccb8b0217f832cb4d1fa47b771f277c78f80.

The harness does NOT load/inject full slinky.dll, start Minecraft,
create a JVM/JVMTI runtime, call actual original gameplay callbacks,
install hooks, or access live multiplayer game state.

The native object itself is not committed to GitHub; it stays on the
connected, authorized Windows machine. All module/event data in the
two new resilience tests is first-party synthetic fixture data.

## Allocator failure matrix

A first-party libc_calloc shim now supports deterministic,
one-shot failures at an exact allocation-attempt index.
It first measures the original compiled manager's number of
allocations in an ordinary successful constructor, then
injects a failure at **each of 38 allocation positions**.

For every one of the 38 failure positions:

1. Fresh compiled manager constructor returns error 0x3ed.
2. The partially initialized manager is destroyed using its actual
   compiled destructor.
3. The failure path's successful allocations and frees balance.
4. A subsequent fresh constructor succeeds with the correct
   66-entry compacted module order.
5. The recovered manager is destroyed with balanced allocations.

Result in Windows Debug and Release:

R198_NATIVE_ALLOCATOR_FAULT_MATRIX_PASS
positions=38 allocated=2185 freed=2185

This confirms that these **38 tested single-allocation failures** do not
cause an observed memory leak in the harness and do not poison a
subsequent manager instance. It is not a general Out-Of-Memory,
thread-safety or full process safety proof.

## Repeated lifecycle acceptance

A second test runs **128 complete manager lifecycles**:

1. Construct the actual 66-entry compiled manager with synthetic modules.
2. Queue and apply two enables under a first-party non-null
   synthetic world/player context.
3. Deliver two enabled event-0 callbacks.
4. Disable one of the modules, apply toggles, rebuild event lists.
5. Confirm that a subsequent dispatch delivers only one callback.
6. Destroy the manager and check cumulative allocation/free balance.
7. Repeat 128 times with a freshly initialized manager instance.

Result in Windows Debug and Release:

R198_NATIVE_LIFECYCLE_PASS
cycles=128 callbacks=384 allocated=4864 freed=4864

The same compiled pinned manager object was used in all runs; this is
not a recreated behavioral model.

## Regression and negative controls

The CMake harness now registers two additional private native tests:

- r198_compiled_manager_allocation_failure_matrix
- r198_compiled_manager_repeated_lifecycle

Both passed in Debug and Release. Previous R195 synthetic, R196
source-linked metadata and R197 private raw-record-layout CTests
also passed: **five of five CTests per configuration**.

The **unmodified original 67-entry compiled COFF** was separately
linked into the exact same first-party harness, built in Debug and
Release, and invoked with both new R198 test modes.

All four invocations correctly rejected the original object with
exit code 2 and precise expected diagnostic:

- Allocator mode: R198 baseline is not the corrected manager.
- Lifecycle mode: R198 repeated constructor manager layout drift.

The hash-guarded local runner
tools/run_r198_native_manager_resilience.ps1
built and tested all corrected and negative-control variants from
scratch in isolated directories and finished with exit code zero,
printing:

R198_COMPILED_NATIVE_MANAGER_RESILIENCE_ACCEPTANCE_PASS

## Reproduction on authorized Windows machine

From the integration branch, with the pinned COFF files already
compiled separately in Windows Temp:

    $root = Join-Path $env:TEMP 'slinkywinky-r193-llvm-portable'
    $rel = 'CMakeFiles/slinky.dir/src/engine/jni/module_manager.cpp.obj'
    $good = Join-Path $root ('build66/' + $rel)
    $bad = Join-Path $root ('build/' + $rel)
    ./tools/run_r198_native_manager_resilience.ps1 -CorrectedObject $good -OriginalObject $bad

The runner refuses to overwrite previous results, requires the exact
compiled object hashes, runs the two R198 tests in Debug and Release,
then requires the specific two failures from the original 67-entry
native manager in both configurations.

Public GitHub Actions only compiles the **first-party source** of
the native harness, without the third-party compiled object.
Therefore GitHub CI cannot independently reproduce these actual
COFF-linked lifecycle and allocator-failure tests unless separate
rights and secure artifact-handling gates are arranged.

## Remaining acceptance gates

- The complete OpenSlinky DLL and the original commercial Slinky
  binary have NOT been executed or authenticated for this test.
- JNI/JVMTI hooking, native bootstrap, class discovery, callback
  payload ownership, original in-game behavior and unloading remain
  unverified.
- Pointer-bearing real native record data were tested in R197,
  but R198 resilience uses safe synthetic descriptors only.
- Reentrant callback interactions, duplicate queued toggles and
  concurrent threads are NOT established safe by these tests.
- ClickGUI visual accuracy and 156 unbound original native
  subscriptions remain separate development work.
- Third-party code and binary distribution rights are not resolved.

The source reconstruction inventory is unchanged:
66 original descriptor identities, 171 native subscriptions,
13 first-party offline module policies and 15 independently
bound callbacks. No further gameplay module is promoted by R198.

Keep PR #2 draft/unmerged and PR #1 untouched.
