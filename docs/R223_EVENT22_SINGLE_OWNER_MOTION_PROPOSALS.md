# R223 — one read-only event-22 motion proposal owner (Chat 2 / PR #2)

Date: 2026-11-15. Source branch: `integration/openslinky-fd9c592-reference`, draft PR #2. Parallel GUI branch PR #1 stays separately owned and unmerged.

## R222 context and source priority

R222 reconstructs an independent offline Strafe steering proposal from event 21 (sprinting snapshot, descending native priority 1000) and event 22 (motion proposal, priority **100**). The Fast Accel policy also runs on event 22 at native priority **0**. R222 specifically left the interaction of two proposed motion writes unresolved; applying both blindly in a future Minecraft adapter would be ambiguous and could lose the first policy's result.

R223 adds a **read-only, optional value-only arbitration step**, not a game action or original-commercial-algorithm equivalence claim. It is a defensive first-party policy to help determine the shape of a future action-owner contract.

## Implementation

`reference_compat/include/slinkywinky/movement_proposal_resolution.hpp` defines `resolve_event22_motion(const ExecutionFrame&)` returning either **no proposal** or one copied X/Y/Z proposal tagged with its source:
- `strafe_priority_100` wins when Strafe and Fast Accel have both emitted valid event-22 proposals from the same typed movement observation;
- `fast_accel_priority_0` is selected if Strafe has no valid proposal and Fast Accel has one;
- neither is selected when neither proposes motion.

Admission requires event 22, world and player presence, a nonempty actual dispatch-recipient receipt, equal frame movement observations if both typed payloads are present, at most one execution per callback, and no stale proposal attached to an unexecuted payload. Non-finite proposed coordinates and changed Y values are rejected; **even an unselected invalid lower-priority proposal invalidates the entire decision**, rather than hiding it behind the winning one. No module metadata, gameplay memory, native DLL, JNI method, key state, packet, player position or runtime action interface is modified.

This helper must be invoked **only on a successfully dispatched, owner-validated `ExecutionFrame`**. `ExecutionFrame` itself is an offline value type, not a cryptographic provenance/owner proof. A future action adapter must independently enforce its JNI/world generation and thread ownership and validate the original reference's composition order before applying any motion.

`reference_compat/tests/test_movement_proposal_resolution.cpp` drives the real first-party Strafe and Fast Accel models through their metadata-bound `ModuleExecutionBridge` and checks single-owner selection. Cases cover no prior event-21 snapshot, both valid proposals, each module independently disabled, world/player absence, event mismatch, empty dispatch receipt, inconsistent forward/Y inputs, NaN/Inf motion, changed Y, duplicate callback and a forged stale proposal with execution count zero. The final test stops the module manager, invalidates the source frame and requires the resolver to refuse it.

`reference_compat/CMakeLists.txt` registers `r223_event22_single_owner_motion_proposal` in the ordinary offline Windows/Linux test suite; this adds **no** previously unimplemented native callback subscription. R222's audited coverage remains 17 independently authored offline models / 24 of 171 callback bindings / 147 unimplemented. No production Strafe/Accel gameplay integration or commercial source parity is claimed.

## Cross-branch CI handoff

At PR #2 head `10eaaa6` before R223, GitHub Actions reports a Windows WGL ArrayList pixel failure across the native-loader, JNI and Windows core workflows. The runner's source diagnostic showed `#F05B10` with `GL_BLEND` enabled versus source `#F35D12` opaque, while pinned-source audit, Linux and offline native policies passed.

Chat 1 owns that renderer. It independently pushed **PR #1 R219 `509905b`** to disable blending for the 3px opaque ArrayList accent only, retaining translucent background and restoring the incoming GL state, plus a strict second-row pixel regression. Chat 1 local Windows Release verified **107/107 CTests** and both affected JNI/WGL tests. **The next GitHub PR #2 CI run is the cross-driver acceptance gate; local success does not establish CI success.**

R223 changes no PR #1 source; both draft PRs remain separate and unmerged.
