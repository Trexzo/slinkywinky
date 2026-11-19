# R246 — PR #2 owner-state to PR #1 GUI/ArrayList adapter contract

Date: 2026-11-19. Chat 1. Branch `reconstruction/r1-core-fidelity`, draft PR #1.

## Specific missing boundary

R245 added requests for the three R211 PR #2 native-controllable modules
No Jump Delay, No Slow, and Sprint, but native `ModuleControlState`
from PR #2 was still not converted to PR #1's
`ModuleArrayListSnapshot` at a real owner-thread boundary.

The PR #2 authoritative API in
`reference_compat/include/slinkywinky/sprint_observation_preview.hpp`
is `request_module({canonical_name, enable})`,
`control_states()` returning
`optional<vector<ModuleControlState>>`, and `generation()`.
PR #1 is still a distinct branch; it does not contain PR #2's native
manager implementation or module_control_contract.hpp.

## R246 implementation

A header-only `NativeModuleSnapshotAdapter` in PR #1 now describes
the exact source-shape API as a type-checked C++17 template, leaving
the PR #2 header and source untouched. The adapter:

- pins its module-owner thread at construction;
- requires the exact ordered R211 3-name vector,
  independently checked against PR #1 R244's native identity set;
- deep-copies all three owner states (names, requested/active bits,
  world generation); never copies PR #2 engine/JVM pointers;
- rejects missing, duplicated, reordered, unknown, inactive-world
  active, and wrong-generation records;
- compares the live native owner state with the full previously
  published snapshot *before* executing a queued GUI request;
- applies the request only through the manager's
  `request_module({canonical_name, enable})`;
- accepts the next rendered state only when the owner has returned
  a fresh, coherent complete snapshot and PR #1's existing GUI
  request-id / sequence / generation acknowledgement succeeds;
- distinguishes explicit manager denial (no state changed) from
  ambiguous or inconsistent post-request states, which quarantine
  the feed. Neither may manufacture a success GUI state;
- requires monotonic source sequence and world generation, and
  retires any visible rows when an owner-supplied world-exit snapshot
  marks the module inactive.

## Regression

New portable `r246_native_three_module_owner_snapshot_and_gui_ack`
CTest constructs an independently written test manager having PR #2's
public ABI shape, runs it on a separate module-owner thread from the
GUI, and tests:

1. Original No Slow GUI click -> owner request -> active acknowledgement
   -> world generation rollover -> inactive ArrayList with requested
   selection retained.
2. A real manager denial leaves the current authoritative snapshot
   unchanged and marks the GUI request denied.
3. Stale world generation and a manager that falsely accepts without
   changing its selection both retire the handoff and its feed.
4. Missing, duplicated, reordered, invalid activation, invalid
   generation, replayed sequence, and wrong-thread snapshots fail
   closed.

This is **contract-shaped first-party integration testing**. Because
PR #2 stays separate, a fake manager is used to instantiate templates;
the actual PR #2 `SprintObservationPreview` has not been compiled
together with this new adapter yet. Real in-world actions, original
physical GUI input, live injected ownership, profiles, and the other
63 native module controls remain out of scope.

## Exact-head CI

Document only independently observed GitHub workflow results.
