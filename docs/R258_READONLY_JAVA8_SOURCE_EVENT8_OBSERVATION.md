# R258 — actual client tick observation to read-only event-8 evidence

Date: 2026-11-20. PR1 draft; PR2 remains source-pinned and unchanged.

## Source boundary

The already implemented `ObservedState.sample()` reads original
Minecraft 1.8.9 fields on the admitted, instrumented client-tick
thread: world/player objects, ray-selected target position, camera
and a monotonic session generation. The existing native JNI
`nativeObserveFrame` acceptance also runs on this same client
thread and rejects malformed frames.

R258 introduces `R258Event8ReadOnlyObservation`. It is created
**inside the existing admitted onGameTick()**, after the JNI
observation succeeds, and creates an immutable value-only receipt
with sequence, world generation, flags, timestamp and copied camera
geometry. It owns no Minecraft, player, world, target, method,
classloader or JNI references. Its external envelope intentionally
contains **only metadata** (never coordinates, names or target IDs).
The view closes on the same client thread before the existing
class-restoration and reference-release path.

A separate `R258ReadOnlyEvent8Gate` parses and validates that
metadata envelope in C++17 with strict formatting, same-thread
source ownership, monotonic generation/time, incrementing sequence,
world/player transitions, and permanent retirement. It is a
**read-only evidence consumer**, not a game module dispatcher.

## Missing event-8 evidence is NOT guessed

The existing original-game sampler has NOT verified the Java
field/method bindings needed for the source's Reach sprint and
last-attack cooldown inputs, or Ghost Hand held-item, friend/bot
and physical mouse criteria. Module-specific configured settings
are also absent. No Hitboxes expansion choice is inferred from a
ray target. As a result `hasAllSourceEvent8Inputs()` remains
false and the C++ gate explicitly refuses promotion to Reach,
Hitboxes or Ghost Hand game decisions. R257's separate offline
source policy remains unconnected until those facts can be
obtained with independently validated bindings.

## Test evidence

- The new always-on native C++ regression tests strict parser
  negatives, cross-frame flags, monotonic world transitions,
  replay and foreign-thread rejection, and zero inferred module
  decisions.
- A new Java8 mapped-fixture smoke test calls the EXISTING
  `ObservedState.sample()` against the disposable `AttachTarget`
  field mappings, replays target loss, disconnect and world
  reentry, checks wrong-thread and nonfinite/invalid frames,
  asserts cleanup, and emits four metadata-only receipts.
- The Windows Java8 loader CTest starts that fixture and passes
  its ACTUAL Java8 receipts to the independently compiled native
  C++ gate, which enforces identical sequence and transition
  rules. The source agent is built and staged by the existing
  native loader workflow. No production JNI event-8 hook is
  installed.

**Important limitations:** The Java8 mapped test target is not
Minecraft, and the original Minecraft event-8 execution is
not confirmed in a running game. The production instrumentation
path is now source-connected to the metadata bridge, but the
current CI does not physically execute original Minecraft 1.8.9
ticks. No game action, packet, raycast, reach or input field is
modified. PR2 is not edited. Full Slinky module/client recovery
is incomplete.

## Exact-head CI

Record only after verified GitHub Actions results.
