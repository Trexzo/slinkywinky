# R283 — opt-in real Forge PlayerTick → Java8 → JNI → native Anti Bot proposals

R278 already samples a real Minecraft 1.8.9 playerEntities roster
into immutable R276 Java values. R282 added a Java8/JNI/C++ read-only
Anti Bot preview, and R283 now **opt-in connects** these two chains.

Admission conditions:
- slinkywinky.r283NativeRosterReadOnlyProbe=true
- slinkywinky.r278RosterReadOnlyProbe=true
- slinkywinky.r237ClutchReadOnlyProbe=true
- slinkywinky.sprintObservation=true
- Existing R237 TEMP gameDir and dummy R237Offline identity,
  local integrated singleplayer client world, F9 preview toggle
- slinkywinky.r283AntiBotUuidMode is 0, 1 or 2 (default 0).
  Mode values follow the recovered Java's counterintuitive
  version-nibble equality rule and affect proposals ONLY.

On Forge PlayerTickEvent.END, the R276 immutable receipt is supplied
to the native C++ R281 event0 adapter via the Java8 JNI R282 wrapper.
R283 checks exact world/owner/generation/tick and ordered UUID
echoes, calculates aggregate proposed rejected count, and logs only
that number at frame 20. It never writes original Minecraft entities,
RelationshipService, scoreboard, input, mouse or packets.
Shutdown closes JNI on the same owner thread before releasing
the Java roster sampler.

CI builds the experimental ForgeGradle 2.1/JDK8 preview JAR with real
Minecraft 1.8.9 mappings and confirms that the JNI facade class is
packaged. Separate JNI CTest runs native decisions on synthetic
rosters, not a game process. **Real-world runtime acceptance
has NOT been obtained.** The existing headless startup workflow is
not an R283 roster receipt and cannot count as one.
