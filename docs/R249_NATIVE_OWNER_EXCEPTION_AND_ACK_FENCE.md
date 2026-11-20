# R249 — fail-closed owner exceptions and uncertain native acknowledgements

Date: 2026-11-20. PR #1 `reconstruction/r1-core-fidelity`, draft and unmerged.
PR #2 untouched; integration source pinned to
`7f457376058ad42e0a0e0323ed874d7e5b2ae03a`.

## Real defect identified in R246

The owner adapter previously interpreted any
`native.request_module(...)` result of false as a stable
denial. A native manager could change its selection and
then report false due to a late internal failure. The GUI
would show `Denied` while the old active-source snapshot
remained published. Additionally, `control_states()` or
`request_module()` throwing would escape the adapter,
leaving any earlier active HUD rows exposed.

## Correction

- A native denial is accepted as a **safe, ordinary denial**
  only after the adapter obtains a second authoritative
  full native state and confirms exact equality with the
  pre-request `ModuleArrayListSnapshot`, including generation,
  requested/active bits and canonical module names.
- Denial that mutated the native state, an interrupted GUI
  acknowledgement, failed post-request state collection, or
  an exception before/after any native mutation retires both
  the request handoff and the published ArrayList feed.
- `publish_on_owner` also catches native snapshot-read
  exceptions and clears the previously published state.
- The adapter explicitly **does not claim to rollback native
  module effects or game state**. Its containment is a view
  and request admission fence. A native mutation may persist
  internally until a separate admitted owner performs a safe
  unload/reset.
- On a legitimate, unmodified PR2 manager denial, the feed
  stays current, and the GUI remains retryable.

## Actual native PR2 failure boundaries

New opt-in `r249_real_pr2_native_uncertain_acknowledgement_fence`
test compiles against the **actual PR2**
`SprintObservationPreview` (not a mock policy engine).
Test-only boundary wrappers intercept responses without
changing PR2 implementation. It checks:

1. Stable denial: native request not executed; UI `Denied`,
   unchanged authoritative source feed.
2. False denial *after* actual PR2 No Slow module activation:
   no successful acknowledgement and no stale HUD state.
3. Exceptions before and after actual native mutation:
   no escape through the owner adapter and no active feed.
4. Receiver retirement between actual native mutation and
   its acknowledgement: quarantines the feed.
5. Native state-read exception in a request and in a
   stand-alone source publication: quarantines the feed.
6. Original PR2 world-generation transition between queued
   GUI request and owner execution: stale request refused.
7. Original PR2 shutdown after GUI request admission:
   source unavailable; request refused and feed retired.

The R247–R249 dedicated CI workflow compiles and executes
all three cross-source tests against the pinned PR2 revision.
Ordinary PR1 builds do not require the PR2 tree. No Minecraft,
injection, actual player movement, OS input, or network
gameplay service is involved.

## CI evidence

To be recorded only when exact-head GitHub checks complete.
