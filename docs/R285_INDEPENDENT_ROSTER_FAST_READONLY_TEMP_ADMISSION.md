# R285 — independent TEMP-only roster profile without Clutch scanning

R278 and R283 initially reused R237's TEMP gameDir admission, but
requiring the R237 Clutch source probe forced unnecessary per-tick
world and candidate-block reads before the read-only roster/JNI
observer could execute.

R285 introduces a narrowly scoped, JDK8-only RosterIsolationGate
for a separate **independent roster-only** profile:

- Isolated existing TEMP directory whose immediate folder name
  starts with slinkywinky-r278-independent-forge- and has a
  nonempty unique suffix.
- Dummy offline session username R278Offline.
- Sprint observation explicitly enabled.
- No R212 game action trial and no R208 auto-world trial.
- GameDir and TEMP root canonicalized via toRealPath; nested,
  missing or outside-TEMP paths are rejected.
- R278 sampling still requires actual local singleplayer WorldClient
  and Forge PlayerTickEvent.END, and R283 still needs its separate
  opt-in flag with an owned Java/native session.

Set slinkywinky.r278RosterReadOnlyProbe=true and optionally
slinkywinky.r283NativeRosterReadOnlyProbe=true. Leave
slinkywinky.r237ClutchReadOnlyProbe=false to avoid the R237 scan.
The ORIGINAL R237-gated path remains supported for diagnostics.

The R285 Java8 CTest probes good/bad TEMP paths, identity, partial
prefix, nested directories and incompatible actions; the Forge
JAR workflow requires the admission class to be packaged.

**No throughput/latency improvement has been benchmarked.** This
structurally removes the unnecessary R237 Clutch scan from the
independent roster-only execution path. No live Minecraft observation
or original Slinky behavior is claimed.
