# R230 — Native Block In: five original callback slots and placement decisions

Date: 2026-11-15. First-party source reconstruction, native integration
branch `integration/openslinky-fd9c592-reference` (draft PR #2).
GUI PR #1 was not modified. **No Minecraft process launched.**

## Evidence from the pinned source

Reviewed `Skid-Technology/OpenSlinky@fd9c592` at
`src/logic/block/block_in.cpp`, `src/data/data.s` and
`src/data/rdata.s` (the ten roof offsets and four side directions).

The original **Block In** descriptor is Block category **3** with
exactly **five callbacks**, registered at source event IDs/priorities:

| Callback | Event | Priority |
|---|---:|---:|
| `block_in_on_tick` (empty) | 0 | 0 |
| `block_in_enable_remember_facing` | 5 | 0 |
| `block_in_rotation_override_placement` | 13 | -2000 |
| `block_in_tick_while_placing` | 15 | -1000 |
| `block_in_tick` | 16 | -500 |

The C++ reference performs a roof-first placement sequence followed
by four wall columns scanned in facing order, with three levels per
wall; bits 32–41 of a single 64-bit mask track roof slots, and bits
0–11 track walls. Y movement >0.01 resets the mask. Open cells are
only original block IDs 0, 8, 9 and 31. The next aim point and player
walking-to-centre state are derived from the remaining placement
candidates, and the module automatically disables when completed,
grounded and centered.

## New first-party implementation

Added `reference_compat/include/slinkywinky/block_in_controller.hpp`.
It implements the independently authored **offline native policy**:

- Roof/side offset tables and source-marked 64-bit placement-mask
  ordering, with direction remembered from rounded player yaw.
- Bounded, typed block-id, cell-solid and player-column-intersection
  observations. **Required observations must exist**: the policy
  refuses missing world evidence rather than inventing an air block.
- Roof-first occupied-anchor checks, low-to-high wall support checks,
  placed-bit marking only after a proposed aim-aligned click, mask
  reset after vertical movement, and ground/centre completion rules.
- Source-style capped and fractional yaw/pitch aim deltas, with
  caller-supplied deterministic jitter observations. A click is
  proposed only when remaining yaw/pitch error sums to <1 degree.
- Original modes: 0 idle, 1 move forward toward block centre,
  2 remain centered and suppress alternate movement keys. Rotation
  override, physical key-state restoration and self-disable are
  **typed proposals**, not game writes.
- The original empty event-0 callback remains explicitly bound, but
  does not produce gameplay output. Event-5 toggle ownership checks
  prevent a different module's notification from resetting Block In.

`ModuleExecutionBridge::bind_block_in(_from_catalog)` requires all five
exact source registrations before installing handlers and dispatches
typed receipts across existing event-0, event-5, event-13, event-15
and event-16 native policies with source priority ordering. Wrong
events, mixed unrelated frame payloads, missing player/world and
disabled modules fail closed. Reset and stop retire the controller.

## Scope and known fidelity gaps

This is **new C++ code**, but it is still an **offline decision model**.
The original game-world block queries, geometry and player box,
random-walk sampling for roof/wall aim points, exact in-game raycast,
live rotation ownership, hotbar selection and actual key/movement
presses are NOT wired or verified. Instead, externally measured
world observations and aim differences are passed as typed inputs.
The preview centre-facing rotation can also be supplied separately.

All output is value-only; no original DLL, JVM hook, injector, packet,
Minecraft input or actual game camera was manipulated.

Invalid/missing observations and nonfinite/out-of-range settings
are deliberately refused, even where the original source used
unchecked fields. This safety divergence is not 1:1 binary parity.
No claims are made for completed game-side Block In behavior.

## Verification and coverage

New native `tests/test_block_in_bridge.cpp` verifies roof-before-wall,
source offset constants, bit ordering and reset, yaw facing, movement
modes, ground and completion gating, angle/click threshold,
source callback ordering, disable/reset, missing source evidence,
and wrong-descriptor rejection.

An additional positive synthetic 26-descriptor fixture and
**eight mutation-based negative metadata fixtures** verify category,
callback events/priorities, missing callbacks and extra registrations.

Windows standalone MSVC x64 native reference_compat suite:
- **Release: 147/147 CTests PASS**
- **Debug: 147/147 CTests PASS**
- **R230-focused: 10/10 CTests PASS**

The first-party offline inventory increases **22 → 23 independently
implemented module policies** and **36 → 41 native callback bindings**
out of the candidate 66 modules and 171 subscriptions, leaving
**130 unimplemented callback subscriptions**. This does not imply
23 fully playable/injectable Slinky modules.

Both GitHub PRs remain draft/unmerged; commit CI is a separate gate.
No Minecraft or Forge test client was launched for R230.
