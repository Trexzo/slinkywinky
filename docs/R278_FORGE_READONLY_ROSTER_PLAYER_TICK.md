# R278 — opt-in Forge 1.8.9 real roster observation hook

R278 wires the read-only R277 Minecraft playerEntities Port to
R276's owner-thread Java 8 roster receipt on PlayerTickEvent.END.

The probe is OFF by default. To admit it the operator must set
slinkywinky.r278RosterReadOnlyProbe=true as well as the preexisting
slinkywinky.r237ClutchReadOnlyProbe=true and
slinkywinky.sprintObservation=true, inside the original isolated
TEMP gameDir (/slinkywinky-r237-independent-forge-) and dummy
R237Offline session. The Forge Port additionally requires local
singleplayer client world; no remote multiplayer path is allowed.

F9 starts and stops the sampler alongside the existing preview.
A valid 20-frame read prints only aggregate remote player COUNT,
generation and an explicit writes=0 marker. It logs no names,
UUIDs, locations or raw game objects. A stale or invalid receipt
stops the preview; no relationship flags, entities, inventory,
rotation, input or packets are mutated.

The separate GitHub ForgeGradle 2.1 / JDK 8 workflow successfully
built and reobfuscated the R278 Forge 1.8.9 preview JAR, and R280
adds explicit packaging checks for the new roster classes.
This does NOT establish that a real Minecraft game fired the
PlayerTickEvent callback or emitted a correct live roster receipt.
An isolated runtime launch and observation are still REQUIRED.
