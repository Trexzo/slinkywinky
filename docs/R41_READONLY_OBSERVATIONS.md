# R41 — strict Minecraft field sampling, no actions

The Java 8 late-injected tick callback now resolves exact known Minecraft 1.8.9 names for the singleton instance, current world, player, selected ray-hit entity, XYZ coordinates and yaw/pitch. Mapped class fields are read-only; no mouse or world mutation occurs. Unknown classes, fields, missing instance or invalid numbers reject sampling and initiate callback cleanup.

It sends `timeMs`, an identity-based world/player generation token, flags (`world`, `player`, `ray target`), six coordinates and two camera angles into the first-party JNI library. The native `nativeObserveFrame` verifies array lengths, flags, finite coordinates, positive session generation and exact disabled-module output. The engine is invoked from the Minecraft `Client thread`, with ALL settings disabled, then its per-tick test context is closed.

The fixture `AttachTarget` simulates an active world, player and ray-selected entity, and the disposable-JVM regression requires `NATIVE_OBSERVATION_PASS world=1 player=1 target=1`. The isolated Minecraft menu acceptance requires the observation marker but may show `world=0 player=0 target=0`, which does not prove in-world observation. No gameplay action or packet interception is implemented. Field mappings for a fully obfuscated production 1.8.9 JAR are not yet certified.

The existing callback classloader admission, 20-tick bound, exact original-bytecode SHA-256 restoration, and explicit experimental opt-in remain mandatory.
