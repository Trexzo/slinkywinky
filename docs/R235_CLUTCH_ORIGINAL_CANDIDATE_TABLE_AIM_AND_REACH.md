# R235 — Clutch original 167-candidate order, side/top aim geometry and reach policy

Date: 2026-11-15. Independent first-party C++ SlinkyWinky reconstruction.
Native draft PR #2, `integration/openslinky-fd9c592-reference`.
The GUI PR #1 remains unchanged. **No Minecraft launched.**

## Original reference inspected

Pinned `Skid-Technology/OpenSlinky@fd9c592`:

- `src/logic/block/clutch.cpp`, specifically lines around
  `clutch_pick_aim_target`, `plan_block` and
  `aim_at_planned_block`.
- `src/data/rdata.s`: the original eight
  `clutch_candidates_within_distance_n` distances and all
  **167 12-bit-packed** `clutch_candidate_offsets_4_bits_each`
  entries (334 source `.byte` values). These are source data,
  not inferred distance-order assumptions.
- `src/engine/math/vec_aabb.cpp` and
  `math_helpers.cpp`: look-at yaw/pitch geometry, degree
  wrapping, source random-walk and interpolation.
- R233/R234 first-party Clutch `SourceWalkBank<8>`,
  including the bridge-owned cross-module xoshiro128+
  generator and frame-atomic rollback.

The original table's *cumulative* candidate counts by
Manhattan distance rank 0..7 are exactly
**1, 6, 19, 43, 79, 123, 163, 167**. Each packed entry
decodes x = (bits 0..3)-8, y = (bits 4..7)-8,
z = (bits 8..11)-8. The table begins with
(0,0,0), (-1,0,0), (0,-1,0), (0,0,-1),
(0,0,1), (1,0,0).

## Actual new first-party native code

- `reference_compat/include/slinkywinky/clutch_candidate_table.hpp`
  transcribes the **exact 167 original source-data offsets**
  into a typed constexpr table, and records the eight source
  candidate-count boundaries. It was extracted from the
  pinned upstream text `rdata.s`, not recovered by launching
  the game.
- `reference_compat/include/slinkywinky/clutch_candidate_planner.hpp`
  independently implements `clutch_plan_original()` as an
  **offline value-only** candidate scanner. Its input consists
  of player bounding-box minimum/maximum, eye, current
  yaw/pitch, source range index 0..7, minimum ground depth,
  max aiming angle, randomisation, source clock and explicit
  167-cell world block-solid observations. An optional
  previously planned block additionally requires actual
  solid and ray-hit evidence.
- The player-centered block base is computed from
  `center.x=(box_lo.x+box_hi.x)/2`,
  `center.y=box_lo.y-0.99` and
  `center.z=(box_lo.z+box_hi.z)/2`, then
  `floor(center)`, exactly as the source.
- When the original minimum-ground-height setting is
  positive, depth-by-depth caller-provided solid observations
  are checked in original order. A solid block closer than
  the configured depth terminates the plan as not needed;
  an unknown observation aborts it as **unverified**.
- A prior planned side block is retained only if it is
  no higher than the player-centered Y, solid, within the
  source configured angular cost, within **strict squared
  reach < (4.49)^2**, and confirmed by the caller-observed
  source ray trace. The original null ray-hit fallback to
  that same prior block is retained.
- New candidates are scanned in original packed order.
  The first *occupied* side block restricts the remaining
  search to its Manhattan-distance prefix, even when
  the subsequent reach test rejects it. Within that
  distance rank, candidates are compared by source
  `abs(wrap(yaw_delta))+abs(pitch_delta)` angular cost,
  with the same best-cost and best-rank guard. **New side
  candidates require the original strict 4.49-block
  reach**, but the original source does **not** perform
  a new-candidate ray trace in this loop.
- Any block directly below (relative x=0,z=0) is treated
  as a **top-face placement**, returning immediately
  unless the `sideways_only` setting rejects Clutch.
  This source branch deliberately bypasses the usual
  side-candidate reach/angle filters: the exact source
  behavior is preserved and documented, not misreported
  as universal reach enforcement.
- The new side aim formula chooses the face nearest the
  player on the source dominant horizontal axis. It
  clamps X/Z to the inset face bounds with source
  walk #1/#2/#3 samples, raises the aim to the source's
  `by + .25 + clamp((abs(rx)+abs(rz))*.25,0,.65)`,
  and optionally biases single-point aim toward the
  previous aim face. The top-face formula clamps the
  source walk #1/#4/#5 X/Z samples to its square inset
  and selects `by+1`. Existing native source
  `rot_look_at` returns the exact yaw/pitch-cost input.
- `ClutchLateInput::source_world` is an **opt-in**
  first-party evidence path within the existing original
  event-18 native callback. On admission, its source
  candidate plan overrides the old caller-provided
  `has_planned_candidate`, block, face, yaw difference
  and pitch difference. Returned receipts expose the
  selected source index, block/face, point and cost.
  Downstream capped/fractional aim and snap-back keep
  their previously verified R229–R234 paths.
- The original source random-walk draws are executed in
  candidate evaluation order using `SourceWalkBank<8>`.
  This shares the existing R234 single xoshiro generator
  across Clutch/Bridge Assist/Block In on opt-in bridge
  mode; invalid source observations roll back both Clutch
  state and the global generator. The older R229–R234
  caller-supplied offline paths remain available
  unchanged when `source_world` is absent.

## Source-grounded tests

New `reference_compat/tests/test_clutch_original_candidate_planner.cpp`
checks:

- All 167 exact source packed offsets, eight cumulative
  counts and each rank-specific table prefix, including
  the exact source first/last items.
- Side faces along negative and positive X and positive
  Z, their bounded clamping and source raised Y geometry;
  directly below top-face aiming and strict reach boundary.
- A top candidate taking priority **after** an earlier
  side candidate, and `sideways_only` causing an early
  no-placement return on the first direct-below block.
- First occupied side narrowing the search to the source
  rank, and lower-angle winners among candidates at
  the *same* rank, even if a more distant target exists.
- Source top-face's intentional reach/angle bypass, side
  reach exclusion, maximum angular-cost rejection, and
  source range index boundary.
- Prior planned block retention with source ray-hit
  and strict reach evidence, rejection on wrong/unknown
  ray hits, minimum-height scan and missing cell evidence.
- Malformed prior block coordinates fail before possible
  signed integer overflow; rejecting absent source-world
  data never silently converts it to air.
- Native Clutch event-18 receipt uses the new source
  planner instead of contradictory manually supplied
  candidate/rotation inputs, deterministic seeded replay,
  and preservation of earlier plan/rotation after a
  rejected candidate scan.
- The R234 bridge-owned **single global RNG** remains
  unchanged when a later unknown candidate is encountered
  **after an earlier candidate already consumed source
  random-walk draws**; a subsequent valid event is
  delivered normally.

Windows MSVC x64 C++ standalone `reference_compat`
full regression suite: **152/152 Release PASS and
152/152 Debug PASS**. The R235 targeted test also
passes independently in both build configurations.
All inherited R176–R234 tests remain green.

## Remaining fidelity gaps

This is first-party **source-order candidate selection in C++**,
not a live in-game Clutch. The original Java/Minecraft world
block-state queries, eye/bounding-box observations, raycast
result, exact match of original live entropy seed/clock,
hotbar owner writes, real mouse clicks and silent rotation
are **not implemented** by R235. The caller supplies these
values explicitly, and an unknown observation is rejected.
The native source data/reference has not been modified.

Even with the source table and deterministic shared RNG,
the complete original client may draw randomness through
other unimplemented modules. Therefore full decrypted
commercial binary 1:1 stochastic parity cannot yet be
claimed.

R235 is a deeper implementation of the **already counted
Clutch policy**: offline model coverage stays
**23 independently implemented policies, 41 native
callback bindings of 171, 130 unimplemented subscriptions**.
These counts are **not** 23 playable/injected modules.

Both GitHub PRs remain draft and unmerged; GitHub CI
requires independent confirmation after the new commit.
