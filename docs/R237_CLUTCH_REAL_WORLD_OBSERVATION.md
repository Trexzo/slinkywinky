# R237 — source-ordered Clutch world observation (Java 8 / Forge 1.8.9)

Date: 2026-11-20. Implementation branch PR #2; PR #1 remains unchanged.

## Why

R235/R236 reproduced the pinned reference's source-order Clutch candidate
selection and aim geometry in first-party C++17, but the test harness supplied
block IDs, player AABB and prior-block visibility. This was not real Minecraft
world sampling. R237 adds the *read side* of that missing boundary.

The relevant provenance is the user-held candidate decrypted image and the
existing Library comparisons (October 31 and November 8). The private
archive itself was **not** accessible in this work session, so no new raw
binary comparison or claim of authenticated commercial fidelity is made.
The independent fixed source table is taken from already-pinned
`reference_compat/include/slinkywinky/clutch_candidate_table.hpp`
(OpenSlinky `fd9c592`, source `rdata.s`). CI checks its full 167-entry
order against the Java 8 sampling table, not just counts.

## What actually executes

- `ClutchWorldSampler` is a new Java 8 value-only observation session. It
  samples original 167 packed offsets, indexed by the eight original source
  prefix lengths (1,6,19,43,79,123,163,167). It uses
  `floor(((AABB.minX+AABB.maxX)/2, AABB.minY-.99, ...))` as source base.
- Block ID **zero vs nonzero** is the original source predicate; this is not
  an invented general `isSolid()` classification. Optional depth samples
  stop at the first nonzero block before candidate iteration.
- The session rejects incorrect thread ownership, stale player/world
  identity, missing identity, repeated ticks, changed generation, excessive
  or non-finite coordinates, invalid range/settings, invalid block IDs, and
  any identity change *mid-scan*. An error irrevocably closes the session.
  Snapshots defensively copy their block IDs.
- Original prior-block visibility is deliberately a **second phase**:
  only a previously sampled, nonzero prior block may request a one-time
  `rayTraceBlocks(from,to,false,true,false)` read. The request target must
  come from the native source planner's randomized prior aim. Java never
  assumes a ray hit or invents the aim. A null ray hit is recorded as an
  observed null rather than an unknown.
- `ForgeClutchWorldPort` binds the `Port` to real Minecraft 1.8.9
  `WorldClient.getBlockState(BlockPos)`, `Block.getIdFromBlock()`, and the
  five-argument World raycast. It holds a specific client world/player,
  rechecks their identity and verifies the Minecraft client thread.
- `ForgePreview` includes a **separately opt-in** full 167-position scan at
  Forge `PlayerTickEvent.END`. It samples 20 client ticks and emits one
  aggregate receipt (number of samples and non-air IDs), without logging
  player positions, names, world keys, or block coordinates.

## Admission and reproducible manual acceptance

In an *independently launched* Forge 1.8.9 test JVM, use:

```text
-Dslinkywinky.sprintObservation=true
-Dslinkywinky.r237ClutchReadOnlyProbe=true
```

The game directory must be under the host's TEMP directory with a path
component `slinkywinky-r237-independent-forge-...`. The Minecraft session
must use the dummy offline name `R237Offline`. R212–R217 action modes and
R208/R209/R224 local-world modes are explicitly rejected. The operator
must manually enter a **singleplayer client world**, press F9 to enable the
read-only preview, and keep it in-world for 20 accepted player ticks.
R237 does not auto-create worlds, synthesize keys, invoke Clutch clicks,
change hotbar slots, rotate, or write gameplay motion.

Expected *once successfully executed on the user's Windows Minecraft*:
```text
R237_CLUTCH_ISOLATION_GATE_PASS
R237_REAL_WORLD_CLUTCH_167_READONLY_PASS samples=20 candidateReads=3340 ... gameplayWrites=0 raycasts=0 source=Forge_PlayerTick_END
```

Do **not** count these strings as observed results until a real,
independently pinned Forge/Minecraft run and its receipts are inspected.
The connected Windows desktop was offline during this implementation.

## CI / tests

`tests/java/dev/slinkywinky/ClutchWorldSamplerSmoke.java` covers:
- source table order, prefix and immutable snapshots;
- full 167-cell read plus six-entry source prefix;
- depth scanning / short-circuit before the candidate scan;
- two-phase observed-null ray and repeated/out-of-reach ray rejection;
- duplicate ticks and malformed range refusing before world reads;
- world changes during a block read, subsequent quarantine;
- cross-thread owner refusal, stale player despite a new generation.

The new `r237-clutch-world-sampler.yml` workflow compiles with
`javac --release 8` against Java 8 APIs, and verifies every Java packed
offset against the C++ pinned source table. This is Java 8 bytecode/API
validation on the hosted runner, **not** a Java 8 JVM execution or full
ForgeGradle/reobfJar test. Windows Forge/JNI build and actual in-game
acceptance remain outstanding.

## Remaining integration work (R238+)

1. Provide the first-party C++ `ClutchCandidateWorld` with the matching
   Java block-ID snapshot on an owner-thread, generation/tick-fenced JNI
   exchange. Confirm its entire 167-source-table ordering is preserved.
2. Implement the two-phase **native randomized prior-aim → Java exact raycast
   → native validated result** handoff. Never call raycast before original
   angle/reach guards. Preserve/rollback the R234 shared RNG exactly once.
3. Validate the native event-18 result against the current Minecraft world
   and ownership before any game interaction. Compare controlled original
   traces for aim/snapback, then implement guarded actual placement/clicks
   only in an authorized isolated world.
4. Produce exact Java8/ForgeGradle and Windows native/JNI build receipts,
   then independent Minecraft 1.8.9 real-world acceptance.

This is a **read-only observation integration**, not a finished Clutch
module, not all 66 original Slinky modules and not a playable client.
Keep PR #2 draft/unmerged; do not copy or publish private binaries.
