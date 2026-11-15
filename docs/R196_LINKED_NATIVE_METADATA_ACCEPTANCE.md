# R196 — Actual native manager against linked DLL descriptor metadata

Date: 2026-11-10. Integration track: Trexzo/slinkywinky PR #2.
This is an isolated offline acceptance; there is no Minecraft client launch,
game input, JNI/JVMTI bootstrap, DLL load, injection, network, or original
commercial Slinky binary execution.

## Pinned evidence and provenance

- Public source revision: Skid-Technology/OpenSlinky at fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4.
- Upstream .rdata source SHA-256: b17dd02a9f6894d3dd366d559dcc5f3e2db71600571c8108473a1fe57618e3e6.
- Corrected Windows AMD64 slinky.dll SHA-256: 3613a466433fb1fa83d2d9e96815ece719c166f881c78642dc18d3e13ae2aa14.
- Corrected compiled module_manager.cpp.obj SHA-256: 632d706ec121d4809a744876f4006c0e4ece19d7af0ad4994def704a77e516ae.
- Original unmodified 67-entry manager object SHA-256: 40573226782654580a89504ecbb7ccb8b0217f832cb4d1fa47b771f277c78f80.
- The standalone C++ harness uses actual compiled manager COFF instructions, alongside synthetic first-party pointers and callbacks. No third-party source/bytecode/DLL has been committed.

## Source-linked metadata extraction

The new tools/extract_r196_linked_descriptor_manifest.py inspects the
corrected compiled DLL's 67 original linked descriptor symbols with their
linker-map addresses. It rejects any divergence from the exact pinned Git
revision, source .rdata SHA-256, PE pointer table or expected 171 original
native subscription count. It excludes Chest Stealer source index 17 and
produces a *sanitized* 66-row metadata fixture with only category,
default-enabled flag, event ID and signed priority. Actual executable
callback pointers, original strings and native instructions are omitted.

Observed linked-data distribution:

- Static descriptor count: 67.
- Active source-ordered module registrations: 66, after skipping index 17.
- Original native event subscriptions: 171 across 27 event types.
- Category bucket counts: 17, 10, 15, 7, 17 (sum 66).
- Event 0 subscriptions: 24; event 5 subscriptions: 25.
- All 66 default-enabled flags were false.
- Signed priority values ranged from -10000 to 10000.
- Sanitized fixture SHA-256 in the standalone local reproduction:
  20c36fe011826bb4cad1cee467815a8096fdf9bf6813e6f4cc90e754c73ac5b3.
- No raw native callback addresses in the generated fixture.

The original upstream source, compiled DLL, and locally extracted metadata
remain on the connected authorized Windows machine; GitHub contains only
independently authored tooling, synthetic test data and this report.

## Actual compiled native constructor and event behavior

The existing R195 first-party harness was extended with an optional R196
runtime fixture. All 66 module descriptors are created with their
**real compiled linked-data categories, event presence and priorities**,
but substitute safe first-party callback stubs. It then executes
the **actual corrected compiled native module-manager object** and verifies:

1. Constructor creates 66 active modules, preserves source order, excludes
   index 17, and matches every source index against its compacted slot.
2. All 5 category bucket sizes match the linked-DLL metadata.
3. Each of all 27 native event registry bucket sizes matches the 171
   linked-DLL subscriptions, with non-increasing registered priority.
4. Disabled callbacks do not dispatch before enabling.
5. 66 enable requests queue successfully, while the 67th is rejected.
6. The null world/player gate defers queue application; a synthetic
   non-null world/player context applies the queued state.
7. All event 5 observer stubs are delivered in the expected count
   (66 module enables times 25 event-5 subscriptions).
8. A live event-0 dispatch delivers 24 callbacks in registration order.
9. Disabling one event-0 subscriber removes it from the rebuilt
   handler list; the subsequent event-0 dispatch has 23 callbacks.
10. The synthetic disable-owner callback is invoked once and all native
    manager allocations are released: 62 allocations, 62 frees.

The result in Debug and Release was:
R196_COMPILED_LINKED_DATA_PASS registrations=66 callbacks=171
event0=24 event5=25 allocations=62 frees=62.

These callback counts refer to *registered linked metadata* and safe
first-party stub invocations, not executed original gameplay callbacks.

## Explicit negative and regression controls

- The unmodified actual native manager COFF file (67 entries) was
  independently linked into the identical test harness in Debug/Release.
  It rejected R196 linked metadata with exit code 2 and the exact expected
  diagnostic: real-metadata manager count not 66.
- The standalone new PowerShell runner
  tools/run_r196_linked_native_acceptance.ps1 verified the three exact
  native artifact SHA-256 hashes, regenerated the fixture, compiled both
  corrected and original-negative test targets, passed two tests per
  corrected configuration, and required specific rejection diagnostics
  for both original configurations. Final runner exit code 0:
  R196_PINNED_LINKED_COFF_RUNTIME_ACCEPTANCE_PASS.
- Nine synthetic Python regression cases passed locally. They test PE
  descriptor offsets, signed priority decoding, invalid categories,
  unexpected defaults, incorrect PE sections, callback address bounds,
  impossible priorities, source-index drift and subscription-count drift.
- Public reference-audit CI runs only those *synthetic* tests and
  first-party C++ source compilation. It does not receive the private
  compiled DLL or COFF artifact, and must not be described as an
  independently repeated actual COFF runtime acceptance.

## Reproduction on the connected authorized Windows machine

From the SlinkyWinky integration branch, using previously built local
reference artifacts (the runner refuses to overwrite its output folder):

    $base = Join-Path $env:TEMP 'slinkywinky-r193-llvm-portable'
    $upstream = Join-Path $env:TEMP 'openslinky-r192-native-reference'
    ./tools/run_r196_linked_native_acceptance.ps1 -UpstreamRoot $upstream -CorrectedBuild (Join-Path $base 'build66') -OriginalBuild (Join-Path $base 'build')

The run creates a local, address-free descriptor manifest and isolated
corrected/original-native CTest executables under Windows Temp. It does
not modify the pinned public OpenSlinky checkout or the other runtime PR.

## What is still unverified

This is stronger than the synthetic R195 registrations and static R194
machine-code inspection. But the module constructor still receives a
safe mirror of the linked descriptor metadata and stub callbacks, not
the loaded original .data records with their executable pointers.

The following have NOT passed live acceptance:
- JNI/JVMTI bootstrap, class discovery, original game version compatibility,
  Java hook installation, renderer/OpenGL/GPU lifecycle, and safe unload.
- The actual original gameplay callback code or event-object semantics.
- Equal-priority ordering against an authenticated decrypted original.
- ClickGUI visual parity, original module settings or module behaviors.
- Commercial 1:1 Slinky parity or redistribution/license clearance.

The overall source reconstruction inventory remains 66 original descriptor
identities / 171 original native subscriptions / 13 first-party offline
module policy implementations / 15 independent callback bindings. R196
does NOT promote any additional gameplay module as functional.
PR #2 must remain draft and unmerged and PR #1 must stay untouched.
