# R209 — Real Forge client player-tick Sprint JNI observation

Date: 2026-11-14. Repo: Trexzo/slinkywinky.
Branch: integration/openslinky-fd9c592-reference, draft PR #2.
Parallel PR #1 remained untouched.

## Why this matters

R208 established a genuine in-world Java 8 -> JNI -> C++ Sprint
observation, but it was sampled from RenderTickEvent and its
movement inputs came from general player movement fields. This
created more samples than simulation ticks and obscured timing.

R209 moves only the source-matched *offline Sprint decision
observation* to the Forge 1.8.9 client player tick boundary.
No Minecraft keys, movement values, sprint state or packets are
changed by Sprint.

The pinned Forge 1.8.9 stable_22 API does **not** contain a
net.minecraftforge.client.event.InputUpdateEvent class. The available
TickEvent.PlayerTickEvent provides its actual player identity,
phase and client tick boundary. EntityPlayerSP exposes a
movementInput object with moveForward and moveStrafe fields.
R209 filters PlayerTickEvent.END to:
- event.player **identically equals** Minecraft.thePlayer;
- Minecraft.theWorld and thePlayer exist;
- the world is the client WorldClient (isRemote).
Integrated-server-side PlayerTickEvent callbacks are excluded.

This is an accurate **Forge client player-tick observation point**,
NOT the commercial original Slinky's exact native event-17/21
callback wiring. R204's JNI adapter still computes both modeled
event-17 and event-21 decisions within one admitted observed tick.

## Code changes

1. experimental/forge189/src/main/java/dev/slinkywinky/forge/ForgePreview.java

   - Removed SprintObserver evaluation from RenderTickEvent. The
     existing read-only GameHost RenderTick preview and render receipt
     are unchanged.
   - Added onPlayerTick(TickEvent.PlayerTickEvent) and one
     Java -> JNI -> native Sprint sample per admitted client
     PlayerTickEvent.END, filtered to the actual EntityPlayerSP.
   - Samples movementInput.moveForward/moveStrafe instead of
     general EntityPlayerSP.moveForward/moveStrafing fields.
   - Keeps capabilities.getWalkSpeed() ONLY as the explicitly labeled
     *diagnostic speed proxy* for the modeled event-21 input.
     The authentic native MoveState.speed remains unknown.
   - Captures player-tick sampling count, client-tick count,
     elapsed monotonic time, smallest/largest sample gaps and
     data-only Sprint proposals.
   - When the player disappears, ClientTick can emit a single
     absent-player observation to retire the native session.
     This path is implemented but was NOT exercised in a real
     world-exit/reentry acceptance in R209.
   - New explicit test flag:
     -Dslinkywinky.r209PlayerTickTest=true
     still requires the existing read-only Sprint observation flag,
     a named temporary game directory and the dummy offline
     username R209Offline. Only this opt-in test automatically
     creates a local flat singleplayer test save. Ordinary
     preview operation still requires manual F9 activation.

2. tools/run_r207_independent_forge_loader.ps1

   Added -R209PlayerTickTest in addition to the prior R207/R208 modes.
   This pins a new JAR hash, a dummy R209Offline identity, isolated
   TEMP directory prefix and test JVM flag, without changing the
   old modes.

3. tools/verify_r209_player_tick_receipts.ps1

   Added a source-controlled verifier that checks the exact
   packaged-JAR fingerprint, isolated Forge launch profile,
   source-attributed positive initialization and player-tick
   receipt markers, local world save, sample counters, and the
   identity of the exact Java8 test JVM. It can request a
   graceful test-window shutdown and then stop only that verified
   Java test PID if the old game leaves Java threads alive.

   The verifier produces a local text evidence record and never
   uploads the original game binaries, credentials, native DLLs
   or third-party compiled references.

## Actual ForgeGradle Java 8 build

Using the already accepted official Gradle 4.10.3 and legacy
ForgeGradle 2.1 toolchain on the connected Windows PC, an
offline --rerun-tasks build passed, including compileJava and
reobfJar:

    BUILD SUCCESSFUL / exit 0

New exact R209 reobfuscated first-party test JAR SHA256:

    27b7f8e707f90b3e4a5f5d56a7d7aa1e02aab55829c46fad494eb8b3771f8de2

The test artifact was retained only in Windows TEMP at:

    %TEMP%\slinkywinky-r209-artifact\
      slinkywinky-forge189-r209-REOBF-TEST.jar

Direct JDK8 javap disassembly of ForgePreview verified the
packaged PlayerTickEvent and movementInput references. The
scoped static bytecode checks found no direct
setKeyBindState, setSprinting or sendPacket calls.

The test mode intentionally calls launchIntegratedServer()
only to create a **new isolated TEMP singleplayer test save**.
This is a controlled game-lifecycle action, separate from the
read-only Sprint decision path.

## Independent installed Forge 1.8.9 live acceptance

The reobfuscated JAR was loaded from a **new temporary mods folder**
through the installed Forge 1.8.9 LaunchWrapper runtime—not
ForgeGradle runClient, and not development-class compiled classes.

Preflight: 48 normal library JARs and 14 Windows native DLLs
resolved from the existing Minecraft installation, read-only.
The new isolated instance and local world are:

    %TEMP%\slinkywinky-r209-independent-forge-LIVE1\
    saves\R209-local-only\level.dat

The test used username R209Offline, zero access token and a dummy
offline UUID. No default Minecraft save, user mod, configuration,
SpawnPK client or other Java process was changed.

Positive source-attributed Forge/client markers in the actual
Minecraft 1.8.9 log:

- R209_ISOLATION_GATE_PASS
- JNI_STARTUP_PROBE_PASS
- Forge Mod Loader has successfully loaded 4 mods
- RENDER_EVENT_PROBE_PASS frames=30
- R209_LOCAL_WORLD_REQUESTED
- R209_LOCAL_WORLD_PLAYER_READY_PASS
- R209_PLAYER_TICK_SPRINT_JNI_PASS
  playerTickSamples=32 clientTicks=32 elapsedMs=1476
  minSampleGapMs=0 maxSampleGapMs=125
  inputForward=0.0 inputStrafe=0.0
  proposedKeyPressed=true proposedSpeed=false
  speedSource=getWalkSpeed_proxy

The actual integrated server separately recorded that R209Offline
joined the test world through the local Minecraft Netty connection.
The accepted first 32 player-tick observations took 1,476 ms
(approximately **47.6 ms between accepted samples** when using
31 gaps), near the nominal 50 ms tick cadence. Individual gaps
ranged from 0 ms to 125 ms: legacy Minecraft can process catch-up
ticks or have temporary latency. The intervals do NOT prove the
exact timing/order of the original Slinky native callbacks.

Stationary observed input produced a data-only proposed sprint
key state and no speed proposal; nothing was applied to the game.
No external multiplayer or gameplay server connection was requested.

The independent source-controlled verifier accepted the runtime:
  R209_REAL_FORGE_PLAYER_TICK_JNI_OBSERVATION_PASS
  samples=32 clientTicks=32 elapsedMs=1476 min=0 max=125
  state=EXACT_TEST_PID_STOPPED_AFTER_WINDOW_CLOSE

The receipt is local:
    %TEMP%\slinkywinky-r209-independent-forge-LIVE1\
      R209_PLAYER_TICK_ACCEPTANCE.txt

The test window was closed; then only the same verified leftover
Minecraft Java8 test PID 29860 was stopped. The test PID's absence
was confirmed. Other Java, Minecraft and SpawnPK processes were
not touched.

## Local negative controls and regressions

Four independent launcher rejection tests passed:

1. Existing R209 gameDir could not be overwritten.
2. User's real .minecraft directory could not be reused for output.
3. R208 JAR was rejected in R209 mode via SHA256 mismatch.
4. Incorrect TEMP instance prefix was rejected.

Previously nonexistent rejected output directories remained absent.

On the connected Windows PC, both configurations passed:

| Regression | Debug | Release |
|---|---|---|
| Source-only first-party CTest | 89/89 | 89/89 |
| Pinned corrected 66-entry compiled native-manager CTest | 7/7 | 7/7 |
| Actual Java8 -> JNI -> native Sprint roundtrip | 1/1 | 1/1 |

These earlier tests exercise offline/native model components and
do not themselves prove original gameplay effects.

At report authoring, R208 final-head GitHub Actions are confirmed
5/5 SUCCESS. R209 final-head GitHub CI must be checked separately.

## Remaining gaps: must not be misrepresented

- R209's callback boundary is **Forge PlayerTickEvent.END**, not
  authenticated Slinky native event-17/21 dispatch. Forge 1.8.9
  does not expose a separate InputUpdateEvent. Its actual
  movement-input update ordering relative to the original
  Slinky callback remains to be independently established.
- The C++ bridge *still* runs modeled Sprint update and movement
  callbacks together per player tick, rather than separating
  them at two independently authenticated original event sources.
- getWalkSpeed() is still a diagnostic proxy and not verified
  to match original MoveState.speed.
- Actual Sprint key/game movement effects remain disabled and
  unverified. The test validates **read-only decisions** only.
- World exit/re-entry, game focus changes, clean JVM shutdown
  and JNI session teardown have not been verified live; the
  programmed absence handling is a **pending test gate**.
- The original commercial ClickGUI and gameplay module
  reconstruction remain far from full verified parity.

Coverage accounting is unchanged:
66 source catalog descriptors / 171 candidate subscriptions /
16 first-party **offline** module policies / 22 callback bindings /
149 unimplemented subscriptions.

PR #2 remains draft/unmerged, and PR #1 was not edited.
