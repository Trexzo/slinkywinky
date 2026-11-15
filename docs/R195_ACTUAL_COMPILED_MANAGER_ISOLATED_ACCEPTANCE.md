# R195 — Isolated execution of the actual compiled 66-entry module manager

Date: 2026-11-10. Branch: integration/openslinky-fd9c592-reference (PR #2, draft).
Separate from the other chat's reconstruction/r1-core-fidelity PR #1.

## Boundary

This acceptance executes only the previously compiled public OpenSlinky
module_manager.cpp.obj inside a newly authored, standalone Windows x64
test executable. The original full slinky.dll is **not** loaded or injected.
No Minecraft JVM, real Java classes, game processes, network, OS hooks,
user input, decrypted commercial binary or upstream asset data is involved.

The first-party synthetic harness resides in
reference_compat/compiled_manager_harness/manager_acceptance.cpp.
CMake links the manager COFF object separately, using synthetic 67 pointer
slots, synthetic module descriptors, stubbed external Java/JNI symbols and
first-party allocation, logging and event callbacks. The source is authored
independently; the third-party object file is **never committed to GitHub**.

## Exact pinned compiled manager inputs

Pinned upstream Git revision:
fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4.

- Original 67-entry module_manager.cpp.obj SHA-256:
  40573226782654580a89504ecbb7ccb8b0217f832cb4d1fa47b771f277c78f80
- Corrected 66-entry module_manager.cpp.obj SHA-256:
  632d706ec121d4809a744876f4006c0e4ece19d7af0ad4994def704a77e516ae
- The standalone test executable is compiled with Visual Studio 2026 MSVC
  19.51; CMake uses MultiThreaded (/MT) static CRT to match the public COFF
  object's own /DEFAULTLIB:libcmt.lib directive. No conflicting CRT warning
  remains after this change.

## Actual corrected COFF: Debug AND Release PASS

Both the Debug and Release standalone executables linked the *same* corrected
COFF object. Both completed CTest with exit code **0**.

Verified at runtime using synthetic module descriptors and event callbacks:

1. Constructor reports **66 modules** and retains original indices 0..16,
   skips Chest Stealer at index 17, and includes all remaining descriptors
   (source slot 18 appears as active slot 17, source slot 66 as active 65).
2. All category buckets sum to **66** registrations. Event-type buckets
   contain the expected synthetic callbacks.
3. Two distinct callback priorities are sorted descending, and live event
   dispatch invokes the higher-priority handler first.
4. Disabled handlers are not delivered before enabling.
5. Queued enables reach **66**; the 67th attempt is denied.
6. The manager defers applying enables with a null synthetic world/player,
   and drains the queue after both synthetic pointers are provided.
7. Enable notices are delivered on event 5 for all **66** enabled entries.
8. Event-4 disable veto prevents a disable, followed by a successful unvetoed
   disable and event-list rebuild. The disabled handler is no longer called.
9. The synthetic release_owner callback fires exactly once on disable.
10. All **38 tracked native manager allocations** are balanced by **38 frees**
    on destruction.

Sample acceptance:
R195_SYNTHETIC_COMPILED_MANAGER_PASS count=66
priority=high-before-low enable=66 veto=PASS disable=PASS
allocations=38 frees=38

## Negative control using ACTUAL unmodified 67-entry COFF

The same compiled test harness was independently built with the original
public upstream 67-entry module_manager.cpp.obj. Debug and Release both
returned the **expected rejection** (exit code 2):

R195_COMPILED_MANAGER_FAIL compiled constructor is not 66-entry

This confirms the harness is sensitive to the regression rather than
unconditionally succeeding. The negative-control runner verifies the expected
exit and diagnostic text; it must not treat a crash or unrelated error as a
successful negative control.

## Reproduction and CI

tools/run_r195_compiled_manager_acceptance.ps1 validates both exact COFF
SHA-256 fingerprints, builds separate corrected and original-negative
executables, runs Debug/Release tests, and requires both positive passes and
both specific negative rejections. It refuses to overwrite any existing
output folder.

Example from the GitHub integration branch using the already-built local
COFF objects (the paths are on the authorized Windows PC):

    $base = Join-Path $env:TEMP 'slinkywinky-r193-llvm-portable'
    $rel = 'CMakeFiles/slinky.dir/src/engine/jni/module_manager.cpp.obj'
    $good = Join-Path $base ('build66/' + $rel)
    $bad = Join-Path $base ('build/' + $rel)
    & ./tools/run_r195_compiled_manager_acceptance.ps1 -CorrectedObject $good -OriginalObject $bad

The script was run successfully end-to-end on connected Windows. Both
corrected configurations passed; both original configurations rejected
with exact diagnostic; runner exit 0 and final marker:
R195_ISOLATED_ACTUAL_COFF_ACCEPTANCE_PASS.

The public GitHub reference workflow on Windows only compiles the new
first-party harness source using SLINKY_MANAGER_SOURCE_ONLY=ON. The public
workflow never receives or executes the third-party COFF object. Therefore
a successful GitHub build of that job is **not** a repeat of the private
actual native manager execution. R194's static PE and object checks remain
a separate gate.

## Remaining gaps

The test uses **synthetic**, not actual DLL-linked, descriptor pointers.
The R194 static audit independently verified that both full DLLs contain
the correct linker-addressed original static table, but the harness did not
load that table into memory. It does not verify behavior under a live JVM,
actual game-driven callback order, reentrant observer safety, equal-priority
ordering, JNI/JVMTI hook installation, ClickGUI rendering, Minecraft input
or lifecycle teardown of the complete client. The original commercial
decrypted reference is not executed and bit-for-bit parity is not claimed.

No new first-party offline module policy or bound callback is promoted.
Coverage remains 66 descriptor identities, 171 original native callback
subscriptions, 13 independent offline policies, and 15 bindings. There
are still 156 native subscriptions without a first-party implementation.
Resolve third-party redistribution rights before considering code reuse
or distributing a compiled candidate. Keep PR #2 draft/unmerged.
