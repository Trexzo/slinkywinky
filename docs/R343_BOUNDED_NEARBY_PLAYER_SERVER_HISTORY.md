# R343 — Bounded histories for every nearby remote player, not only crosshair target

## Motivation

R342 finally captures authentic historical vanilla client-applied server coordinates, with 80–600ms history and exact packet provenance. But its Forge sampler only observed the target currently under the crosshair. The moment the crosshair switched to a new player, that player's history began empty. The original Backtrack source independently tracks current and earlier target server positions, so this diagnostic limitation materially reduced usefulness.

## Changes

`SourceWorldReadOnlyHost.HistoryPort` now has a Java 8 default `captureRemoteTargets()` method preserving compatibility with all R342 and older test adapters. The source-owner tick captures a **single bounded snapshot of up to 32 distinct entity IDs**, checks player/world identity before and after each record, rejects null, repeated-ID or oversized batches, and preserves the existing JNI tick/generation fence. `SourceRemotePositionHistory` itself remains capped at 32 entities and 24 changed serverPos samples per entity.

`ForgeSourceWorldPort.captureRemoteTargets()` always gives the currently observed living crosshair target first, then selects up to 31 other nearest remote `EntityPlayer` objects from real `WorldClient.playerEntities` within a **24-block diagnostic radius**. This is an intentional diagnostic admission range, NOT recovered original source reach/range. Other entities cannot push the crosshair target out. Deterministic tie handling by entity ID and a capped insertion-sorted list of at most 31 players keep scratch memory O(31), time O(P*31) for P nearby remote players, and prevent per-tick sorting of unbounded world mob/particle lists. The source sampling does not inspect or retain any packets, mutate world state or write player motion.

History now accrues on nearby players *before* target acquisition. When the player is later aimed at, the same original Backtrack/Lag Range source event6 JNI callbacks can use already observed historical server positions. No packets are held or retransmitted; the R342 gold outline is still explicitly marked an observed diagnostic rather than original ghost-client parity.

## Verification

New real-Java8/JNI CTest `r343_java8_remote_history_batch_jni` checks one admitted native tick records 32 distinct players; switching target retains the other records; world switch atomically clears the batch; duplicate entity IDs, oversized lists, null rows and attempted native reuse after failure all fail closed. The Forge Gradle JDK8 workflow continues real Minecraft compilation/packaging and asserts the WorldClient bounded selection path is present.

**Still to reconstruct:** authentic original crosshair/reach source admission, exact packet timing and per-entity S14 ID extraction without Netty world access, 1:1 ghost packet-hold queues, in-game differential acceptance, the remaining original modules and GUI. PR #2 remains draft; PR #1 unchanged.