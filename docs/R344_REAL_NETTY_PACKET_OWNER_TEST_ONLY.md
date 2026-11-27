# R344 — First real Minecraft 1.8.9 packet ownership/replay, isolated in Netty tests

R343.2 passed all five GitHub workflows after a rerun of an unrelated truncated Forge Maven download. This milestone implements the first **actual** packet-reference retention/delivery FIFO in a strictly isolated **Forge JUnit test source set**. It is not a simulation of merely copying primitive metadata; real `S18PacketEntityTeleport`/`S06PacketUpdateHealth` objects go through the Netty pipeline, are temporarily held, and are later forwarded as the exact original object references. It does NOT turn on the mechanism in any playable client.

## Packet-owner invariants

- `ForgePacketHoldOwner` is packaged ONLY under `experimental/forge189/src/test/java`, never in the released JAR. `ForgePreview` remains wholly unchanged. Test-owner creation and admission must occur on the SAME Netty event-loop thread. No JNI, Minecraft World or entity references or game writes.
- Packet holding is disabled by default. A positive, explicit target ID must arm the isolated test owner. An exact `S18PacketEntityTeleport` matching that entity ID initiates a hold. Once any packet is held, *all subsequent decoded `Packet` objects are held behind it* to prevent FIFO inversion. Wrong-target and unarmed packets pass straight through.
- Bounded 64-entry FIFO, bounded age 150ms. On timeout, cap overflow, clock regression or explicit disarm, the owner releases the exact original packet references to the original downstream pipeline once, in arrival order, and disarms on failures/expiry. No copying/re-encoding or fabricated `Packet`/`ChannelPromise` objects.
- On active pipeline removal, the queued originals are delivered before handler detaches. On an inactive/disconnected channel, queued originals are not sent to a dead world: remaining Netty references are `ReferenceCountUtil.release`d and counted abandoned (not miscounted as delivered). Callback and management operations are event-loop-affine; a foreign thread cannot arm/release.
- This is a deterministic protocol experiment. Clock/expiry are explicitly driven by the test. The next live adapter must establish exact source timing, reentrancy, world admission and client-to-Netty policy ownership, then require independent real in-game acceptance before any live packet interception is considered.

## Executable tests

`ForgePacketHoldOwnerTest` uses eight JUnit4 tests and the REAL Forge 1.8.9 Netty `EmbeddedChannel` + Minecraft packet classes: default pass-through; selected teleport plus following packet retained/replayed FIFO; wrong entity pass-through; deterministic timeout; 64+1 overflow fail-open without loss; active handler-removal replay; disconnect abandon/no replay; foreign-thread refusal. CI asserts all eight tests passed and the actual packet-owning class is absent from the distributable mod JAR.

## Remaining 1:1 work

This isolated, bounded ingress mechanism is NOT evidence that the proprietary original Backtrack holds these precise packet classes, nor that the production client safely delays/discards or replays original packets. It does not cover outbound packets, the original interleaving of pass-through feedback, exact original source reach admission, same-world JNI/Netty owner handoff, or test multiplayer behavior. PR #2 stays draft and unmerged, and no decrypted original assets are committed.