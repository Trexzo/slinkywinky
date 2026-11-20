# R257 — source event 8 Reach + Hitboxes + Ghost Hand composition

Chat 1 PR #1, source-pinned PR2 comparison. Date: 2026-11-20.

The original OpenSlinky `fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4`
contains three registered callbacks on descriptor slot `+0xc8`
(event 8): `src/logic/combat/reach.cpp`,
`src/logic/combat/hitboxes.cpp` and
`src/logic/block/ghost_hand.cpp`.

PR #1 already has an independently implemented ReachController in
`src/core.cpp`, with the original source's decrementing 9-tick
attack cooldown, optional sprint requirement and activation-time
threshold. R256 newly added two separate value-only Hitboxes and
Ghost Hand proposals. R257's
`RecoveredEvent8Owner` now composes **all three** under one
owner-thread, world-generation and source-event admission boundary.

**Important fidelity constraint:** Reach distance, Hitboxes
expansion and Ghost Hand's ignore helper are **separate
typed proposed values**, not a fabricated field overwrite or
unverified cross-module priority. The original manager's exact
multi-module dispatch ordering is not yet proven. We explicitly
avoid claiming 1:1 effects on original Minecraft's raycasts.

R257 tests owner-thread-only switching, source descriptors, stale
world generations, same-generation world reentry refusal, world-loss
and re-enable reset, attack cooldown behavior and all 3 source
states in the existing authoritative ArrayList snapshot. Invalid
reach distances above 16 blocks are rejected before state
mutation as an intentional protection (the original C++ source
did not bound them).

The new pinned-source PR2 comparison compiles actual PR2
`ModuleExecutionBridge` with `Reach` event-8 registration
and its existing ReachController implementation. It compares
140 event frames across changing sprint/attacker times and
world changes; in parallel, the same R257 owner evaluates
the already reconstructed R256 Hitboxes and Ghost Hand.
Thus the 140-frame test proves **Reach parity** against that
pinned PR2 callback path, but not Hitboxes/Ghost Hand parity
against a running original game.

No Java/JNI/game thread is attached in R257; no gameplay
state is mutated. The user goal of fully reconstructed,
runnable, source-readable Slinky with all native callbacks
remains incomplete. PR #2 is never edited.

## Verification
Only mark after exact-commit GitHub CI results.
