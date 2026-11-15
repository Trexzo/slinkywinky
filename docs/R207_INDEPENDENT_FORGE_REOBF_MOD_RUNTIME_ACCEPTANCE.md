# R207 — Independent installed Forge 1.8.9 loader accepts reobfuscated preview

Date: 2026-11-14. Repository: Trexzo/slinkywinky.
Integration branch: integration/openslinky-fd9c592-reference.
Draft PR #2; separate PR #1 remains untouched and unmerged.

## Acceptance scope

R206 established a ForgeGradle 2.1 reobfuscated Java 8 JAR and a
successful Forge development-classpath launch. This R207 milestone
tests the actual *packaged reobfuscated mod* in a separately launched
Forge 1.8.9 runtime, with its own temporary game directory and
ordinary installed Minecraft Forge profile libraries.

The runtime was NOT ForgeGradle runClient. It was started directly
through the original installed Forge LaunchWrapper entry point:
net.minecraft.launchwrapper.Launch, with the Forge FMLTweaker.
The sole SlinkyWinky mod artifact was staged under the isolated
test instance's mods folder. The runtime had no development build
classes on its classpath.

## Source-controlled first-party scripts

New script:
  tools/run_r207_independent_forge_loader.ps1

It requires explicit paths to:
- the existing Minecraft profile, used read-only for Forge 1.8.9
  metadata, dependency JARs, natives, the vanilla JAR and assets;
- Java8 home and previously verified first-party JNI DLL;
- exact R206 reobfuscated JAR;
- a nonexisting TEMP game directory.

The script validates the exact reobfuscated JAR SHA256:
  f331bb3367236b15846288b3afb3c4d054d0e1b5bab941290e78dff121769e87

It also verifies the locally reviewed vanilla Minecraft 1.8.9 JAR
fingerprint:
  14f0d96d1a56fb4f5c3b2233d00699525893fe5ce3dcf181e7de59120595d298

The user machine's installed Forge profile is:
  1.8.9-forge1.8.9-11.15.1.2318-1.8.9

The script resolves the Forge and parent vanilla Maven library
metadata, with Forge entries overriding duplicated vanilla
group/artifact dependencies. Windows natives are extracted from
local dependency archives into a new private TEMP folder, capped
at 32 MB per DLL. It copies the single verified first-party mod
artifact to TEMP/mods; it does not copy the user's existing
configuration, credentials, other mods or saves.

The independent launch uses Java8 and a dummy offline username
R207Offline, dummy access token 0 and zeroed UUID. It enables
the existing first-party read-only startup/JNI and render probes.
No Minecraft player, key, movement or packet setter is called
by the preview, and no game server is contacted for a gameplay
session.

New verification and narrowly scoped cleanup script:
  tools/verify_r207_independent_forge_receipts.ps1

The verifier requires the exact expected staged JAR digest,
a TEMP-only isolated game directory, real Forge startup log
markers, and an identity-verified Java8 process whose command
line has the isolated gameDir and no ForgeGradle development
class directory. It can then issue a graceful window-close
request; if legacy Java threads remain, it can terminate only
that exact verified isolated test process.

The verifier writes a local text acceptance receipt; it does
not publish process logs, private paths or game data to GitHub.

## Actual Windows independent-loader run

The successful live test ran on the authorized Windows 11 PC
DESKTOP-LTXXO4K using Java8. The new isolated instance was:

  %TEMP%\slinkywinky-r207-independent-forge-LIVE1\

The preflight reported:
  R207_INDEPENDENT_FORGE_PREFLIGHT_PASS libraries=48 natives=14

The installed Forge runtime found the new TEMP/mods folder
and reported four mods identified. The log contained all
required source-attributed first-party receipts:

- Forge Mod Loader has identified 4 mods to load
- FMLFileResourcePack:SlinkyWinky Preview
- [SlinkyWinkyPreview] initialized - read-only
- [SlinkyWinkyPreview] JNI_STARTUP_PROBE_PASS
- [SlinkyWinkyPreview] RENDER_EVENT_PROBE_PASS frames=30

The independently launched Java process used installed Forge
libraries rather than experimental/forge189/build/classes
or GradleMain. Its visible game window was Minecraft 1.8.9.
The verifier observed no stderr output from that process at
acceptance time (0 bytes); this does not rule out general
legacy Forge warnings in stdout.

Accepted result:
  R207_INDEPENDENT_FORGE_REOBF_MOD_RUNTIME_ACCEPTANCE_PASS

The verification receipt was written locally at:
  %TEMP%\slinkywinky-r207-independent-forge-LIVE1\
    R207_INDEPENDENT_FORGE_RUNTIME_ACCEPTANCE.txt

The test window was asked to close. When its exact Java process
PID 26628 remained alive after the window closed, only that
identity-verified test process was stopped. A subsequent check
confirmed the process was absent. No other Java, Minecraft
or SpawnPK process was stopped.

## Negative controls (all PASS)

The launcher refused all three deliberately invalid cases:

1. Overwriting an existing isolated test directory.
2. Using the user's original .minecraft directory as its writable
   game directory.
3. Accepting the older R205 development-only JAR in place of the
   exact R206 reobfuscated artifact; rejected via SHA mismatch
   before creating an output directory.

The test and verification scripts have not been promoted into
normal startup/install flows.

## Strict fidelity boundaries

R207 conclusively strengthens the packaging evidence: the
reobfuscated, first-party preview JAR can load in an independent
installed Forge 1.8.9 runtime and invoke its read-only native
startup probe and genuine Forge render event listener.

R207 does NOT establish:
- a player in a real singleplayer or multiplayer world;
- authentic event 17 and 21 timing;
- Sprint's actual movement or key effects;
- an in-world sample through the R204 opt-in Sprint adapter;
- clean in-game world switching, unload/reinject or natural
  JVM teardown (shutdown required targeted termination);
- original commercial Slinky GUI and gameplay fidelity;
- safety or compatibility with third-party online servers.

The R204 optional Sprint observer requires the separate F9
preview enable and has not been exercised as actual player-world
gameplay by this independent-loader test.

The reconstructed offline inventory is unchanged:
66 mapped module identities, 171 native subscriptions,
16 independent offline module policies, 22 bound offline
callback decisions and 149 not implemented.

Future integration: opt-in first-party in-world Sprint observation
on game thread, exact event timing and movement speed source,
clean session shutdown and reference-based GUI matching.

PR #2 remains draft/unmerged. PR #1 was not edited.
Final-head GitHub Actions CI should be checked independently.
