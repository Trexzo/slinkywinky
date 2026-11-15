# R225 — Native Fast Place callback and bounded placement policy

Date: 2026-11-15. SlinkyWinky Chat 1+2 development, native integration
branch `integration/openslinky-fd9c592-reference` (draft PR #2).
No changes to GUI PR #1. **No Minecraft process was launched.**

## Genuine new native client implementation

R224 left the first-party module-policy count at 17/66, with 24
registered native callback bindings. R225 independently reconstructs
a **new Fast Place C++ state machine** from the pinned C++ reference
`Skid-Technology/OpenSlinky@fd9c592` at
`src/logic/block/fast_place.cpp` (block category, native event 0,
priority **-1000**, one callback). The inspected source defines the
following actual behavior:

- A signed click timer decrements once per native tick; when it still
  exceeds zero the callback returns before polling the use key.
- Releasing use resets a configured activation-delay counter; continuing
  to hold must first consume that delay.
- A queued vanilla press precludes additional queued presses.
- The first physical use press is not duplicated by Fast Place itself.
- The timer advances by `20 / max_cps` Minecraft ticks per scheduled
  interval. More than one slot can be due in a tick at higher CPS.
- Only held block items may receive additional clicks; ladders bypass
  ordinary geometry checks, blocks placed into the player's bounding
  box are suppressed, and a small random-exception path exists.

These observations were independently translated into a source-compatible
**typed decision model**, not copied binary or third-party source code.
`reference_compat/include/slinkywinky/fast_place_controller.hpp`
owns its clock, held-edge state and activation countdown. The output is
`extra_use_press_count` as a **value-only proposal**, never an actual
Minecraft key press, game field write, packet, installer or injector.

The original implementation's random-walk exception is represented by
a **caller-supplied `random_bypass_granted` decision**. Its random
distribution, hardware-input synchronization and commercial exact
sequence are NOT claimed to be reconstructed. Invalid, non-finite or
unsafe configuration is refused; this independently implemented safety
cap restricts `max_cps` to 1–100 and activation delay to 0–200 ticks
and bounds the number of candidate scheduled slots per tick to six.
These limits are deliberate deviations from the unchecked source.

## Native registration, lifecycle and shared callback

`ModuleExecutionBridge` now binds Fast Place only if its metadata
matches EXACTLY one callback on **event 0 / priority -1000 / category 3**,
then evaluates the state machine when a real first-party offline
`ExecutionFrame` is admitted. The callback is absent unless explicitly
bound. Runtime owner admission is still required, as with previous
module policies.

On event 0, Safe Walk priority 0 runs before Fast Place priority -1000,
and the bridge admits both typed payloads only when their `in_game`
observation agrees. Any mismatched frame type, unexpected event or
multiple unrelated payloads fails closed. Disabling Fast Place prevents
callback execution; `reset_fast_place_world_state()` and `stop()`
retire its local clock and held state. No original client queue/hook is
modified.

## Coverage and independent regression

The synthetic source-backed 21-module fixture includes 18 independently
implemented first-party offline policies and 25 native callback
bindings (up from 17/24). For the known original 66/171 inventory, the
expected gap is **146 unimplemented callback subscriptions**; private
candidate/source hash verification is a distinct gate. Module-count
progress is NOT proof that 18 modules run in Minecraft.

New native CTest coverage:
- `r225_fast_place_native_event0_policy_and_dispatch`: direct state
  behavior, cooldown, delayed activation, first press, queued click
  suppression, ladder/overlap/target branches, deterministic random
  exception, high-CPS bounded count, invalid settings, world/player
  admission, Safe Walk priority, disable, reset, stop and incorrect
  callback descriptor rejection.
- `r225_eighteen_policy_coverage`: genuine executable module
  coverage audit against a new synthetic 21-descriptor fixture.
- Four deliberately wrong metadata fixtures (category, event,
  priority and additional subscription) must all fail audit.

**Windows MSVC Debug + Release: full first-party offline
reference_compat suites 105/105 PASS each**, including the six new
Fast Place tests. No game launch, Java agent, original DLL injection,
network use or commercial private binary execution was performed.

## Remaining real Slinky work

Fast Place is one additional native **offline implemented policy**, not
a new fully working injected gameplay module. Its original random
walk, exact client event hooks, placement legality measurements,
runtime JNI/JVMTI event binding, GUI controls and verified in-game
effects remain unimplemented. The pinned C++ source reference is
useful for fidelity, but commercial 1:1 equivalence is not verified.

Keep PR #1 and #2 draft and unmerged.
