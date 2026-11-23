# R300 — Minecraft 1.8.9 read-only ESP world-snapshot admission

R297 Chest ESP, R298 Item ESP and R299 Player ESP are **offline C++ models**,
not on-screen features. R300 introduces a common immutable Java 8 receipt
from a real Forge 1.8.9 client-side singleplayer world. The port uses:
- `loadedTileEntityList` encounter order and **world block IDs**, including
  the chest/beacon IDs needed by R297;
- `loadedEntityList` encounter order, filtering only to EntityItem and
  preserving old/current positions, item IDs, stack counts and dead flags;
- `playerEntities` encounter order, UUIDs, self/dead flags, positions,
  health, max health, absorption and hurt time.

The Java8 sampler is a source-independent **read-only boundary**. It refuses
foreign threads, absent worlds, duplicate/stale ticks, world/player changes
during collection, duplicate player UUIDs, nonfinite coordinates/health,
missing self, oversized collections, closed sessions and invalid data.
It issues generation-bound world/player tokens and unmodifiable copied rows.
World transition, missing world and F9 stop invalidate the retained observer.
No Minecraft objects or world/player pointers are retained in a Receipt.

R300 is opt-in through `-Dslinkywinky.r300EspReadOnlyProbe=true` and
`-Dslinkywinky.sprintObservation=true`, in an **independent TEMP-only
singleplayer profile** with `/slinkywinky-r300-independent-forge-` in
the game directory and the dummy offline username `R300Offline`.
F9 enables the diagnostic; after 20 eligible actual PlayerTick.END
observations it emits `R300_FORGE_ESP_WORLD_SNAPSHOT_RECEIPT`, with
counts but **no player names, UUIDs or coordinates**.

**Important gaps:** this does not connect R297–R299 C++ controllers to JNI,
does not reproduce original Chest event-0 versus Item/Player event-18
cadence, does not supply recovered target/friendly color resolution,
does not implement screen projection, item count text or any OpenGL calls,
and is not evidence of verified in-world rendering or full Slinky parity.
Forge is a development-only harness; the production vanilla injector
remains separate. Both PRs remain draft.
