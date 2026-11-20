# R239 — real Java 8 JNI transport into first-party native Clutch policy

Date: 2026-11-20. Work scoped exclusively to SlinkyWinky draft PR #2.
Original pinned source: `Skid-Technology/OpenSlinky@fd9c592`,
`src/logic/block/clutch.cpp`, 167-candidate rdata table.
The private candidate decrypted DLL is not included or executed.

## Implemented

- `ClutchNativePreview`: Java 8 owner-thread session wrapper around a
  JNI library that returns **value-only** selected block/face/index and
  visited/count/cost fields. It cannot send a game click or apply rotation.
- `clutch_world_observation_jni.cpp`: independent JNI session table and
  owner-bound gateway consuming the exact 167-source-prefix block IDs
  produced by R237. The R238 `ClutchObservationOwner` validates tick,
  generation, opaque same-session identities, player AABB, base calculation,
  bounds, ground-depth/candidate ordering. Failures quarantine the session.
- The JNI method invokes the *actual first-party*
  `clutch_plan_original()` from R235 using a session-owned source walk.
  It never holds Java object pointers, calls back into the game, sets
  inputs, changes state, or emits packets. The preview uses an explicitly
  deterministic testing seed, **not** proof of commercial RNG parity.
- The existing opt-in R237 Forge `PlayerTickEvent.END` preview now
  passes each real Java-observed 167-position world snapshot into the
  native C++ planner. It reports aggregate read-only native plan counts
  over 20 ticks. All prior R237 TEMP/dummy offline admission rules remain.
  No auto-world construction or synthetic key events are introduced.
- `ClutchNativePreviewSmoke` and `run_r239_clutch_native_observation.cmake`
  compile Java 8 and call the actual native JNI DLL/SO: source index5
  positive, air negative, ground-block negative, duplicate tick negative,
  foreign-thread negative, and proper resource closure.
- Source-level native gates still have their independent R238 CTest
  negative controls on Linux/Windows.

## Exact limitations

1. R239 does **not** attach the actual native event-18 Clutch callback
   to vanilla Minecraft. The Forge preview is a separately opted-in
   diagnostic on PlayerTickEvent.END, not authentic source event timing.
2. Prior-block retention needs the separate source-ordered randomized
   aim followed by Java five-argument ray tracing, then a second
   owner-checked native result. For now `request.previous!=null` is
   explicitly rejected by the JNI wrapper. No ray-hit evidence is faked.
3. R239 never issues clicks, camera rotations, hotbar changes or block
   placements. The selected candidate is a read-only receipt.
4. The operator's Windows desktop is offline. No new in-world Minecraft
   real runtime acceptance, exact game process injection, visual fidelity,
   or original decrypted-binary differential result was performed.
5. Native RNG is seeded with a test fixture, not source session entropy.
   Snapshot values are bounded but not currently accompanied by an
   externally authenticated original-JVM capture proof.
6. This only advances **Clutch**. A full client still needs the
   remaining native callbacks, packet/event timing, GUI parity and
   actual module gameplay integration.

## Verification

Target CTest: `r239_java8_native_clutch_source_planner` with a compiled
real JNI library, and Linux GitHub workflow
`.github/workflows/r239-clutch-jni-source.yml`. It is important to
read completed run receipts before claiming a pass. Existing Windows
JNI and original-source workflows are separate gates. Keep both PRs
draft/unmerged.
