# R227 — Native Bridge Assist: four original callback phases

Date: 2026-11-15. This is first-party, source-based SlinkyWinky
reconstruction in `Trexzo/slinkywinky` draft PR #2
(`integration/openslinky-fd9c592-reference`).
Chat 1 GUI draft PR #1 stays unchanged.

## Original pinned reference

Studied the pinned C++ reference `Skid-Technology/OpenSlinky@fd9c592`:
- `src/logic/block/bridge_assist.cpp`, including its helper
  `bridge_assist_edge_check` and `aimed_at_edge_below`.
- `src/data/data.s`, `bridge_assist_module_descriptor`.

The descriptor is **Block category 3**, with four original native
callback registrations, all source priority zero:

| Native descriptor slot | Candidate event | Handler |
|---|---:|---|
| +0x98 | 5 | `bridge_assist_disable` |
| +0x128 | 14 | `bridge_assist_place` |
| +0x158 | 17 | `bridge_assist_tick` |
| +0x168 | 18 | `bridge_assist_on_late_tick` |

Offsets use the descriptor's callback stride of 16 bytes, beginning
at +0x48. They are not inferred from GUI labels.

## Independent first-party C++ implementation

`reference_compat/include/slinkywinky/bridge_assist_controller.hpp`
implements a *pure offline value-only* Bridge Assist owner. It never
reads or sets the Minecraft sneak key or clicks an item. The implementation
contains:

- A source-grounded `BridgeEdgeSample` for a predicted next
  horizontal foot position. It reproduces the four corner checks after
  margin shrink, including duplicate-corner optimization and the
  original rule that **any solid sampled ground corner is safe**.
  The four cell-solid values are caller observations, not simulated
  Minecraft world reads. Missing required observations are rejected,
  rather than interpreted as air.
- `on_tick`: original physical sneak requirement (equality with
  `only_while_sneak_held`), look-down >= 65°, disallow forward when
  configured, two-tick block-held window, a 20-tick placement cooldown,
  ground/air and vertical motion checks, sneak ownership/restore, and
  the release-delay countdown. It handles only typed inputs;
  `proposed_sneak_pressed` is not an actual key write.
- `on_place`: original right-click cancellation/cooldown handling,
  back-only diagonal yaw band 30–60° per 90° quadrant, vertical
  face and ground requirements, hit coordinates within one block of
  the player and original 0.15/0.25 face-edge band. It returns
  `proposed_allow` (not a game click) or updates a bounded cooldown.
- `on_late`: source `placing` observation and toggle-mode owner
  tracking as an optional value.
- `on_disable`: on a notification explicitly targeted at this
  module, returns the physical sneak state for restoration, then
  closes retained ownership and countdowns.

The `BridgeAssistController` stores its own owner, placing status,
cooldown, held-block countdown, activation mode and unsneak timer.
`reset_bridge_assist_world_state()` and `stop()` clear the state.
The frame callback object remains outside JNI, Forge and any
original injector.

The pinned source's random-walk generators are **not** implemented
or falsely simulated: optional deterministic random margin and
release-delay deltas come from the caller. These are explicitly
marked source-fidelity gaps, as are unknown ordering/timing of native
game callbacks, actual world corner sampling and physical key
synchronization. Intentional safety limits reject non-finite or
out-of-range coordinates/settings (including missing world block
observations) rather than guessing; no 1:1 claim is made for
invalid settings.

## Four bindings / event ownership

`ModuleExecutionBridge::bind_bridge_assist_from_catalog()` requires
**exactly** category 3 and event 5/14/17/18 at priority 0. All four
first-party offline handlers are installed only after descriptor
validation, disabled modules do not receive active callbacks and
native owner state closes on reset/stop.

`ExecutionFrame` now records the explicit event notification
target and new state to ensure a disable notification can only
restore sneak for its own module. The renderer and GUI do not
synthesize module state.

On shared events, typed frames can carry other already-implemented
policies: event 5 Timer, event 14 Keep Sprint/Item Use Fix/No Hit Delay,
event 17 Sprint/Keep Sprint/Jump Delay, and event 18 Anti Debuff/
No Use Delay/Fast Mine. The bridge rejects unrelated mixed payloads
or contradictory in-game observations before any callback executes.

## Evidence and limits

New standalone C++ test:
`r227_bridge_assist_native_4_callback_policy`. It verifies unsafe and
safe corners, missing required corners, sneak acquisition and
restoration, direction/pitch gating, held-block countdown, release
behavior, edge-placement geometry, place cooldown, correct four
callback receipts, wrong source event, no-world/player refusal,
disable and shutdown. Source signature guards test six incorrect
module descriptor variants.

Native coverage tests include the genuine new 23-descriptor/20-policy
fixture and **six negative metadata fixtures** (category, disable
priority, place event, tick priority, missing late and extra hook).

**Full Windows MSVC offline `reference_compat` test suite:**
- Release: **120/120 PASS**
- Debug: **120/120 PASS**
- New R227 dedicated checks: **8/8 PASS**

The independently implemented **offline policy inventory** is now
**20** (was 19), with **31** executable original callback bindings
(was 27) and a theoretical **140 of 171** original callbacks remaining
without first-party implementations. These are source-level coverage
counts, not installed/live gameplay modules.

**No Minecraft launch, JVM hook, physical test, game write, injected
binary, game input queue or normal user Minecraft directory touched.**
The native reference C++ files remain untouched. Both GitHub PRs are
draft and unmerged. GitHub CI acceptance is separate from local MSVC.
