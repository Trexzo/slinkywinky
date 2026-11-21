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

This Forge preview source is NOT part of the default GitHub CI.
R276's separately tested fake-port owner guards do not establish
that Forge 1.8.9 compiles or the game fires this callback.
Real isolated Forge build and runtime receipt are still REQUIRED.
