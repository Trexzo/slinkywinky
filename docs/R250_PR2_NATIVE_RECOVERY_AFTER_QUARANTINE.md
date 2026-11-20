# R250 — post-quarantine native owner recovery (PR1 + real PR2)

Date 2026-11-20. Chat 1 draft PR #1. PR #2 remains unchanged.

## R249 residual risk

An ambiguous native module operation can change actual PR2 native state
before PR1's request channel and published ArrayList are retired.
Quarantine correctly clears the HUD but cannot roll back the native
module activation. Reusing the old request, feed or adapter without a
separate native shutdown and a newer session would make the view
untrustworthy.

## R250 recovery fence

New `include/slinkywinky/native_module_recovery_fence.hpp` defines
`NativeModuleRecoveryFence::admit_after_quiescence` for **offline
source-state recovery only**. It checks:

1. The old module adapter and feed are retired, have no visible
   snapshot, and the old GUI request channel is retired.
2. The old native manager explicitly reports `stopped()`, and the
   replacement is a separate non-stopped native manager.
3. The new manager reports a world generation strictly above the
   last authoritatively published old generation. Zero and exhausted
   floors fail closed.
4. The new source sequence strictly exceeds the last confirmed old
   sequence. Replays and out-of-world activation are refused.
5. The new adapter and feed must both be fresh, not already retired
   or previously published. The new owner then uses the real
   `control_states()` through the existing validated R246 adapter.

R250 records confirmed generation and sequence in the owner adapter
and retains these floors even after quarantine. Reading these floors
requires complete old-owner quiescence; they are not atomics.

**This is not OS/JVM authentication.** A future live loader must
separately prove that the previous game attachment has stopped and
the new attachment really owns the current Minecraft session.
The offline fence only validates consistency of claimed native
source-session state; it never independently establishes process
identity or caller trust.

## Exact real-native integration test

The opt-in source-pinned PR2 workflow builds
`slinkywinky_r250_native_recovery_test` against the actual
`reference_compat::SprintObservationPreview` at PR2 commit
`7f457376058ad42e0a0e0323ed874d7e5b2ae03a`.
Its disposable test owner first executes an ACTUAL No Slow mutation,
then throws before acknowledgement to reproduce R249's uncertain
native result. It confirms the GUI/ArrayList are quarantined while
the original native No Slow remains active. A premature recovery is
refused; the old native manager must successfully shut down.

A **separate native owner thread** then constructs an independent
`SprintObservationPreview` and PR1 adapter, verifies rejection of
old generation, replayed sequence, out-of-world recovery and
recycled old feed, and publishes a new authoritative session.
A new original-source GUI handoff enables No Slow and the actual
PR2 `observe` produces the corresponding decision. The old
GUI, adapter and feed cannot be reused, even after recovery.
All modules and source publisher views are retired at unload.

### Scope limitation

These are first-party offline simulations using the authentic
PR2 native policy header; no live Minecraft process, JNI attach,
actual game mutation, input interception, full original Slinky
reconstruction or new module callback recovery is claimed.
No PR2 files are modified and PR1's normal independent build
remains unaffected.

## CI

The result is recorded only from exact-head completed GitHub checks.
