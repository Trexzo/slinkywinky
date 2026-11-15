# R231 — Block In exact source-target geometry and look-at rotation

Date: 2026-11-15. First-party SlinkyWinky native C++ reconstruction.
Branch `integration/openslinky-fd9c592-reference` (draft PR #2).
The separate draft GUI PR #1 remains untouched.

## Source audit

Compared the R230 first-party `BlockInController` with the pinned
C++ reference `Skid-Technology/OpenSlinky@fd9c592`:

- `src/logic/block/block_in.cpp`: `find_roof_spot`,
  `find_wall_spot`, `aim_and_place` and the player-centering branch.
- `src/data/rdata.s`: source constants, including 3.6–3.8,
  -0.1→-0.2, 1.1–1.2, 0.4–0.6, -0.25→-0.4 and 0.25–0.4.
- `src/engine/math/vec_aabb.cpp`: `rot_look_at` yaw =
  `atan2(dz,dx)*180/pi-90` and pitch =
  `-atan2(dy,sqrt(dx*dx+dz*dz))*180/pi`.
- `src/engine/math/math_helpers.cpp`: yaw `wrap_angle` into
  [-180,180), followed by original silent-offset subtraction.

R230 knew which roof or wall *slot* to use, but only returned a
relative block offset. Its yaw/pitch `diff_yaw` and `diff_pitch`
were caller-supplied and the walk-to-center camera angle was
manually supplied by a preview setter. That was a real
source-fidelity gap.

## Actual new first-party native implementation

Added `reference_compat/include/slinkywinky/block_in_aim_geometry.hpp`
with the independently authored exact source *target geometry*
for all nine reachable roof-anchor cases and all twelve
wall-direction/height slots:

- Roof bit 33–40 uses the source's four cycling face formulas.
  Roof bit 41 uses the final underside formula at `by+4`.
  Roof anchor bit 32 does **not** produce a placing target.
- Wall bits 0–11 use each source direction's `dx,dz`, the
  bottom-level negative edge-range and the upper-level positive
  edge-range. Wall height is `by+level`.
- Source target calculations use
  `bx=floor(me.x)`, `by=floor(me.y)-1`,
  `bz=floor(me.z)`, matching original block-below-feet origin.
- A new typed `BlockInSourceAim` supplies the two original
  random-walk observations as **normalized bounded fractions**
  and provides the measured eye position, player rotation and
  silent rotation offset. The fractions are mapped to the original
  source-range endpoints (including the descending negative ranges);
  the RNG itself has NOT been independently reconstructed.
- `block_in_source_look_at` and `block_in_source_look`
  compute a deterministic target world point, absolute yaw/pitch,
  and wrapped relative yaw/pitch. The resulting delta is used
  by the already implemented bounded aim/click decisions.
- When the native source-aim path is in mode 1 (player must move
  toward the middle of the current block), the controller now
  computes centre facing directly from `me` to
  `(floor(me.x)+0.5, me.y, floor(me.z)+0.5)`, instead of requiring
  externally supplied preview rotation.

The new `BlockInTickDecision` exposes the actual computed
`proposed_world_target` and `proposed_absolute_yaw/pitch`.
Neither of those outputs, nor any proposed key press, is sent
to a live Minecraft client.

## Safety and compatibility

The new source-aim input is **opt-in**: when supplied it replaces
the legacy R230 caller-provided diff angles. When absent the
existing typed-diff API is preserved for downstream offline
tests/consumers. Source-aim observations must be finite, bounded
and must have normalized walk fractions in [0,1].
Zero-length look vectors and missing necessary block IDs fail
closed, without fabricating world geometry.

R231 also fixes a previously latent state-integrity flaw:
if a missing/out-of-range world block observation is discovered
*after* a vertical position change, its denied tick now rolls back
the placed mask, last-height observation, and movement/rotation
ownership. Invalid source-aim inputs are rejected before mutation.

These explicit bounds/rollback changes are first-party safety
decisions and not claims that malformed original binary states
would behave identically.

## Executable evidence

Added `reference_compat/tests/test_block_in_source_targets.cpp`
with direct C++ assertions for:

- All original roof face directions, corner-case repeats, and
  final upper underside slot.
- All four wall directions and wall-level edge offsets, plus
  exact endpoint samples at 0 and 1.
- Invalid/anchor placement-bit refusal and malformed source fractions.
- Source `rot_look_at` geometry, wrapped yaw near 360°, silent
  rotation offsets, invalid eye/silent rotation, and zero-length
  target vector refusal.
- Native `BlockInController::on_tick` and actual
  `ModuleExecutionBridge` event-16 delivery using the new source
  path, proving computed geometry overrides contradictory legacy
  prefilled angle differences.
- Source-derived walk-to-centre rotation and the legacy
  no-source-aim pathway preserved for compatibility.
- Rejection of a missing world observation after a Y-axis change
  **without consuming an existing placement-mask bit**.

Full Windows MSVC x64 standalone `reference_compat` builds:
**148/148 CTests passed in Release; 148/148 passed in Debug**.
Both the new R231 geometry executable and R230 original five-hook
integration test passed in Release; existing 147 inherited tests
remain green. No Minecraft, Forge or injector process was started.

## Honest remaining gaps

R231 does **not** implement the upstream `walk_update` random-walk
state machines, true JVM world block measurements, item-stack
inspection, player-visibility/raycast checks, or original in-game
rotation/key writes. Source coordinate calculations are exact
functions of the externally provided source-range observations,
not proof of stochastic trace parity or of a playable Clutch/Block In.

The native offline source-coverage inventory **remains unchanged**:
23 independently implemented policies and 41 bound callbacks of the
66-module / 171-subscription pinned reference (130 callback bindings
still missing). This release improves one existing policy's
geometry fidelity, rather than inflating implementation counts.

Both PRs stay draft and unmerged; GitHub CI is an independent
verification gate from the local Windows tests.
