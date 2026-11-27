# R345 — Real outbound Minecraft Packet and ChannelPromise ownership, JUnit only

R344.1 provided a verified test-only inbound owner that holds the *original* Minecraft 1.8.9 `Packet` objects in a Netty `EmbeddedChannel`, and releases them unchanged in FIFO order after an expiry, overflow, or disarm. The R344.1 Forge JDK8 build passed all eight real Netty tests.

R345 extends the **actual packet/promise ownership infrastructure** to the outgoing direction while retaining an explicit test-only boundary. `ForgeOutboundPacketHoldOwner` and eight JUnit4 tests exist exclusively under `experimental/forge189/src/test/java`, NOT under src/main, NOT in the built mod JAR, and NOT installed into the current passive R339 Forge tap. No client gameplay or network interception is enabled.

## Packet and promise ownership contract

- Owner is attached to one Netty event loop, with explicit admission; unarmed default sends everything unchanged. The first real `C03PacketPlayer` starts the bounded hold. Subsequent decoded `Packet` writes stay behind that first packet, regardless of type, to preserve FIFO. If the queue is empty, other packet kinds bypass normally.
- Stores up to **64 original `Packet` references and corresponding original Netty `ChannelPromise` objects**, not serialized copies or synthetic promises. A deferred `flush` is tracked and performed only after the original writes are replayed, ensuring no later outbound packet is silently flushed ahead of the held original.
- Explicit disarm, 150ms monotonic expiry, capacity overflow and clock rollback perform fail-open FIFO release and end admission. A removed handler replays while the channel remains active; on channel close/inactive/disconnect the owner fails unresolved promises with `ClosedChannelException`, releases reference-counted messages, and refuses writes into a dead world.
- All administrative actions, write callbacks and downstream packet/promise completions occur only on the owning Netty event-loop thread. No JNI, game World, live player input or packet rendering is accessed here.

## Tests

The actual ForgeGradle Minecraft 1.8.9 JDK8 test runtime executes eight real Netty `EmbeddedChannel` JUnit tests, covering: unarmed pass-through and promise completion; two original held packet/promise pairs and release; a non-movement packet with an empty hold queue; deterministic expiry; 65-write overflow fail-open and ordered promise completion; active handler-removal replay; channel-close failed original promise without network write; and foreign-thread ownership rejection. CI verifies all eight testcases and asserts the exact test-only packet-owner classes are absent from the distributable mod JAR.

## Incompleteness and next admission gates

The test-only outbound mechanism does not yet expose the full original Lag Range/Inv Walk delay or UI settings. Source-native `LagRangeSourcePacketProposal.hold` and `TimerRangeSourcePacketPayload.hold_packets` still do NOT control real traffic, and original packet type selection and timing are not independently validated. To enable any of this in the playable client safely, the next stage must verify client-to-Netty verdict transfer, exact source packet event order, protocol phase, asynchronous timeout scheduling, world reset and packet cancel/delivery semantics. The current Forge client remains read-only. PR #2 draft/unmerged; PR #1 untouched, no original decrypted code committed.