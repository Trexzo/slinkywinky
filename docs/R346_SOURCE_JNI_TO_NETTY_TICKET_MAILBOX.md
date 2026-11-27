# R346 — Real source-JNI client tick → immutable one-slot Netty policy candidate

R344–R345 established **real** Minecraft 1.8.9 inbound and outbound Netty packet + ChannelPromise ownership in **test-only** handlers. The missing safe bridge is transferring native source module state, owned by the Minecraft client thread, to Netty's foreign event-loop without calling JNI or dereferencing Minecraft `World` on that loop.

## Implemented

- `SourcePacketControlMailbox` is a pure Java 8, bounded **one-slot** `AtomicReference<Ticket>`, published solely by one authenticated client-thread producer. The ticket contains strictly primitive identity/epoch fields (opaque channel token, JNI world/player tokens, monotonic generation, tick, sequence, target entity ID, time and Backtrack/Lag Range candidate-active flags). It owns NO packet, entity, world, channel, GL or JNI references.
- The only source of active flags is the actual already-executed native `ModuleExecutionBridge` Java8 JNI receipts for Backtrack event18 and Lag Range event18. Both receipts must match the SAME world/player/generation/tick, correct opcodes, and strict boolean shapes. A reused source tick or old-world native JNI receipt cannot be published twice or in a newer generation.
- A foreign Netty reader may call `inspect(expectedChannel, expectedWorld, expectedGeneration, monotonicNow)`. It returns a ticket only if every opaque token agrees and the tick is at most **100ms old**; future timestamps, old channels, stale generations, closed or revoked publishers all return null. Publisher authority is single-thread enforced. The reader never performs JNI or controls actual traffic.
- Critically, a native source tick's `active=1` is **NOT the source packet hold verdict**; this mailbox labels it merely a non-actionable **candidate**. No live packet gate consumes a ticket, and R344/345 test-only packet owners are still never installed by `ForgePreview`.

## Verification

- New Java8 + real JNI CTest `r346_java8_native_netty_policy_mailbox` uses the actual R337 native four-module world tick, checks byte-for-byte receipt state projected into a ticket, publishes/polls from different threads, rejects foreign producers, replayed ticks, wrong channel/world/generation, future/expired stamps, revocation and retired session. World generation replacement transitions to a new valid ticket while the old ticket is inaccessible.
- ForgeGradle JDK8 compiles/packages the purely passive mailbox and checks no network/JNI invocation occurs there. All earlier R344/345 actual Netty inbound/outbound owner classes remain **test source set only** and are refused by the JAR packaging gate.

## Outstanding for active ownership

Native original per-packet verdict generation and exact synchronous timing, valid reach admission in the real client, safe netty event-loop authorization, actual protocol-phase handshake, scheduled timeout, cross-world revocation of any in-flight *real* packets, original packet type selection and objective local singleplayer/remote multiplayer acceptance remain unverified. These tickets cannot be treated as permission to hold packets. PR #2 stays draft and unmerged, PR #1 unchanged; no original decrypted assets committed.