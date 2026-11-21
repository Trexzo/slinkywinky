# R276 — Java 8 read-only roster receipt boundary

The R275 shared Teams/AntiBot event-0 bridge requires the same world, local owner and ordered UUID roster. R276 introduces a dependency-free Java 8 sampler that can receive Minecraft player observations through a separately supplied Port.

Only the creator thread can observe or close the sampler. Every snapshot validates world/owner object identity, client-thread assertion, remote UUID uniqueness, finite coordinates, player age and owner membership before committing. Both owner/world changes produce new opaque positive tokens. Regressed ticks and mid-sample world changes are refused.

Receipt.Remote contains only immutable Java values; no Minecraft entity or world object is exposed to C++ or retained in the receipt. The tests use fake JDK-only player sources and are run through a Java 8-compatible CTest entry when a Java toolchain exists.

This is **not** a live Minecraft Forge hook, nor is it native JNI wiring or verified runtime parity. The next step is a strict opt-in Forge 1.8.9 Port in an isolated offline world.
