# R247 — source-ordered Clutch click-timer and placement gate (offline only)

PR #2 development; do not merge or treat as a playable Slinky implementation.
PR #1 native injector/ClickGUI is unaffected.

## Evidence

The user-provided private reconstructed `ClutchModule.State.placementTick`
contains the candidate event-0 ordering. Its `ActionConstants` table yields
`f(1400944) = -1.0f` and `f(1400948) = 20.0f`. Before checking a world
or block, a positive click timer decrements by one source event; when due,
`20.0f / click_speed` is added with the old fractional remainder intact.

A press is proposed only when the game is ready, placement is active,
Minecraft's hit is a block at the currently selected coordinate, and
`centered` is equivalent to the ray face being UP (`EnumFacing` ordinal 1).
The candidate state then moves to the ray-hit block's adjacent position.

The new first-party header `clutch_click_gate.hpp` implements only this
**read-only proposal**: it has no Minecraft object, key/mouse press, packet,
rotation change, JNI action port or inventory write. It rejects malformed
CPS/timer values and integer coordinate overflow. Those bounds are local
safety policy, not claims about the original client.

## Verification

The `reference_compat/tests/test_clutch_click_gate.cpp` CTest checks integer
and fractional CPS schedules, gated and rejected hits, all six ray faces,
readiness/placement ordering and malformed-evidence denial. The test makes
no real or synthetic game actions.

## Still missing

- Authentic event-0 callback, per-world/client-thread and toggle binding
- Real prior-ray/placement-result association and held-block/hotbar evidence
- Owner-bound rotation, press delivery, snap-back and disable lifecycle
- Original Minecraft 1.8.9 non-Forge injectable integration
- Live local-world acceptance and full recovered-module parity

A passing CTest is **not** evidence of in-game block placement or click
execution. This milestone only recovers the source-ordered click decision.
