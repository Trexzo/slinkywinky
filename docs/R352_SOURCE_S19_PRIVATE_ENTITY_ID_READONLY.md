# R352 — Private S19 entity-ID read-only source parity

## Evidence
The private `c++_possibly_OpenSlinky-main.zip` contains:
- `src/engine/jni/jni_world_accessors.cpp`:
  `java_get_s19_packet_entity_status_entity_id` calls `GetIntField`
  directly; it does **not** call `getEntity(World)`.
- `src/engine/gui/jni_ids.cpp` resolves that field using the original
  `S19PacketEntityStatus_entityId` JNI ID table.
- `src/data/rdata.s` maps the candidate's three name variants:
  `a` (obfuscated), `entityId` (MCP) and
  `field_149164_a` (SRG), each an integer field.

Previously `ForgeSourcePacketTap` deliberately recorded
`S19PacketEntityStatus` with `entityIdKnown=false` because
the public accessor `getEntity(World)` would require a prohibited
Netty-thread World lookup. This unnecessarily lost source-available
packet identity, preventing live Backtrack target-status feedback
proposals.

## R352 correction
- The existing passive Netty Observer lazily resolves an exact
  candidate-name `int` field at class initialization using the
  supported Java 8 reflection API. Read-only `getInt(Packet)` occurs
  on the existing Netty tap; no World/Entity/JNI access or mutation.
- Source IDs **greater than zero** are copied to the existing primitive
  `SourcePacketReadOnlyQueue` (zero/unknown/missing mapping remains
  untrusted, conservatively). No reflective write in production.
- The production Observer still calls `ctx.fireChannelRead(msg)`
  with the **identical packet object**, and all outgoing writes pass
  through unchanged. Reflection failures poison only the *diagnostic
  queue*; they never block or release a real packet.
- ForgeGradle's existing `ForgeSourcePacketTapEmbeddedTest` includes
  a disposable `S19PacketEntityStatus` with a known private ID,
  verifies same-object forwarding/no duplication and confirms that
  source event2 category 1 can now receive the true ID, without a
  World lookup. The existing default/unknown S19 test remains intact.

## Remaining
A positive in-world client-thread Backtrack feedback receipt still
requires a real eligible target and R349/R350 opt-in source observation.
No original network hook, packet control, original 1:1 parity or
production injectable gameplay has been established.
**PR #2 stays draft and unmerged, PR #1 unchanged.**
