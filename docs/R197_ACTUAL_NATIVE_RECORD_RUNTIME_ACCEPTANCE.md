# R197 — Actual linked native descriptor-record acceptance

Date: 2026-11-13. Integration track: Trexzo/slinkywinky PR #2,
branch integration/openslinky-fd9c592-reference.
The other runtime reconstruction PR #1 is not edited.

## What changed from R196

R196 executed the original compiled module-manager COFF object against 66
synthetic C++ descriptor structs populated with sanitized metadata extracted
from the compiled OpenSlinky DLL.

R197 first extracts the **exact 504-byte original linked PE .data records**
for those 66 source-indexed modules, preserving 33,264 raw descriptor bytes.
Those records are stored **only in a private, temporary binary fixture** on
the connected user's PC. This fixture contains live native address values
from the public OpenSlinky build and must NOT be uploaded to GitHub.

The independently authored C++ harness reads the raw records into its
matching 0x1f8-byte descriptor struct, cross-checks source index,
category, default-enabled flag, callback presence and every original
signed event priority against an *independently extracted R196 text manifest*.
Before the actual compiled manager sees any descriptor, the harness
scrubs/rewrites all callable callback pointers, the name pointer, extra
pointer-bearing padding and handler x values. Callable slots are replaced
with safe first-party test callbacks and priority values are preserved
as bounded synthetic scalar representations.

This is **not execution of original native gameplay callbacks or the full
OpenSlinky DLL**. It is isolated execution of the original compiled
module manager against validated original record bytes after sanitization.

## Provenance, exact hashes and private-boundary checks

- Public upstream Skid-Technology/OpenSlinky commit pinned exactly:
  fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4.
- Corrected native 64-bit Windows DLL SHA-256:
  3613a466433fb1fa83d2d9e96815ece719c166f881c78642dc18d3e13ae2aa14.
- Corrected manager COFF SHA-256:
  632d706ec121d4809a744876f4006c0e4ece19d7af0ad4994def704a77e516ae.
- Unmodified 67-entry manager COFF SHA-256:
  40573226782654580a89504ecbb7ccb8b0217f832cb4d1fa47b771f277c78f80.
- Private local source record fixture SHA-256:
  9b0904f558f9fd3ff3043f1dd381511d69bfdbb8738f85c6cff0a69a8abc3a04.
- Private binary fixture length: 33,576 bytes (48-byte header,
  plus 66 times [4-byte source index + 504-byte record]).
- Actual DLL retains **67** static descriptor pointers, including the
  public Chest Stealer slot at index 17. The corrected manager registers
  **66** entries, excluding source slot 17.
- Original linked subscriptions: **171** callbacks across 27 event
  types and five module categories. Original data callback addresses
  were never made executable in this harness.
- The exporter rejects changes in upstream Git revision, .rdata fingerprint,
  linked PE pointer table, actual DLL SHA, source order, category, enabled
  flag, callback presence or priority against the prior manifest.
- The exporter refuses to write private native pointer-bearing records
  within either the upstream source checkout or the integration project.
  The full binary fixture is not included in Git commits or public CI.

## Acceptance results — real compiled manager, local Windows

The corrected public OpenSlinky manager COFF object was linked into a
standalone first-party Windows x64 executable. No Minecraft, injection,
original commercial decrypted binary or JNI/JVMTI runtime was launched.

In both **Debug** and **Release**, all three CTest tests passed:

1. R195 synthetic manager constructor and event replay.
2. R196 source-linked 66-entry / 171-callback metadata replay.
3. R197 actual private PE record bytes after safe pointer sanitization,
   with the same module category, event priority, queue and callback
   lifetime checks.

In R197 the initialized compiled manager had 66 active registrations,
171 stubbed callback subscriptions, expected category and event bucket
counts, priority order, 66 queued enables, null world/player gate,
event-0 delivery 24 then 23 after disabling a subscriber, and balanced
native manager allocations: **62 allocations / 62 frees**.

End-to-end runner:
tools/run_r197_private_native_acceptance.ps1

Its result on the connected Windows machine:

- Corrected Debug CTest: **3/3 PASS**
- Corrected Release CTest: **3/3 PASS**
- Unmodified 67-entry native manager: **expected rejection in Debug
  and Release** with the specific 66-entry manager-count diagnostic.
- Seven deliberate malformed native record fixtures: **7/7 expected
  rejections** (incorrect header/digest, source index, category,
  event priority, missing callback pointer, truncated payload, and
  trailing unexpected byte).
- Final marker:
  R197_PINNED_NATIVE_RECORD_RUNTIME_ACCEPTANCE_PASS
- Runner completed with exit code 0.

Seven independent pure-Python regression tests also passed for the
synthetic native record serializer: exact binary size, header format,
index 17 skip, record ordering, short/oversized record detection,
wrong digest length and missing row checks.

## Repeat the local acceptance

Use verified private/local upstream and native outputs (on the user's
connected Windows machine), from the integration branch:

    $base = Join-Path $env:TEMP 'slinkywinky-r193-llvm-portable'
    $up = Join-Path $env:TEMP 'openslinky-r192-native-reference'
    ./tools/run_r197_private_native_acceptance.ps1 -UpstreamRoot $up -CorrectedBuild (Join-Path $base 'build66') -OriginalBuild (Join-Path $base 'build')

The runner refuses to overwrite a previous output folder, validates
all input compiled object/DLL fingerprints, regenerates private raw
records, verifies their deterministic SHA-256, builds separate
corrected/original-negative executables, runs both configurations and
executes seven negative controls. Its output directory is temporary
and must remain private. Public GitHub CI only runs synthetic serializer
unit tests and compiles the first-party harness source without the
third-party compiled object/record fixture.

## What this does NOT prove

- Nothing authenticates a real commercial Slinky release or proves
  bit-exact original implementation parity.
- The actual original native module callback functions were not called.
- A standalone native manager with stubbed JNI symbols cannot prove
  correctness of hooks, JVM initialization, class discovery, event
  payload ownership, original module settings, ClickGUI visuals or
  multiplayer behavior.
- Startup/unload and renderer/OpenGL integration under real Minecraft
  1.8.9 remain separate acceptance gates.
- Third-party source and binary redistribution rights must be resolved
  before adopting or distributing reconstructed/upstream native code.

Coverage inventory unchanged: 66 original descriptor identities /
171 original callback subscriptions / 13 first-party offline policy
implementations / 15 callback bindings / 156 unimplemented original
native callback subscriptions. This R197 acceptance does not promote
new gameplay modules to working status. Keep PR #2 draft/unmerged.
