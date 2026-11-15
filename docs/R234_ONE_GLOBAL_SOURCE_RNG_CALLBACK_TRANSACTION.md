# R234 — shared xoshiro source RNG across three native modules

Date: 2026-11-15. First-party, source-grounded SlinkyWinky C++
reconstruction on draft PR #2, branch
`integration/openslinky-fd9c592-reference`. The GUI track on
draft PR #1 remained untouched. **No Minecraft launch**.

## Specific original-source behavior

The pinned C++ reference
`Skid-Technology/OpenSlinky@fd9c592` implements a single
four-word **global** xoshiro128+ generator in
`src/engine/math/math_helpers.cpp`, used by its `rng_next`,
`random_rotation`, `random_chance`, `walk_randomize`,
`walk_update` and `walk_update_timed` functions. Each source
module's individual `Walk {double t0;float a,b;}` state is
independent, but every new random target is drawn from that
**shared PRNG stream**, in callback delivery order.

Prior R232 and R233 implemented the source RNG and walk timing,
but each first-party offline module controlled its own copy of
a seeded RNG. Even when seeded identically, two modules consuming
randomness could not reproduce the source's *cross-module*
random-draw interleaving.

## New C++ implementation

R234 provides an **opt-in bridge-owned shared source RNG**,
without changing the previously tested independent-mode default:

- `SourceBlockInWalks` and `SourceWalkBank<N>` now expose
  value-copy `source_rng_snapshot()` and
  `adopt_source_rng(const SourceWalkRng&)`. These transfer
  only the four-word generator; they do **not** swap or reset
  any module's walk t0/a/b history.
- The source-model controllers for **Block In**, **Bridge Assist**
  and **Clutch** forward those transfer methods while retaining
  their existing source-clock and `walk_update_timed` behavior.
- `ModuleExecutionBridge::configure_shared_source_walk_seed`
  explicitly initializes **one** four-word generator with a
  caller-supplied nonzero seed and starts all three module's
  interpolation banks together. A failed zero seed does not
  enter shared mode; further per-module reseeding is blocked
  while shared ownership is active. Stopped or actively
  dispatching owners cannot reconfigure the generator.
- On event **17** (Bridge Assist tick), event **18**
  (Clutch late aiming), and event **16** (Block In placement),
  an opted-in policy temporarily adopts the bridge's
  current generator state **just before** its source callback,
  and the bridge commits its resulting generator state
  **only after** an admitted policy result. Source callbacks
  that are disabled or rejected do not draw from the
  bridge-owned stream.
- Clutch's separately modeled candidate-point sampling can
  now be invoked as an **opt-in single source candidate
  sampling phase within** its late-tick callback, before its
  speed/jitter walk draws. The receipts report the candidate
  inset and X/Z shifts. This models one externally observed
  candidate, not the original complete world enumeration
  and its potentially many candidate draw calls.
- `ModuleExecutionBridge::dispatch` takes a value snapshot of
  the shared generator and the three source controllers
  whenever shared mode is active. A callback delivery failure
  or thrown exception restores **both the global PRNG state
  and all three source interpolation/owner states**. The
  frame is treated as a single transaction for these
  source controllers; unrelated module policies and
  already emitted diagnostic receipts are not rolled
  back by R234.
- `stop()` retires shared RNG ownership and clears its
  seed. Existing no-shared-controller test paths
  `configure_block_in_source_walk_seed`,
  `configure_bridge_assist_source_walk_seed` and
  `configure_clutch_source_walk_seed` retain their prior
  independent model behavior.

The original code has runtime-seeded entropy from TSC and
address values. R234 **does not** reproduce that private seed,
intercept a game RNG or infer a real source session; all replay
seeds are supplied explicitly. The bridge maintains a
single-thread offline event owner. It is not a claim to
implement an independently thread-safe shared random service.

## Test evidence

`reference_compat/tests/test_shared_global_source_rng.cpp`
is a new first-party executable with these checks:

1. Failed zero seed, explicit shared initialization and
   prevention of three per-module seed overrides after global
   ownership is enabled.
2. Bridge Assist → Clutch → Block In event delivery at
   the same clock time, compared **after each callback**
   to a separately instantiated three-controller reference
   oracle manually handing off the same four-word generator.
   Random samples and terminal generator states must match.
3. Reordered Block In → Bridge Assist → Clutch delivery,
   proving that random sample traces depend on the source
   callback ordering rather than being three identical seeds.
4. Preflight-rejected and typed-invalid world observations
   do not consume global draws. Subsequent admitted callback
   matches a fresh seeded baseline.
5. A synthetic throwing lower-priority event-17 callback
   executes **after** the Bridge Assist edge check has
   consumed shared draws. The exception is caught by the
   test, and subsequent callback confirms the global RNG
   **and Bridge Assist interpolation clock** were restored.
   This is an injected offline failure, not a Minecraft crash.
6. Disabled/module-no-draw callback inputs retain shared
   state, while legacy independently seeded offline policies
   continue working.
7. Stop retires shared seed and refuses future configuration.

**Full standalone MSVC x64 native reference_compat tests:
151/151 PASS Release and 151/151 PASS Debug.**
All previously validated R176–R233 CTests passed.

No original Slinky DLL was started, no Forge/Minecraft
process was launched, no in-game user state changed, and no
source/reference binaries were altered.

## Limits and next fidelity milestones

The original shared generator is used by **every** compiled
module and helper, not just these three modules. The R234
owner currently coordinates only the first-party modeled
randomness of Bridge Assist, Clutch and Block In. The
remaining original callbacks, world-query-dependent branches
and other random consumers are not yet integrated, so
**exact complete commercial client draw order/parity is
not proven**.

The C++ source requires candidate enumeration, raycasts,
live world-block measurements, camera/hotbar arbitration,
and source-native timing; those are still externally supplied
or unimplemented. This is a real source-level native
integration step, not three fully playable modules or a
runtime injector.

R234 adds no descriptor or original native callback:
implementation counts remain **23 source-level offline
policies / 41 bound callbacks of 171** (130 unimplemented).
Both PRs remain **draft/unmerged**. GitHub Actions on the
new commit require separate verification from local tests.
