# R289 — isolated TEMP-world real roster JNI acceptance mode

Opt-in R289 creates a disposable flat creative singleplayer save named
R289-local-only under the independently validated R285 TEMP gameDir.
Requires R278/R283 roster read-only flags, Sprint observation enabled,
dummy R278Offline session, and forbids competing action/Clutch/world tests.

After actual client player/world entry, the existing Forge PlayerTick
read-only roster and Java8/JNI/native Anti Bot proposal path must produce
32 owned observations. R289 logs only counts and generation, stops and
closes all native/Java owners, and refuses preview auto-restart.
Only this explicitly isolated test creates a world; roster decisions
never write Minecraft entities, packets, inputs or relationship flags.

The R289 Python selftest generates FAKE logs and negative controls
only. Real acceptance remains unverified until a source-controlled
verifier checks an actual Forge log, exact JAR identity and isolated
singleplayer level.dat. R288 headless startup is an independent gate.
