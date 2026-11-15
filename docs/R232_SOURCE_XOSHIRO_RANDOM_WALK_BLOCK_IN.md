# R232 — source xoshiro128+ random walks connected to Block In

Date: 2026-11-15. First-party native reconstruction branch
`integration/openslinky-fd9c592-reference` (draft PR #2).
GUI PR #1 is unchanged. No Minecraft, Forge or injector launched.

## Direct pinned source comparison

Inspected the pinned `Skid-Technology/OpenSlinky@fd9c592` sources:

- `src/engine/math/math_helpers.cpp`, `rng_next`,
  `rng_range`, `walk_randomize`, `walk_update`.
- `src/data/rdata.s`, the source 0.2 minimum retarget distance,
  xoshiro range scaling epsilon **2^-24f**, and range endpoints.
- `src/data/data.s`, **six** independent, initially zeroed
  16-byte `block_in_rng_states*` instances.
- `src/logic/block/block_in.cpp`: roof targeting calls source
  walks 1/2, wall targeting calls walks 3/4, and successful
  rotation/hotbar admission calls yaw/pitch jitter walks 5/6.
  Placement walks use period **0.5 s**, the aim-limit jitter
  walks use period **0.2 s**.
- `src/engine/protected/init.cpp`, where `seed_rng` initializes
  the original global generator from the CPU timestamp counter,
  process address entropy and four splitmix64 rounds.

The original `walk_update` uses a shared unsigned 32-bit
xoshiro128+ RNG state, retargets on strict `now > t0+period`,
initializes both endpoints if `a == b == 0`, otherwise moves
`a = b` and rerolls `b` until `abs(a-b) >= 0.2`. It outputs
`float((hi-lo) * (a+(b-a)*((now-t0)/period)) + lo)`.
No smoothing curve, arbitrary noise and independent per-module RNG
streams were invented.

## New independent C++ reconstruction

`reference_compat/include/slinkywinky/source_random_walk.hpp`
contains `SourceWalkRng` (four 32-bit words, reference shifts,
rotations and xoshiro128+ output), `SourceWalkState` (double
`t0`, floats `a`/`b`) and `SourceBlockInWalks` (six
independent states sharing exactly one PRNG).

The original uses runtime entropy unavailable to this offline
model. A caller **must provide an explicit 4-word nonzero seed**.
The model does not generate or claim the original client seed.
It accepts a measured, monotonic time in seconds and never accesses
a hardware timer, filesystem or clock.

`block_in_aim_geometry.hpp` adds opt-in
`use_reconstructed_walk` and `clock_seconds` to the existing
source-aim input. In `BlockInController::on_tick`, the
source-aim path now optionally uses walk states #1–#4 to generate
source-ranged 3D placement target samples and walk states #5/#6
to generate 0.8–1.2 yaw/pitch aim-limit jitter. This preserves
source call order: placement walks are consumed once a candidate
is selected, while jitter walks are consumed only after the
owner/held-block checks have admitted rotation.

`ModuleExecutionBridge::configure_block_in_source_walk_seed`
configures the native owner with the exact explicit seed,
rejecting stopped/active-frame calls. The previous R231
caller-supplied source samples remain the default (opt-in backward
compatibility); the R230 prefilled rotation path is also unchanged.

The R232 decision receipts expose both normalized placement
samples and jitter scale values, making source-walk state visible
in offline tests without changing game input, inventory or
camera rotations.

## Safety adaptations and fidelity limits

- Original retarget loops are unbounded; first-party C++ allows
  at most 4096 attempts before failing closed rather than hanging.
- Malformed times, periods, impossible samples and backward-time
  observations are rejected, rather than replaying undefined
  or stale source state.
- Paired placement/jitter sampling is transactional, and failed
  tick/geometry admission rolls back both the module's placement
  state and its shared RNG/walk history.
- The source's actual `rdtsc`/address/splitmix64 seed derivation
  is **observed but not executed**, since using a synthetic
  seed makes offline replay deterministic.
- Floating-point rounding could differ across compilers; this
  is source-structure fidelity with controlled inputs, **not
  verified bit-for-bit parity with a decrypted commercial binary**.
- Live world-block reads, raycasts, hotbar arbitration, user
  input and Minecraft rotation writes remain unimplemented.

## Tests

Added `reference_compat/tests/test_source_random_walk.cpp` and
CTest `r232_source_xoshiro_random_walk_block_in_pipeline`:

- Exact xoshiro128+ initial outputs from known four-word seed
  `1,2,3,4`, including post-step state and the 24-bit scale.
- Initial zeroed walks, strict `>` rather than `>=`
  retarget boundary, half-period interpolation, endpoint
  continuity, and >=0.2 next-target separation.
- Deterministic replay and refusal of zero seed, wrong
  period, backward clock, nonfinite time and invalid walk index.
- All six Block In states, separate roof/wall walks and shared
  RNG consumption, plus event-16 `ModuleExecutionBridge`
  delivery on the opt-in source-generated path.
- Invalid world observations (including after vertical movement)
  reject without consuming RNG draws or placement-mask bits.
- Absence of source opt-in preserves earlier R230/R231 behavior.

**Windows MSVC x64 local full native `reference_compat`
suite: 149/149 CTests passed in Release and 149/149 in Debug**.
R231 and R230 inherited Block In tests remain green, and
the original pinned source files were not changed.

R232 is an incremental fidelity improvement to an already counted
native offline module. Source coverage remains **23 independently
implemented policies / 41 callback bindings**, with **130/171**
callback subscriptions unimplemented. This is **not** 23 fully
playable or automatically injected Slinky modules.

Both GitHub PRs remain draft and unmerged; GitHub CI for the new
commit must be checked separately.
