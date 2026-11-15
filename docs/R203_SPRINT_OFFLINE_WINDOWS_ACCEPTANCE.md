# R203 — Sprint source-matched offline module reconstruction and Windows acceptance

Date: 2026-11-14. Repository: Trexzo/slinkywinky. Branch:
integration/openslinky-fd9c592-reference, draft PR #2. PR #1 belongs
to the parallel Minecraft runtime and GUI track and was not modified.

## Reference and provenance

Source read directly from public Skid-Technology/OpenSlinky pinned commit:
fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4

- src/logic/move/sprint.cpp
- src/logic/move/sprint.h
- src/data/data.s (sprint_module_descriptor)

The native linked Sprint descriptor is category **1** (movement)
and contains exactly two event subscriptions:

| Phase | Native event | Signed priority |
|---|---:|---:|
| Player update | 17 | 100 |
| Movement input | 21 | 100 |

Source settings are while_in_gui, while_using_item, while_backwards
and while_sideways. The source-aligned player update requests the
sprint key held when no GUI is open, or while_in_gui is enabled.
When a GUI disallows sprint and the prior forced flag was set, it
proposes stopping forced sprint; notably the source returns early
without clearing its forced flag. When allowed and the player is
already sprinting, the module proposes reapplying that state.

On movement input it makes no change when stationary, or when using
an item without permission, or when running forwards without an
item (already supported by vanilla). When the appropriate sideways,
backwards or item-use setting allows a nonstandard sprint, it
proposes **1.3x** movement speed and sprint state true.

The archived third-party Java and C++ candidate sources are useful
cross-checks but none is authenticated as the original commercial
Slinky binary, and this report does not assert exact original parity.

## What R203 actually implements

New first-party:
reference_compat/include/slinkywinky/sprint_controller.hpp

Added two typed, data-only callback bindings:
reference_compat/include/slinkywinky/module_execution_bridge.hpp

- SprintUpdateInput / SprintUpdateDecision: sprint-key pressed proposal,
  optional player sprint-state proposal, first-party forced-state tracking.
- SprintMovementInput / SprintMovementDecision: optional finite speed
  proposal and boolean sprint-state proposal.

Both callbacks are registered atomically only when the candidate
catalog contains exactly Sprint/category1 and both event/priority
pairs (17,100) and (21,100). Duplicate binds, wrong category, wrong
priority, missing or unexpected subscriptions are rejected.

Bridge dispatch accepts exactly the source-aligned shared event
combinations:

- Event 17: Sprint update, No Jump Delay, Keep Sprint update (any
  valid two together or all three).
- Event 21: Sprint movement input and No Slow.
- Event 21 with both typed movement payloads must describe the same
  forward/strafe coordinates and using-item value.
- Sprint and Keep Sprint update payloads must agree about player
  presence if delivered on the same event.

Incorrect event/payload pairs and absent world/player are refused
before invoking callbacks. Reset and shutdown clear the first-party
forced-state tracker. The key-binding release itself remains a
separate unfinished runtime/JNI shutdown obligation.

Defensive differences from the candidate source: nonfinite input,
missing player, and float overflow are rejected rather than
producing invalid proposals. An MSVC warnings-as-errors check
found speculative constant arithmetic overflow while testing
FLT_MAX. R203 now bounds the double intermediate and performs
the final float conversion using a runtime intermediate; subsequent
Windows Debug and Release builds passed.

**No actual JNI key press, Minecraft movement setter, input hook,
network event, client injection, or live process interaction is
performed in these offline callback paths.**

## Deterministic regression tests

New:
reference_compat/tests/test_sprint_bridge.cpp

Tests include GUI allowance, forced sprint state persistence on
the native early-return branch, no-player behavior, reset,
already-sprinting reapply; stationary/forward/backward/sideways and
item-use conditions; invalid values including NaN, Infinity and
overflow; typed multi-module event dispatch; wrong-event and
inconsistent input rejection; disabled-state suppression; and five
malformed module-source metadata shapes.

Added the 19-row synthetic offline coverage fixture and five
negative fixtures under reference_compat/tests/fixtures/.
The audit now requires Sprint in the private candidate 66-module
catalog, and computes the intended total:

- 66 catalog module descriptors;
- 171 candidate original callback subscriptions;
- 16 first-party *offline* module policies;
- 22 independently bound offline callback subscriptions;
- 149 candidate callback subscriptions not implemented.

The private 66-module manifest was **not** reloaded in this specific
acceptance. The new synthetic 19-row fixture actually verified
16 policies/22 callbacks and two unbound synthetic subscriptions;
149 is the 171-minus-22 full-candidate coverage target, not direct
evidence of in-game completeness.

Named R203 CTests:
- r203_sprint_update_and_movement_offline
- r203_sixteen_policy_coverage
- r203_reject_sprint_wrong_category
- r203_reject_sprint_wrong_update_priority
- r203_reject_sprint_wrong_movement_priority
- r203_reject_sprint_missing_movement
- r203_reject_sprint_extra_subscription

Five intentionally malformed metadata fixtures must return failure,
and CTest marks those expected rejections as PASS.

## Actual local Windows verification

On the authorized connected PC DESKTOP-LTXXO4K, the isolated PR #2
checkout was fast-forwarded without modifying PR #1. The local build
used Visual Studio 18 2026 / MSVC 19.51 x64, Release and Debug.

- New R203 specific tests: **7/7 PASS** in Release.
- All first-party reference suite CTests: **88/88 PASS in Debug**
  and **88/88 PASS in Release**.
- Actual pinned corrected native public OpenSlinky module-manager
  COFF object SHA-256:
  632d706ec121d4809a744876f4006c0e4ece19d7af0ad4994def704a77e516ae
- Compiled native-manager harness (the separate R195–R199 manager
  acceptance): **7/7 PASS in Debug, 7/7 PASS in Release**,
  including the private R197 record-layout test. The tested
  compiled manager is a reference component, *not* the new
  SprintController or original Sprint gameplay callback.

Local test output remained under Windows Temp:
slinkywinky-r203-reference-local
slinkywinky-r203-actual-coff-native

Original DLLs, private pointer-bearing fixtures and third-party
compiled native objects remain local; they are NOT committed.

## What remains before full parity

1. Verify Sprint event timing in a real, authorized isolated
   Minecraft 1.8.9 client with controlled measurements.
2. Route player update and movement input through an authenticated
   game-thread JNI/JVMTI adapter with strict ownership rules.
3. Handle sprint-key release on module disable, world unload,
   disconnect and client teardown so no key remains stuck.
4. Validate GUI setting persistence, defaults, keyboard overrides,
   and interactions with No Slow and Keep Sprint under live input.
5. Compare the GUI and gameplay against authenticated original
   client evidence, including the reference's forced-flag quirk.
6. Keep PR #2 draft/unmerged until license, source provenance
   and real runtime acceptance are resolved.

All acceptance claims above are deliberately scoped to *offline
first-party algorithms* or the separately tested public compiled
manager; no authenticated commercial original parity is claimed.
