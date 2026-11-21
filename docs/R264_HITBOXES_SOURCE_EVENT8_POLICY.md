# R264 — Hitboxes source-derived read-only event8 policy

Verified Java reconstruction source:
Slinky-Verified-SourceBuild.zip/HitboxesModule.java (848 bytes).
The class subscribes to semantic event8 and, when delivered,
sets event.enabled=true and event.expand=(double)number("expand").
Its status text appears only when expand is configured.
Decrypted native source registrations:
- src/gen/mod_hitboxes.cpp SHA256
  de47010ecd1e6ca56e92893c90d29e0492886fb1cd810d539f19f419a9fb9d36
- src/gen/mod_hitboxes_aux.cpp SHA256
  9789a48d53e5a55451b248b0517d7e96fd4aa4e5850cd15fe9bc2706a4a4bb66

The generated native source proves a numeric setting/slider and a
status label, but NOT a native event8 priority or authentic runtime
input hook. First-party C++ only reproduces the event value proposal,
with exact float-to-double conversion, optional status flag, and a
finite-value safety gate. It does not expand entity hitboxes, override
a Minecraft ray, mutate a player or send packets.

The new R264 offline test checks enabled and disabled delivery,
status independence, fractional expansion and invalid values.
No original game behavior acceptance is claimed.
