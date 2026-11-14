# R208 — Real in-world Sprint JNI observation in isolated Forge 1.8.9

Date: 2026-11-14. Repository: Trexzo/slinkywinky.
Branch: integration/openslinky-fd9c592-reference, draft PR #2.
Parallel PR #1 remained untouched.

## Scope and rationale

R207 demonstrated that an independently launched Forge 1.8.9
instance could discover and initialize our actual *reobfuscated*
SlinkyWinky Preview JAR from a clean, temporary mods folder.
However, it only tested Forge startup and render events on the
main menu, not a player/world sample.

R208 adds an explicitly opt-in *local singleplayer acceptance mode*
and proves that the game's observed world/player state crosses the
first-party Java 8 -> JNI -> C++ Sprint observation path while
Minecraft is running a real integrated-server world.

This is a read-only **Sprint gameplay diagnostic**: no synthetic
movement, key presses, packet manipulation or player setters were
performed by Sprint. The **test-only world bootstrap** is the
deliberate exception to a strictly read-only game lifecycle: it
creates a new creative-mode flat singleplayer world **only** in an
isolated Windows TEMP game directory under the R208 test flag.
It never creates or changes a save in the user's normal Minecraft
profile.

## Changes on PR #2

1. experimental/forge189/src/main/java/dev/slinkywinky/forge/ForgePreview.java

   New JVM gate: -Dslinkywinky.r208LocalWorldTest=true.

   The opt-in test checks that the active Minecraft game directory
   is under java.io.tmpdir, its path contains the exact
   slinkywinky-r208-independent-forge- test prefix, the session
   username equals R208Offline, and the read-only Sprint observer
   is enabled. Otherwise it refuses to initialize its test mode.

   From the normal Forge 1.8.9 main menu, after 30 client ticks
   it launches a single new local creative flat world using a
   fixed test seed and a save name R208-local-only. Once its
   client-side world and player objects are present, it starts
   the normal first-party read-only preview automatically rather
   than synthesizing a user F9 keypress.

   It logs a one-time acceptance marker after 32 **admitted**
   in-world Sprint observations crossing Java/JNI/native code.
   The marker records render frames, client ticks, elapsed
   milliseconds and latest key/speed **proposals**.

   The default mod behavior is unchanged when the test flag is
   absent; manual F9 remains available. Neither this flag nor
   these automated world actions are intended for normal user
   installations.

2. tools/run_r207_independent_forge_loader.ps1

   R207's independent loader retains its original default SHA
   and R207Offline identity. New explicit switch
   -R208LocalWorldTest selects the pinned R208 JAR hash, the
   R208Offline dummy identity, the test-only JVM flag, and an
   additional TEMP-only output-path naming check. The normal
   R207 independent launcher remains supported.

3. tools/verify_r208_in_world_sprint_receipts.ps1

   A new source-controlled receipt verifier validates the exact
   reobfuscated JAR SHA, preflight marker, Forge loader identity,
   local integrated world and player logs, and the positive
   JNI/render observation marker. It validates plausible counts,
   one-shot world creation, the exact test JVM's gameDir,
   dummy identity and command-line flags. It can close the game
   window and stop **only** that identity-verified Java8 test PID.

   It emits a machine-readable local text receipt. No private
   game log, player account, Minecraft binary, DLL or compiled
   third-party source was uploaded to GitHub.

## Actual Windows build and in-world evidence

Connected device: DESKTOP-LTXXO4K, Windows 11, Temurin Java 8.

Using the already verified *official Gradle 4.10.3* and cached
ForgeGradle 2.1-SNAPSHOT / Forge 1.8.9 toolchain, the updated
ForgePreview source passed an actual offline ForgeGradle build,
including compileJava and reobfJar.

The accepted new R208 reobfuscated Java8 JAR SHA256:

    5084bfce416abc8527f7e0e61f104378e104496e0eee40d6828a2b37970a5c0f

It was copied to a dedicated TEMP artifact folder and loaded
through the real *independently installed* Forge 1.8.9 profile,
not ForgeGradle runClient or development compiled classes.

The separate game instance was created under:

    %TEMP%\slinkywinky-r208-independent-forge-LIVE1\

Its mod was staged in that instance's mods folder, and the
new local world was created under its own saves/R208-local-only
directory. The program used a dummy offline account named
R208Offline; no real account credentials were copied.

Launcher preflight:
- 48 installed Forge/vanilla Java libraries read from the user's
  existing Minecraft library cache.
- 14 native Windows DLLs unpacked into the dedicated TEMP folder.
- Staged JAR exact SHA verified before launch.
- Original .minecraft mods, settings and worlds not modified.

The real Forge log then emitted:

- R208_ISOLATION_GATE_PASS
- JNI_STARTUP_PROBE_PASS
- Forge Mod Loader has successfully loaded 4 mods
- RENDER_EVENT_PROBE_PASS frames=30
- R208_LOCAL_WORLD_REQUESTED
- R208_LOCAL_WORLD_PLAYER_READY_PASS
- R208_IN_WORLD_SPRINT_JNI_OBSERVATION_PASS
  renderSamples=32 clientTicks=15 elapsedMs=320
  latestKeyProposal=true latestSpeedProposal=false

The local integrated-server log separately confirmed that
R208Offline joined the world using a local (Netty) connection
and loaded the normal Minecraft world dimensions.

These are **real Minecraft world/player observations** and actual
JNI-native Sprint decision calls. They are not synthetic CTest
fixtures, invented log messages or game inputs applied by the bot.

At the point of verification, the player was stationary and
Sprint proposed a held key but no horizontal speed change.
This is an uncalibrated snapshot, not authenticated original
Sprint movement-behavior acceptance.

The verifier ended with:

    R208_REAL_WORLD_SPRINT_JNI_OBSERVATION_PASS
    render=32 ticks=15 elapsedMs=320

It wrote a local receipt to:
    %TEMP%\slinkywinky-r208-independent-forge-LIVE1\
    R208_IN_WORLD_SPRINT_ACCEPTANCE.txt

The independent client window was asked to close. Since its
JVM retained threads after the window closed, the verifier
stopped **only** the previously identity-verified test PID
16340 and recorded that state. No unrelated Java or SpawnPK
process was stopped. The main test stdout contained legacy
Forge startup signature/diagnostic warnings but continued
through all positive Forge and JNI markers; stderr was empty
at the acceptance check.

## Negative controls and regression acceptance

Four deliberate R208 launcher safety tests PASS:

1. Existing R208 preflight instance refused (no overwrite).
2. The user's original .minecraft path refused as gameDir.
3. Previous R206 JAR refused in R208 mode (SHA mismatch).
4. Wrongly named R207-prefix TEMP path refused in R208 mode.

Both rejected attempts that targeted nonexistent TEMP directories
left those output paths absent.

Rechecked unchanged local regression suites in both configurations:

- First-party source-only C++ CTests: 89/89 PASS Debug and Release.
- Pinned 66-entry native compiled-manager CTests: 7/7 PASS Debug
  and Release (separate from the Sprint gameplay implementation).
- Java8 -> JNI -> native offline Sprint roundtrip: 1/1 PASS
  Debug and Release.

R207's final GitHub Actions were separately confirmed:
5/5 workflow names completed successfully. R208 final-head
GitHub CI must be checked independently.

## Exact fidelity limitations and next steps

R208 is a crucial *real in-world instrumentation milestone*,
but it does not establish that the original commercial
Slinky Sprint module is working 1:1 in Minecraft.

- The data still arrives from Forge RenderTick and a client
  tick count, **not** recovered original native event-17/21
  game-thread callbacks. Their ownership, ordering and precise
  timing remain unverified.
- The movement speed value still uses the local client's
  getWalkSpeed() as a **diagnostic proxy**, not the authentic
  original event-21 MoveState.speed.
- No game inputs or actual Sprint movement modifications were
  applied by the observation adapter. Real gameplay action
  ownership and cleanup need separate implementation and tests.
- The local world session was stopped after positive receipts;
  natural clean shutdown, disconnect, world switches and
  repeated JNI unload/reload remain unverified.
- The auto-world creator is test-only and has been gated behind
  a strict dummy identity and temporary gameDir. It is not
  intended for any production mod release.
- GUI appearance, animation, settings and authentic original
  binary/module parity remain separate unfinished work.

Coverage accounting is **unchanged**:
66 mapped module descriptors; 171 candidate native callback
subscriptions; 16 independently developed first-party *offline*
module policies; 22 callback bindings; 149 not implemented.

R208 improves live observation acceptance, not the numeric count
of reconstructed original gameplay callbacks.

PR #2 remains draft/unmerged; PR #1 stays independently managed.
