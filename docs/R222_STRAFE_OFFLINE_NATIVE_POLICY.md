# R222 — Strafe: independently authored two-callback offline steering policy

Date: 2026-11-15. Repository: Trexzo/slinkywinky.
Working branch: `integration/openslinky-fd9c592-reference` (draft PR #2).
Parallel GUI/runtime source: PR #1, **not modified**.

## Reference provenance and exact native registration

The pinned public C++ reference
`Skid-Technology/OpenSlinky@fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4`
identifies `src/logic/move/strafe.cpp` and the Strafe descriptor
in `src/data/data.s`. These were read to understand the algorithm,
not copied or linked into our source tree.

The separate **private**, hash-verified user-provided candidate archive
`decrypted files.zip` yielded:

- descriptor index **59**, name **Strafe**, movement category **1**,
  initially disabled;
- event **21** at native descending priority **1000**
  (snapshot sprint state);
- event **22** at priority **100**
  (propose redirected X/Z motion);
- no other registered Strafe event callbacks.

`tools/export_candidate_event_manifest.py` now pins these values as
an independent negative-control contract. The private ZIP, raw
descriptor pointers, unlicensed binary instructions and the
66-module derived TSV remain **local only** and were not committed.

## Independently authored behavior

New file:
`reference_compat/include/slinkywinky/strafe_controller.hpp`.

The model takes a typed, immutable movement observation and proposes
new horizontal motion; it **never writes** game state.

Event 21 records whether the player was sprinting. Event 22 computes
a heading from player yaw and forward/strafe signs, selects ground,
air or jump steering strength, retains horizontal speed, adds
**0.2** on a jump phase only if the preceding sprint snapshot was
false, interpolates X/Z toward the target vector, and preserves Y.

Pure policy gates include stationary input, weapon requirement,
hurt-time threshold and zero steering strength. Non-finite input,
unbounded strength and missing previous sprint snapshot fail
closed. These extra validity conditions are intentional safety
limits, **not claims of identical exceptional behavior** to the
original commercial binary.

`ModuleExecutionBridge` only binds the two callback events when
the **complete** exact name/category/event/priority pair is present.
The event router additionally supports:
- event 21 sharing with No Slow and Sprint, with native descending
  Strafe priority 1000 above No Slow 200 and Sprint 100;
- event 22 sharing with Fast Accel (Strafe priority 100 vs
  Fast Accel 0), only when both typed views agree on movement
  X/Y/Z, ground state and forward/strafe inputs;
- lifecycle world reset, no world/player admission, module disabled
  gating, and full teardown.

Both modules compute **separate proposals** on event 22. A live
action-owner resolution policy for concurrent Strafe/Fast Accel
writes remains future work; this change performs no game writes.

## Deterministic validation

`reference_compat/tests/test_strafe_bridge.cpp` exercises:
- straight, diagonal forward/backward, sideways steering and yaw;
- air vs ground vs jump strength; original 0.2 non-sprint jump boost;
- weapon, hurt time, stationary, missing snapshot and non-finite
  negative gates;
- exact native metadata, atomic two-event binding and duplicate bind;
- shared event 21/22 order, inconsistent shared movement refusal,
  world/player admission, disabled callbacks, world reset and stop.

Six synthetic fixtures include one valid **20-module/17-model**
catalog and five intentionally invalid Strafe metadata variants.
Seven named R222 CTests cover these outcomes. The older R203
fixture remains in the suite as a backward-compatibility check.

The **hash-verified private candidate manifest** was re-exported
and the actual first-party coverage executable returned exit 0:

```json
{
  "registered_modules": 66,
  "modules_with_native_handlers": 64,
  "declared_native_callback_subscriptions": 171,
  "modules_with_firstparty_offline_bindings": 17,
  "firstparty_offline_callback_bindings": 24,
  "unimplemented_native_callback_subscriptions": 147,
  "minecraft_runtime_verified": false,
  "original_module_algorithm_equivalence_verified": false
}
```

Six Python candidate-manifest unit tests passed, including a new
negative check that rejects an incorrect Strafe event-22 priority.
The targeted seven R222 native CTests pass in Windows Release.
The **entire standalone reference C++ suite passed 98/98 in
Windows MSVC Debug and 98/98 in Release** at exact source.
GitHub Linux/Windows checks are a separate acceptance gate;
the pre-existing PR #1 WGL ArrayList pixel assertion may
still fail in the composite Windows workflow.

## Boundaries

**No Minecraft Strafe runtime integration is claimed.** The new
Strafe policy is offline C++ computation only. It is not original
commercial Slinky parity, does not hook JVM packets or send user
input, does not load proprietary DLL code, and is not production
injectable. Gameplay tests still focus on previously verified
Sprint, No Jump Delay and No Slow paths.

PR #2 stays draft/unmerged; PR #1 source and the parallel GUI
blending fix remain independently owned.
