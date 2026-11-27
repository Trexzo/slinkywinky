# R353 — Original Lag Range event15 JNI and bounded Forge tick-snapshot proxy

## Private source evidence

Inspected the privately owned `c++_possibly_OpenSlinky-main.zip`,
`src/logic/combat/lag_range.cpp`. Its `lag_range_attack_hook` reacts
**only to phase 1**: compares the player's current `ticksExisted` to
`EntityLivingBase.lastAttackerTime` and stores the result in
`lag_range_hit_time_unchanged`. The next `lag_range_tick_decide_whether`
consumes and clears this flag. The subsequent outgoing packet callback
may release its hold on an attack. Source subscription event IDs:
Lag Range event15 (attack), event18 (tick), event3 (packet).

## Implemented

- Bound the already existing `ModuleExecutionBridge.bind_lag_range_source_attack`
  inside the **same C++ source JNI owner** used for the other four modules.
  `SourceRuntimePreview.LAG_RANGE_ATTACK=10` accepts strictly two bools,
  returns an event15 execution receipt, and shares the same world/player/
  generation/tick and source-state lifecycle. The Java8/native ABI now has
  eleven operations, not a separate fake Java attack controller.
- `SourceWorldReadOnlyHost.AttackPort` supplies an *optional*
  conservative current-tick signal. If present, the owner invokes
  native event15 **before** its Lag Range event18 tick and returns a
  typed `TickReceipt.lagRangeAttack`; no attack sends, click simulation
  or packet retention.
- The opt-in Forge 1.8.9 adapter uses
  `-Dslinkywinky.r353NativeLagAttackReadOnly=true` (plus existing R337
  flag, local singleplayer and F9 admission). It reads the candidate-named
  `EntityLivingBase.lastAttackerTime` as a private `int` field, requiring
  `lastAttackerTime > 0` and equality to `ticksExisted`. Unknown
  reflection mappings and changed world ownership fail closed. A single
  `R353_READONLY_NATIVE_LAG_ATTACK_E15` diagnostic is emitted on positive
  JNI receipt, with `exactOriginalPhaseUnverified=1 gameWrites=0`.

## Verification contract

New `r353_java8_native_lag_range_attack_phase` CTest executes real
Java8→JNI→C++ event15→event18→event3, asserting a genuine native
attack release, wrong-phase non-release, fresh-world reset, invalid
boolean rejection, foreign-thread refusal and the full owner host's
optional attack-port ordering. Existing JNI smoke, native CTests,
ForgeGradle packaging and source audits remain enforced.

**Limitation:** Forge `ClientTick.END` is not the original
source event15 phase-1 attack callback, so the real Forge trigger is a
**read-only tick-timestamp proxy**, not exact original timing or proof
of Slinky behavior. No live positive in-world receipt yet; production
network-owner integration, standalone injectable UI and 1:1 original
fidelity remain incomplete. Keep PR #2 draft, PR #1 unchanged and
private original sources out of GitHub.
