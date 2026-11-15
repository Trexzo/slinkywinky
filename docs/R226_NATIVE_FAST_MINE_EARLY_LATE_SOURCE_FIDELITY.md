# R226 — original-source Fast Mine dual callback native reconstruction

Date: 2026-11-15. Owned by combined Slinky reconstruction chat; source
branch: `integration/openslinky-fd9c592-reference` (draft PR #2).

## Exact reference evidence

Reviewed the pinned independent OpenSlinky C++ reference
(`Skid-Technology/OpenSlinky@fd9c592`):
`src/logic/block/fast_mine.cpp`, and its descriptor
`src/data/data.s` at `fast_mine_module_descriptor`.
The descriptor's category byte is **3 (Block)** and has exactly two
callbacks: `fast_mine_on_tick` at offset `+0x48` (event **0**,
zero native priority), and `fast_mine_on_late_tick` at `+0x168`
(event **18**, zero native priority). The module source confirms this
is a **two-phase controller**, not a simple generic speed multiplier.

On the early tick, it reads the block-controller's progress and
block-hit delay, stores the RAW observations, optionally caps a delay
above `break_delay`, or decrements an unchanged delay by one when
`decr_block_hit_delay` is enabled. In particular, even when a cap
is proposed, the saved last delay remains the RAW observed delay.
After the game's mining update, the later tick accelerates progress
only for a zero -> positive progress transition. Its math is:

`normal_ticks=ceil(1/progress_per_tick)`
`wanted_ticks=ceil(1/(speed*progress_per_tick))`
`proposed_progress=(normal_ticks-wanted_ticks+1)*progress_per_tick`.

## First-party implementation

Added `reference_compat/include/slinkywinky/fast_mine_controller.hpp`
with independently authored `FastMineController`, `on_early()`
and `on_late()`. It preserves the source's exact two-phase
observation dependencies, delay precedence, original first-positive
mining progress rule and raw-last-delay tracking. It returns only
**typed proposed delay/progress values**, with no actual Minecraft
fields, key state, memory, packets or native injectors touched.

Bound its two callbacks from the exact original Block
category=3/event0:0/event18:0 descriptors using
`ModuleExecutionBridge::bind_fast_mine(_from_catalog)`. No
implicit subscriptions or synthetic callback owner are invented.
The bridge admits source-matching shared event0 observations with
Safe Walk (priority0) and Fast Place (priority-1000), and shared
event18 observations with No Use Delay and Anti Debuff (priority0).
On a mismatch of `in_game` observations it rejects the frame.
It resets both per-frame decision receipts and the native module
controller on disable/world reset/stop.

Original code leaves unbounded or unsigned-overflow arithmetic
possible for malformed settings. Our **explicit safety deviations**
reject nonfinite or out-of-range damage/delay/speed instead of
propagating invalid proposals, require `speed>=1` and bound it
at 100, and decline values above complete mining progress. A rejected
early observation clears stale prior state. These bounds are
first-party safety policy, NOT independently proven commercial
exactness. No 1:1 behavioral claim for exotic invalid settings.

## Tests and coverage

Added a native state/bridge executable, the synthetic
22-module / 19-implementation fixture, and **five deliberate
metadata negative fixtures** for wrong category, incorrect early
priority, incorrect late priority, missing late hook and extra
subscription. The test verifies real C++ model behavior with
progress 0 -> 0.1/speed2 producing **0.6** proposed progress,
raw delay cap 8 -> 5, repeated unchanged-delay decrement, initial
zero state, invalid inputs, independent early/late phase, shared
event0/event18 callback delivery, world/player admission, disable,
world reset and permanent shutdown. No gameplay writes.

The synthetic accounting increases first-party independently
implemented native module policies **18 -> 19** and executable
registered callbacks **25 -> 27**. The expected gap in the
original 66-module / 171-callback inventory is now **144** callback
subscriptions. This is SOURCE IMPLEMENTATION coverage, not
the number of active playable modules.

**Windows MSVC first-party standalone offline suites: 112/112 PASS
in Release and 112/112 PASS in Debug**, after the final stale-
early observation safety change. CI on the combined PR #1/#2
GitHub merge remains a separate acceptance gate.

**No Minecraft game process, Forge launcher, game input hook,
JNI gameplay integration or native injector was executed.**
PR #1 GUI was not edited and remains draft/unmerged; PR #2 also
remains draft/unmerged.
