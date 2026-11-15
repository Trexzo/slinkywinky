# R224 — actual Minecraft 1.8.9 player motion → Java8/JNI → native Strafe proposal

Date: 2026-11-15. Source branch `integration/openslinky-fd9c592-reference`, draft PR #2. GUI branch PR #1 remains separately owned and unchanged in R224.

## Goal and exact scope

R222 reconstructed a first-party, independent pure Strafe proposal and its exact native candidate descriptors (event21 priority1000 snapshot, event22 priority100 steering). R223 added a read-only conflict resolver for Strafe vs Fast Accel on event22. Neither had observed real Minecraft motion through a native Strafe JNI callback.

R224 closes this *observation* gap: the **real Forge 1.8.9 Client PlayerTickEvent.END** supplies an actual `EntityPlayerSP`'s `MovementInputFromOptions`, yaw, sprint flag, on-ground state and **actual horizontal/vertical motion** to the first-party Java 8 → JNI → C++ `StrafeController`. That policy computes a copied X/Y/Z motion proposal. **It never changes player motion, packets, gameplay state or sends clicks.**

This test is **not** an original Slinky event21/22 hook or binary-parity claim. The Forge test samples sprint and steering from **one** owner client tick, whereas the original candidate has two independently registered callbacks with different priorities. The R223 motion-owner arbitration is still separate from the live JNI query, and no production action adapter is connected.

## Native owner and JNI protocol

The existing `SprintObservationProbe` Java class receives `observeStrafe(...)` for the source-contained owner session:
- The native session is created and retired together with the actual existing Sprint module-control JNI session.
- The native C++ `StrafeReadOnlySession` independently pins the creating `std::thread::id`, world generation and strictly increasing player-tick ID.
- The real Sprint JNI world observation must **already** have admitted the same world generation; foreign threads, closed handles, stale worlds, repeated ticks, non-finite coordinates and malformed JNI arguments are rejected.
- A world/player absence explicitly resets the read-only native controller; a new world generation resets old steering state.
- The native method returns a fixed five-double receipt: admitted flag, proposal-present flag and X/Y/Z **value only**; neither Java nor JNI has a motion-write implementation in this API.

The existing Java 8 `SprintObservationProbeSmoke` now tests actual JNI output (straight-motion vector, preserved Y), duplicate tick denial, wrong thread denial, world exit/reentry, stale generation denial and no-world absence with a real compiled native DLL. The native library is still `slinkywinky_jni.dll`.

## Real Forge 1.8.9 source and isolation

The new explicit `-Dslinkywinky.r224LocalStrafeReadOnlyTest=true` mode is admitted only in a **fresh TEMP** game directory named `slinkywinky-r224-independent-forge-*`, an exact dummy identity `R224Offline`, the independent verified Forge 1.8.9 Java 8 launcher, and the already existing `slinkywinky.sprintObservation=true` gate. It refuses any R212–R217 action-mode combination.

Inside only that disposable world, Minecraft creates the isolated creative flat save `R224-local-only`; the test sets a synthetic forward **key-binding state** using an existing test-only stimulus. It **does not fake** the game's `MovementInputFromOptions`: Java insists the actual Minecraft class and real player/world identity are present. The player moves naturally under that accepted vanilla input and actual position/motion fields are read from its own world. The new native Strafe query occurs **after** the existing read-only real-world Sprint observation at every admitted Forge PlayerTickEvent.END. Once enough real motion and proposals have been measured, the acceptance frees the owned synthetic key, closes native owner/preview and **prevents auto-restart**.

The launcher explicitly SHA256-pins the **exact** R224 reobfuscated JAR and verifies the official original Minecraft 1.8.9 vanilla hash. The test creates no files in the user's real Minecraft profile and does not install an agent or injector. Only local TEMP-owned saves/logs are written.

## Verified live Windows acceptance

Device: DESKTOP-LTXXO4K; Windows 11, Temurin Java 8, official installed Forge 11.15.1.2318 and Minecraft 1.8.9; independent Java8 process with exact dummy identity and `%TEMP%\slinkywinky-r224-independent-forge-LIVE2` as its game directory.

R224 final reobfuscated Forge JAR SHA256:
`ba8759cec99148cf14b31ad4e4b2c7ec06abe387a9730e3a0bb8586d60b2d345`

Original vanilla Minecraft 1.8.9 JAR SHA256:
`14f0d96d1a56fb4f5c3b2233d00699525893fe5ce3dcf181e7de59120595d298`

Actual independent local-world logs show the ordered markers:
- `R224_STRAFE_ISOLATION_GATE_PASS`
- `R224_LOCAL_WORLD_REQUESTED` and `R224_LOCAL_WORLD_PLAYER_READY_PASS`
- `R224_REAL_VANILLA_INPUT_ARMED source=MovementInputFromOptions synthetic_key=forward`
- `R224_REAL_PLAYER_TICK_READONLY_STRAFE_JNI_PASS samples=50 movingSamples=50 proposedSamples=50 distance=10.233705335225267 gameMotionWrites=0 source=Forge_PlayerTick_END_and_vanilla_motion`
- `R224_READONLY_STRAFE_CLEANUP_PASS key_released=1 native_owner_closed=1 preview_closed=1 auto_restart=0 gameplay_motion_writes=0`

The world save `saves/R224-local-only/level.dat` was verified. stderr was empty. The exact-independent-JAR launcher loaded the first-party Forge mod, including the new JNI protocol, and the client preview also passed the older R208 real in-world Sprint observation receipt (32 player ticks).

The first live R224 candidate reached 50 proposals and 10.23 blocks. Reviewing its lifecycle exposed an existing automatic preview restart after a completed non-R214 test. **R224 fixed that restart**, strengthened teardown asserts and rebuilt/re-pinned the JAR. The second, final LIVE2 acceptance passed the same 50 native motion proposals and 10.23 blocks with the corrected cleanup. The old LIVE1 Java test process was stopped only after validating its exact TEMP world/username command line.

Independent `tests/r224_in_world_strafe_receipt.py --root <fresh R224 gameDir>` verified the real log, launcher preflight, exact JAR SHA and real saved world. `--selftest` passed **23 deliberately mutated negative controls**, including forged movement/proposal counts, missing source markers, wrong hashes, invalid source input evidence, missing native cleanup, a fabricated gameplay-motion write and invalid ordering. Fake receipts are not counted as real Minecraft acceptance.

## Limits and next work

The Strafe native method is currently an **observational preview on real client state**, not original native event-bus registration in the injected client. Native `StrafeController` is a separately authored policy, not a proven 1:1 reimplementation of commercial Slinky exceptional behavior. Real client X/Z *positions* changed because of a deliberately synthetic vanilla forward key; there are **zero Strafe-initiated motion writes**. No hardware key event was verified.

R224 does not prove real in-world Fast Accel vs Strafe selection, original Slinky callback equivalence, the Minecraft JNI GL11 active-only ArrayList, physical ClickGUI mouse routing, or a production injection package. Those remain separate acceptance milestones.

Both PRs remain draft/unmerged; PR #1 was not edited by R224. Private original binaries, game saves and logs were not committed to GitHub.

## Full local verification

- Real Windows MSVC x64 `slinkywinky_jni.dll` rebuilt with R224's new native method; exit 0.
- Real Java 8 `r204_java8_native_sprint_observation_roundtrip` passed with the new `R224_JAVA8_IN_WORLD_STRAFE_READONLY_JNI_SMOKE_PASS` assertion and foreign-thread/stale-generation tests.
- Full JNI-enabled Windows CMake Release build and **62/62 CTests PASS**, including `r224_real_world_strafe_receipt_negative_controls`.
- The separately preserved R223 standalone reference-compatibility suite was previously **99/99 MSVC Release and 99/99 Debug PASS**. No R223 policy implementation files were altered in R224.
- ForgeGradle 2.1.3 using official Gradle 4.10.3 and Java 8: `clean build reobfJar` **BUILD SUCCESSFUL**, all 11 tasks completed without compilation error, final SHA256 pinned above.
- Exact LIVE2 process no longer existed when checked after accepted receipts. Original Minecraft profile and libraries were only read, not overwritten.

The new JAR and local JNI DLL are **test artifacts** kept on the authorized Windows PC. They are not uploaded or included as source in GitHub PR #2. Fresh PR #2 GitHub merge CI against the current GUI PR #1 base remains a separate acceptance requirement.
