# R282 — Java 8 roster receipt -> native Anti Bot through real JNI

R281 established native owner-bound event0 decisions from strongly
validated value snapshots, but did not expose a JNI entry point.

R282 introduces RelationshipRosterNativePreview, a JDK8-only Java
facade over an explicitly created native JNI session. It receives
the *immutable R276 RelationshipRosterSampler.Receipt*, packs ordered
UUIDs, UTF-16 source names, movement samples, age and visibility
flags, and calls a new native entry point in the EXISTING
slinkywinky_jni library.

The native JNI code:

1. Rejects malformed dimensions, metadata, invalid Boolean flags
   and non-UTF-16/null/oversized source names before dispatch.
2. Pins the session to its owner Java/native thread.
3. Maps the Java receipt into an R281 RosterSourceSnapshot, retaining
   UUID bits exactly (including signed 64-bit UUID values).
4. Enables Anti Bot **only**, routes its decision through the existing
   ModuleExecutionBridge (R271 and R274 source UUID fix), and refuses
   stale generations/ticks and owner swaps.
5. Returns versioned, UUID-echoed BIT-2 rejection proposals.
   Neither Java nor JNI writes RelationshipService or any game data.
6. Closes the native session explicitly on the owner thread.

**Teams is intentionally unavailable from this facade.** The R276
Forge receipt lacks full original TeamsModule scoreboard/player-list
ChatComponentText evidence. R281 refuses to invent that metadata, so
the native JNI only previews Anti Bot until the real inputs exist.

The r282_java8_native_antibot_roster_roundtrip CTest compiles Java8,
loads the actual C++ JNI binary, and checks UUID fidelity,
real source UUID mode, movement accumulation, world reset, stale
receipt rejection, wrong thread, bad settings and close.

This proves a Java8-to-native executable roundtrip using FAKE roster
objects, not a launched Minecraft process. R278 still has only a
read-only Forge sampler; a separately gated live JNI invocation and
real runtime acceptance remain necessary.
