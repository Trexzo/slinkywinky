# R366c — Actual original Mojang NetworkManager entry instrumentation

A new **independently authorized** finite original-only mode
`minecraft-network-source-readonly` requires already-loaded
`ave` and `ek` classes from the **same Java classloader**.
Both must be modifiable, and the original `ff`/ `ip` source packet
classes must have strict exact-name/type-compatible metadata.
The transformer validates `ave.class` SHA-256
`430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82`
and independent original `ek.class` SHA-256
`0b898cf41a16e432dd4e873ed3bfbaf2536c149d44269910f31dca5f242d9436`
before instrumenting either class. Both are derived from the exact
Mojang 1.8.9 original client, SHA-1
`3870888a6c3d349d3771a3e9d16c9bf5e076b908`.

The packet-specific ASM5 transformer matches only
`ek.a(Lff;)V` and emits one `onOriginalOutboundPacket(manager,packet)`
read-only callback at the original method's entry. It does NOT
alter vanilla send behavior, return value, packet contents, Netty
thread, channel, queue or timing semantics. The callback catches
all errors rather than throwing into NetworkManager.

`Vanilla189OutboundPrimitiveQueue` captures **only** original
`ip` C03 movement position flags and `a,b,c` doubles, or an
UNKNOWN category for every other `ff` Packet. It stores no
packet, manager, entity, world, channel or JNI refs. The existing
R339 bounded, poisonous 256-item source queue accepts snapshots
from original senders on any thread; 32 are drained per game tick
and processed only by the existing JNI Lag Range event3 on the
game/client owner after a native event18 tick.
A native event3 HOLD is reported only as a **hypothetical** counter,
never applied to packet delivery.

The session is finite (20/600 tick variants). On game-thread
retirement the primitive queue, native source owner, clock fence
and reflection caches close before asynchronous retransform
restores **both** original classes. The verifier counts exactly
one original `ave` and one original `ek` class and compares both
input fingerprints. A missing class, mismatched digest, uncertain
transform/restore or incomplete/expired packet stream aborts
the diagnostic and preserves the reattach lock.

**Important limitations:** NetworkManager sender identity is not yet
bound to the actual client's active NetHandler/channel; a different
original NetworkManager in the same classloader could contribute
primitive observations. This diagnostic is not authenticated source
packet ownership, native callback phase parity, or a functioning
network packet hold/replay system. Source e3 uses a client-thread
drain after the original send entry; it does not execute at the
original on-wire callback time. No live original game process
acceptance is claimed without manual testing.
