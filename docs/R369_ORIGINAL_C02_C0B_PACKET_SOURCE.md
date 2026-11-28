# R369 — Original source attack and stop-sprint packet mappings

Recovered owner-private C++ reference `src/data/rdata.s` and
`src/engine/gui/jni_ids.cpp`, 1.8.9 variant 1, identify:

- `C02PacketUseEntity` as `in` (Lag Range labels **any** C02 use-entity
  as its attack-release packet category, without inspecting its action)
- `C0BPacketEntityAction` as `is`
- C0B action enum field `is.b:Lis$a;`
- C0B action enum owner `is$a`
- For the original Lag Range packet code, **ordinal 4** is specifically
  the stop-sprint release condition.

R369's original-JAR read-only audit requires the exact obfuscated
owners and field descriptor in the already SHA-1/size-verified
Mojang 1.8.9 client, in addition to the prior C03, click,
active-NetHandler and NetworkManager checks. Negative controls
reject wrong class owner, field type and missing enum.

This deliberately differs from guessing that every entity-action
packet represents stop sprinting: other C0B action ordinals stay
UNKNOWN to the native Lag Range event3 bridge. An original C02
is classified as a source **use-entity/attack-release category**, not
as definitive proof of a hit or a successful server-side attack.

R369 is structural until tested end-to-end through the immutable
packet ingress and real native event3. No original bytes are
distributed; no on-wire packets are changed, held, canceled or replayed.
