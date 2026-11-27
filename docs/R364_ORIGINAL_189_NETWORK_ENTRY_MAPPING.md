# R364 — Source-backed original 1.8.9 network-send entry verification

Private retained `OpenSlinky-main/src/data/rdata.s` and
`src/engine/gui/jni_ids.cpp` have separate protocol paths for two
historical Minecraft variants. In original variant 1 used for
Minecraft 1.8.9, the source JNI table maps:
- `NetworkManager` to obfuscated class `ek`
- `Packet` to obfuscated class `ff`
- `NetworkManager.sendPacket(Packet)` to `ek.a(Lff;)V`

The other variant uses `scheduleOutboundPacket` with a
`GenericFutureListener[]` argument, and must not be mixed into
the original 1.8.9 source-event3 ingress.

R364 extends R357/R361's independent SHA-1 and size-verified official
Mojang 1.8.9 JAR audit to verify the exact `ek.a(Lff;)V` method,
owner, descriptor, instance-method accessibility, executable
`Code`, and the existence of exact `ff` Packet class.
Synthetic negative controls reject the wrong packet type, swapped
variant, wrong NetworkManager owner and wrong Packet class.
The official JAR is temporary and deleted after the audit.

**This is static structural evidence only.** No original packets
are intercepted, delayed, recorded, replayed or dispatched to JNI.
A future network-owner implementation must separately establish
class-loader identity, source event3 timing, bounded packet metadata,
exact client-thread handoff, overflow failure and restoration before
enabling any live side effects. Existing Forge R339 packet capture
remains independent/read-only; this commit doesn't route it into the
original standalone injector.

Original ghost-client 1:1 parity is still incomplete.
PR #2 remains draft/unmerged; PR #1 unchanged.
