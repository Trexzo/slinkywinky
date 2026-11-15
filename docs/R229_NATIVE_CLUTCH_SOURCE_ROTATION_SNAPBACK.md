# R229 — native Clutch click cadence, rotation flag and snap-back lifecycle

Date: 2026-11-15. Source track: `integration/openslinky-fd9c592-reference`,
draft PR #2. First-party C++ reconstruction only; GUI PR #1 unchanged.
No Minecraft/Forge process or injector was launched.

## Pinned reference and exact callback descriptors

The pinned independent C++ source
`Skid-Technology/OpenSlinky@fd9c592` was inspected directly:

- `src/logic/block/clutch.cpp`, particularly `clutch_tick`,
  `clutch_rotation_override_flag`, `clutch_on_late_tick`,
  `snap_back`, `should_clutch`, `plan_block`, and
  `aim_at_planned_block`.
- `src/data/data.s:clutch_module_descriptor`.

The original descriptor registers **three** Block-category-3
callbacks, not the five callbacks used by Block In:

| Source callback | Candidate event | Priority |
|---|---:|---:|
| `clutch_tick` | 0 | 0 |
| `clutch_rotation_override_flag` | 13 | -100 |
| `clutch_on_late_tick` | 18 | -1000 |

The event-13 and event-18 priorities were verified from the signed
32-bit descriptor bytes `0xffffff9c` and `0xfffffc18`.
The module maintains planned-block/face state, a click timer, a
short jump snap-back window, hurt-time history, snap-back delay and
duration, and a one-time disable-after-snap-back flag.

## What R229 actually implements

`reference_compat/include/slinkywinky/clutch_controller.hpp` is
a new independently authored C++ state machine with three
separately invoked methods:

1. `on_tick`: models the source tick's signed cooldown decrement,
   the next scheduled `20 / clicks_per_second` interval even
   without a target, exact block-and-face match, one proposed use
   click, and the newly adjacent planned block after that click.
   It never increments a real Minecraft key press.
2. `on_rotation_flag`: returns rotation override flags *only*
   while the late callback has acquired rotation ownership.
3. `on_late`: models the source hurt-age window, air/damage/backwards/
   riding admission, original candidate/held-block gate,
   optional hotbar owner-track decision, delayed and duration-based
   snap-back, angle wrapping and per-tick return-to-render-rotation,
   immediate jump snap-back and the optional one-time module disable.
   Its independent aiming step combines capped speed and fractional
   rotation, with source jitter supplied as explicit typed inputs.

The original **world candidate enumeration, block-visibility/raycast
checks, actual player-motion-relative backward detection, ray-hit
aiming, RNG random walks, hotbar switching and JNI/gameplay action
writes are NOT implemented by this increment**. The late controller
takes a candidate preverified by an external source, a backwards
predicate and bounded/finite random draw observations as inputs.
It makes no claim to synthesize these observations, reproduce the
exact original stochastic trace, or implement a complete commercial
in-world Clutch module. Click, rotation, hotbar and disable outputs
remain *proposals only*.

Malformed settings (nonfinite aim speed/click rate, out-of-range
duration/multiplier, invalid rotation priority inputs) are rejected
rather than passed to the original unbounded arithmetic. These
safety limits are first-party deviations; 1:1 parity with malformed
settings is explicitly not claimed.

## Native callback bridge and failure isolation

`ModuleExecutionBridge::bind_clutch(_from_catalog)` now requires
exactly **category 3 / event0:0 / event13:-100 /
event18:-1000**, validates all three signatures before installing
its callbacks and maintains a separate per-module owner.

On event0, the Clutch click callback composes with the existing
Safe Walk, Fast Mine and Fast Place policies; event18 carries
No Use Delay, Anti Debuff, Fast Mine, Bridge Assist and Clutch,
with Clutch's priority -1000 last. The event13 rotation flag
does not intersect another implemented first-party policy.

Event frames refuse wrong event/type, missing world/player and
contradictory shared event0 in-game observations before callbacks
run. Disabled modules get no callback receipts; reset/stop retire
the independent controller and clear state.

New `tests/test_clutch_bridge.cpp` validates:
- planned click and exact block/face targeting, adjacent-block
  update and cooldown even when no target exists;
- 15-degree yaw and 10-degree pitch native bounded aim proposal,
  immediate 20-degree jump snap-back with source-supplied wiggle,
  three-tick delayed snap-back and one-time disable;
- hurt-time, air and backward gates; bad CPS and nonfinite aim
  rejection; actual source event0/13/18 ordering in composed frames;
- wrong-world/typed-event, disabled binding, reset/shutdown, and
  **eight invalid native metadata signatures**.

The new 25-descriptor / 22-policy synthetic fixture and **seven
deliberate metadata drift fixtures** independently exercise
registration and coverage accounting.

**Source-level reconstruction coverage rises from 21 to 22
independently implemented offline policies, and 33 to 36
first-party executable callback bindings.** Of the candidate
original 171 callback subscriptions, **135 remain without a
first-party implementation**. These counts do NOT mean 22
fully working injectable gameplay modules.

The native controller is actual C++ code in the SlinkyWinky repository;
no Minecraft process was started or altered. The scope deliberately
keeps the original decrypted binary, pinned C++ upstream and
GUI PR #1 unchanged. The two GitHub PRs remain draft and unmerged.

## Verified standalone Windows acceptance

Full MSVC x64 native `reference_compat` Release build passed with
**137/137 CTests PASS**. The independently rebuilt Debug configuration
also passed **137/137 CTests**. The new R229 source-policy test, positive
22-module coverage audit and seven negative descriptor audits all
passed in the same test runs. The module-only and all-suites tests were
performed without opening Minecraft, changing its installation, or
altering the pinned reference files.

GitHub CI on the eventual R229 PR #2 commit is a separate verification
gate; local Windows tests alone do not establish cross-platform
compatibility.
