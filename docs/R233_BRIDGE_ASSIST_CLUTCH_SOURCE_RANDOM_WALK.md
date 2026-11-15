# R233 — source random-walk integration in Bridge Assist and Clutch

Date: 2026-11-15. Combined Chat 1/Chat 2 coordination:
native `integration/openslinky-fd9c592-reference` draft PR #2 changed.
GUI `reconstruction/r1-core-fidelity` draft PR #1 was not edited.

## Pinned source reviewed

Pinned `Skid-Technology/OpenSlinky@fd9c592`:
- `src/engine/math/math_helpers.cpp`: the original shared
  xoshiro128+ `rng_next`, `walk_update`, `walk_update_timed`,
  `walk_should_change` and random-range code;
- `src/logic/block/bridge_assist.cpp`: edge margin from walk 1
  (range **0–0.15**, period **5 s**), release delay from walk 2
  (range **-0.5–0.5**, period **30 s**), and walk 3 chance
  (range **0.04–0.08**, period **60 s**) tested against a random
  0–1 roll before optional 0–3 second spike;
- `src/logic/block/clutch.cpp`: candidate-side inset
  (0–0.05, period 1 s) and X/Z shift (±0.01, 0.5 s);
  top-face candidate inset and shift (±0.1, 1 s); straight-down
  yaw randomness (±20, 0.5 s). Importantly, each actual
  aim callback samples **both** timed walk 7/8 first for
  `aim_speed * walk_update_timed(range [1-0.25*randomisation,1],
  period 10, dt 1)` and again for fractional jitter
  `walk_update_timed(range [-0.05,0.05], period 10, dt 1)`.
  The second call advances the same walk's accumulated timer
  **again** on that frame; it is not a clone/new state.

## Code changes

New `reference_compat/include/slinkywinky/source_walk_bank.hpp`
implements reusable first-party `SourceWalkBank<N>`, using the
R232 `SourceWalkRng` xoshiro algorithm and source-defined
`SourceWalkState { double t0; float a,b }`. The bank supports
the **two genuinely distinct** source algorithms:
clock-driven `walk_update` retargeting strictly when
`now > t0 + period`; timed `walk_update_timed` accumulating
`t0 += dt`, resetting its timer to zero and retargeting
only when `t0 > period`. Both linearly interpolate source
`float` endpoints, use the 0.2 minimum endpoint separation,
and preserve one PRNG sequence across the bank's indexed walks.
All samples reject invalid periods/times and commit their state
transactionally; 4096 reroll attempts bound pathological
nontermination.

`BridgeAssistController` now has opt-in `use_source_walk` and
`clock_seconds` on `BridgeAssistTickInput`, an explicit
`configure_source_walk_seed` API, and C++ source-backed
margin and release-delay draws only at their original relevant
branches. The edge checker samples the first walk when actually
called, and the release branch takes the second walk, the random
chance draw and third walk, and optionally one extra random
0–3 draw. Source values appear in typed receipts
`sampled_edge_reduction` and `sampled_release_delta`.
Failed edge validation rolls back the module's previous owner
and the PRNG state. This path is opt-in; the existing R227
external value inputs remain supported.

**Critical boundary:** the source re-reads the world corner
blocks *after* shrinking the margin. This offline controller
has caller-supplied block-solid observations, not JNI block
reads. If a sampled reduction changes which grid cells need
checking, it fails closed rather than pretending the old
observations describe different cells.

`ClutchController` has a corresponding source walk seed and
optional `use_source_walk` on its late-tick input. Its
`source_candidate_samples(top,now,randomisation)` models
the source's three random candidate-point values on side or
top faces, returning typed inset and X/Z shifts to a future
world candidate planner. The actual original candidate
enumerator, reach/raycast and plan ranking are still external
and are **not** simulated here. On an admitted actual aim,
the controller now samples the source's two timed walks **twice
each**, generating capped speed scale and fraction multiplier
jitter. When the caller reports straight-down aiming, it
also samples the original clock-driven ±20 yaw walk before
those timed samples. Result receipts expose all source-derived
values for trace inspection. The source output is still a
rotation *proposal*, not a Minecraft camera change.

`ModuleExecutionBridge` adds explicit offline seed APIs for
Bridge Assist and Clutch and refuses seeding during a callback
or after shutdown. Their original native callback bindings
and source priorities are unchanged.

## Tests

New `reference_compat/tests/test_bridge_clutch_source_walks.cpp`:
- Correct clock and timed retarget periods, the strict `>`
  boundary, the two timed updates per frame and invalid-input
  rejection without PRNG-state consumption.
- Bridge Assist margin walk only at an edge-check call,
  source release countdown and additive release delta, and
  validation of changing grid coordinates without silently
  assigning the old world block observations to new cells.
- Clutch side/top point variation, independent clock and
  timed walk indices, source first-frame speed and multiplier
  outputs, strict timed retarget, straight-down yaw walk,
  deterministic replay and nonfinite-clock rejection.
- First-party native `ModuleExecutionBridge` dispatch of
  both new opt-in policies on their original event slots,
  and refusal to configure after the owner shuts down.

Full offline Windows MSVC x64 `reference_compat` tests:
**150/150 Release PASS; 150/150 Debug PASS**. All previously
green R227–R232 module/coverage tests remain green.
No Minecraft, Forge, original injector or live user input
was run or modified.

## Honest limits and future work

These are genuine C++ offline source-structure implementations,
**not yet a playable reconstructed Slinky client**. The actual
original DLL uses one **global** RNG across every module; this
first-party API currently owns seeded RNG state per offline
module for deterministic replay. Mixed Clutch/Bridge Assist/
Block In simultaneous callback draw interleaving therefore
**cannot yet be claimed identical to the commercial binary**.
A true cross-module global-RNG transaction owner remains
future work. The production seed derives from runtime
TSC/process entropy and is neither known nor claimed to
match these user-provided fixed test seeds.

Clutch's candidate/block search and original JNI world
queries are still caller-owned evidence. Source walk outputs
are not applied to real raycasts, keyboard state, rotations
or placement packets. The opt-in paths and safety bounds
are documented first-party adaptations.

R233 extends existing source models, **not the number of
registered first-party callbacks**: policy count remains
**23**, executable bindings remain **41/171**, and
**130** callback subscriptions remain unimplemented.
Both GitHub PRs remain draft/unmerged. GitHub Actions for
the new commit are an independent verification gate.
