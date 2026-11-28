# R365 — Verified original NetworkManager fingerprint prerequisite

This stage extends the official 1.8.9 SHA-1 and size-pinned Mojang
JAR inspection to produce only the **SHA-256 of an independently
verified `ek.class` byte array**. The previously checked
`ek.a(Lff;)V` and `ff.class` original signatures are required before
the digest is accepted.

The CI output `R365_ORIGINAL_189_NETWORK_MANAGER_CLASS_SHA256` is
intended for a subsequent exact-class instrumentation gate, not to
identify third-party modified clients by heuristic names. No JAR
or class bytes are copied into Git or an artifact.

A standalone packet callback must not be enabled without checking the
exact original `ek.class` loaded-class bytes before a retransform,
then proving exactly one matching `a(Lff;)V` and restoring both
the original Minecraft and NetworkManager classes safely.

R365 does not attach or mutate any network packet, does not execute
native event3 and does not change module behavior.
