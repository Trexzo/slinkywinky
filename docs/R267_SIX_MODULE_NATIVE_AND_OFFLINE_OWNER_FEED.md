# R267 — unified six-module source feed and GUI-native/offline owner tests

**Status:** implementation committed to PR #1 (draft/unmerged);
CI is the acceptance authority. Cross-branch source checkout is verified by
Git SHA; PR #2 stays separate (draft/unmerged).

## Previously split runtime authorities

The R211 `SprintObservationPreview` in PR #2 owns **three** source-backed
native policy probes: `No Jump Delay`, `No Slow`, `Sprint`. PR #1's
independent R254 `RecoveredSourcePolicyOwner` owns **three** strictly
OFFLINE policy models: `No Hit Delay`, `Safe Walk`, `Fast Place`.

The former R246 `NativeModuleSnapshotAdapter` published only three module
states, and R255's offline policy adapter published its own three states.
A caller could not publish an authenticated six-entry owner snapshot to one
ArrayList feed or present a consistent combined provenance roster.

## What R267 implements

`include/slinkywinky/combined_module_owner_adapter.hpp` is a header-only,
first-party, source-only bridge. The actual PR #2 native manager is supplied
as a C++ template type rather than copied or reimplemented.

- Pins every snapshot and mutation to the adapter's owner thread.
- Requires both providers to report **exactly the same nonzero world generation**
  and world-ready state, checking all six canonical names and category-backed
  registered identities.
- Publishes one **copy-only, contract-v1, six-row**
  `ModuleArrayListSnapshot` to the existing monotonic/retirable source feed.
  Reuses R265's **version-2** full 67-GUI-row provenance projection:
  three NativeOwnerProbe reports, three OfflinePolicy reports, and
  60 still-unobserved registered descriptors plus Chest Stealer's
  descriptor-only row.
- Routes an authenticated GUI focus/request to **exactly one** matching owner
  and checks that every other source row remains unchanged before ACK.
- Rejects unauthorized names, unsupported modules, stale source sequence,
  altered owner generation, mismatched snapshots, foreign owner threads,
  ambiguous rejection/exception, and post-retirement calls.
- Retires/quarantines the feed on uncertainty rather than inventing successful
  rollback or a Minecraft gameplay receipt.

`tests/test_r267_combined_six_module_owner.cpp` links the **actual**
source-pinned PR #2 R211 `SprintObservationPreview` with the existing PR #1
offline policy owner and sends all six GUI requests from a separate GUI
thread to the single owner thread. It checks all six ACKs, the unified
66-registered/67-descriptor roster, correct native-vs-offline provenance,
world loss and requested selection retention, refusal on foreign owner
calls, and complete retirement on an inter-owner generation mismatch.

The R261 current-PR2 integration workflow now compiles the new target and
runs it as its **ninth** cross-branch CTest. Historical R247 pinned-source
regressions are retained and unchanged.

## Functional boundary / still missing

This work does **not** make six modules live in a Minecraft client:
three are isolated native-policy **decision probes**, three are independent
**offline-only models**. No Java bytecode instrumentation, packet transmit,
gameplay action or WGL hook is enabled by this adapter.

The original 66 native registrations, 60 partial C++ controllers and six
experimental combat models remain separate inventory/evidence measures.
Additional in-game native providers require independently validated
callbacks, settings and owner-safe lifecycle receipts before they can
enter the same runtime feed.

## Acceptance

- `r267_real_pr2_combined_six_owner_provenance`
- The inherited R247–R266 cross-branch integration regression suite
- Main PR #1 C++ core / Windows Java 8 JNI / native loader CI

A successful CI run is **source-level integration acceptance only**,
not full original-client fidelity or a standalone injected playable build.
