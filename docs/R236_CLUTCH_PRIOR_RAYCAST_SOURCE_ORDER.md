# R236 — original Clutch raycast call-order parity

Date: 2026-11-20. SlinkyWinky draft PR #2, no PR #1 source changes.

## Source comparison

Pinned public original: `Skid-Technology/OpenSlinky@fd9c592`,
`src/logic/block/clutch.cpp`, `plan_block`.

The original checks a previous solid candidate's **angular cost** and
**strict squared 4.49-block reach** *before* issuing
`java_world_ray_trace_blocks`. When either guard fails, the source
continues scanning candidates without running that ray trace.

R235 had required `previous.ray_known` before these geometric guards.
That could reject the entire source world observation as unverified
even when no prior-block raycast would have occurred and a different
reachable candidate was available.

## Change

In `clutch_plan_original`, the previous block is still sampled and
its aim/reach evaluated in the same source order. Only when the prior
passes both angle and reach checks is `ray_known` required.

A known null ray hit still falls back to the previous block, matching
the source. A reachable previous block whose ray observation was never
made is still rejected rather than silently treated as visible.

The existing unknown-candidate, bridge-owned RNG rollback, prior block
occlusion and other conservative admission behavior remain unchanged.

## Regression

Extended the existing original candidate planner test to cover:

1. A solid, **out-of-reach** previous block whose ray observation was
   intentionally not made, followed by a valid in-range new candidate.
   Expected: source-ordered fallback candidate is selected.
2. A solid, **in-reach** previous block with missing ray evidence.
   Expected: fail closed, no state or RNG advancement.

The tests are source-owned C++ fixtures, with no game writes or JNI
interception. The old R235 test path remains intact.

## Verification boundaries

The code and tests are committed to the PR #2 branch. At the time of
writing, no GitHub workflow runs or commit status checks were returned
for the R236 branch head; this is **not** a claim that R236 CTest or
Windows CI passed. Run the complete `reference_compat` CTest suites
(Debug and Release) on an appropriate Windows/MSVC build before
considering the regression verified.

This does not bridge live Java world observations or make Clutch
playable. PR #2 remains draft and unmerged.
