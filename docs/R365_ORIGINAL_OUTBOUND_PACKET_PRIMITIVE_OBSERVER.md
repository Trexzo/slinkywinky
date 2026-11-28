# R365c — Original outbound packet primitive sampler (offline)

Production classes implement a strict original obfuscated
`ff`-Packet/`ip`-C03PacketPlayer reader:
`ip.g:Z` (moving) and `ip.a,b,c:D` (position).
A valid movement packet returns only immutable kind/position/coordinate
primitives. All other `ff` packets are **UNKNOWN** (never guessed as
attack, sprint or movement). Invalid classloader, wrong field
type, NaN, infinity, world bounds or a non-`ff` object fail closed.

The read-only `Vanilla189OutboundPrimitiveQueue` wraps the already
tested R339 `SourcePacketReadOnlyQueue`: 256-row capacity, FIFO
sequence and poisonous overflow, non-blocking on the JVM native
controller, source owner-thread drain/close. An outbound callback
may be invoked by any original packet-sender thread; it never
persists `Packet`, `NetworkManager`, `Channel`, `World` or
`Entity` references.

The pure ASM5 `Vanilla189OutboundEntryWeaver` recognizes only
`ek.a(Lff;)V` and inserts a no-result, read-only callback at the
start. It never replaces a send invocation, changes method return,
retains/cancels a packet, or delays Netty.

**Not yet wired into original client**: this stage has offline
contract tests only. A real original `ek.class` loaded-class
fingerprint and exact transformer/restore lifecycle are required
before any original packet callback can run. R365a official
JAR audit produces this fingerprint without exporting game bytes.
No source event3 is sent from a live game at this stage.
