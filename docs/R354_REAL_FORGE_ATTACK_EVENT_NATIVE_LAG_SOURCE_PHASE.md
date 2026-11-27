# R354 — Real Forge 1.8.9 attack-event observation to native Lag Range e15

R353 added the source-native Lag Range event15 JNI callback but used
only a `lastAttackerTime == ticksExisted` snapshot at ClientTick.END.
Forge 1.8.9's actual `AttackEntityEvent` is published on
`MinecraftForge.EVENT_BUS`, already registered by `ForgePreview`.
Unlike the recovered original phase-1 hook, this Forge event occurs
before the attack is fully processed and can be canceled. It proves an
**attack attempt**, never a confirmed hit or original hook parity.

R354 adds a separate property
`-Dslinkywinky.r354ForgeAttackEventReadOnly=true`.
It requires manual F9 opt-in, the R337 native read-only owner, local
singleplayer and no action-trial mode. The Forge subscriber at LOWEST
priority rejects canceled attempts, the wrong player/world, foreign
thread and nonliving targets. It never writes to the event, game or network.

The new pure Java8 `SourceAttackEventFence` coalesces same-tick
attempts into one pending receipt and requires exact object world/player
identity, a one-shot same/next-tick consume, age <=150ms and the
original client-owner thread. Disconnect, world replacement, stop,
stale timestamps, foreign players and duplicate delivery fail closed.

`ForgeSourceWorldPort.sourceLagRangeAttackNow` requires BOTH a
consumed real Forge attempt AND the candidate's actual private
`EntityLivingBase.lastAttackerTime == ticksExisted` before invoking
native source e15 through the existing `SourceWorldReadOnlyHost`.
The original C++ event15 then executes before event18, sharing JNI
world/player/generation ownership; it may subsequently influence
the read-only event3 packet decision.

The explicit R353 timestamp-only mode remains unchanged if R354 is
disabled. No packet retention, cancellation, input automation,
native injection or original-event phase parity is claimed.

Tests: `r354_java8_forge_attack_event_fence` exercises absent,
duplicate, wrong world/player, timeout, clock inversion, previous
or more-than-next tick, stop/reset, malformed input, foreign owner
and close. The real-JNI R353 smoke also checks a fenced attempt prior
to native event15, while ForgeGradle 1.8.9/JDK8 builds the actual event
subscriber. PR #2 stays draft and PR #1 is not changed.
