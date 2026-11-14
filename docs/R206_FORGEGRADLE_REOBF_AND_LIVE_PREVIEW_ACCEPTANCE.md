# R206 — ForgeGradle reobfuscation and isolated Minecraft 1.8.9 launch

Date: November 14, 2026. Trexzo/slinkywinky.
Branch: integration/openslinky-fd9c592-reference, draft PR #2.
Parallel reconstruction/r1-core-fidelity PR #1 was not modified.

## Goal

R205 produced only a development-mapped Java 8 Forge 1.8.9 preview JAR.
R206 restores the historical build system, performs actual ForgeGradle
reobfuscation, and tests the read-only preview in an isolated
Minecraft 1.8.9 development client.

This is not a complete original Slinky recreation or a validated
production mod installation.

## Official toolchain restoration

The user's Windows PC has JDK8 but initially no Gradle 4.x on PATH.
An earlier cached Gradle 6.9.4 could not resolve the legacy
ForgeGradle 2.1-SNAPSHOT dependency in offline mode.

Downloaded official Gradle 4.10.3 into an isolated temporary directory
and verified its archive against the published official SHA256:

- Gradle archive: gradle-4.10.3-bin.zip
- Official SHA256: 8626cbf206b4e201ade7b87779090690447054bc93f052954c78480fa6ed186e
- Java runtime: Temurin 1.8.0_504 / Windows 11 x64
- Gradle executable: %TEMP%\slinkywinky-r206-toolchain\gradle-4.10.3\bin\gradle.bat
- Locally resolved ForgeGradle 2.1-SNAPSHOT JAR SHA256:
  29f4f9a4b7ad917937d6ca761404ed4c56ee2a716cbfdd190b9aa99f25eb4695

The actual Gradle tasks command completed successfully and exposed
ForgeGradle reobfJar, runClient, setupCiWorkspace and setupDevWorkspace.
No global PATH, default Minecraft installation, existing mods folder,
or parallel source branch was changed.

## Real ForgeGradle build: PASS

From the isolated PR #2 checkout at
%TEMP%\slinkywinky-r191-isolated-check\experimental\forge189,
the actual Gradle 4.10.3 + JDK8 invocation ran:

    gradle --no-daemon --console=plain build

The build completed successfully, including compileJava, jar,
reobfJar, assemble and build tasks: BUILD SUCCESSFUL / exit 0.

Produced:
experimental\forge189\build\libs\slinkywinky-preview-0.1.0-preview.jar

First accepted package SHA256:
f331bb3367236b15846288b3afb3c4d054d0e1b5bab941290e78dff121769e87

The package contains 12 first-party Java8 classes. The ForgePreview
class has classfile major version 52, the runtime-visible Forge @Mod
annotation descriptor Lnet/minecraftforge/fml/common/Mod;, and
the expected ForgeGradle-remapped Minecraft method func_71410_x.

This is actual reobfuscation evidence, unlike the development-only
mapped JAR from R205. It does not prove that the packaged artifact
works in an independent production installation.

## Reproducible acceptance runner

New first-party repository script:
tools/run_r206_forgegradle_reobf_acceptance.ps1

The script takes an explicit Java8 installation, official Gradle ZIP
and fresh output path. It verifies the official ZIP SHA256 before
extracting Gradle, rejects overwriting prior evidence, checks the
Gradle/JDK8 versions, reruns ForgeGradle build with --rerun-tasks,
requires successful reobfJar logging and examines the produced JAR.

It verifies class entries, Forge annotation, Java8 classfile major
version, the SRG remapped Minecraft reference, and absence of selected
direct gameplay input/motion/packet setter calls in the first-party
bytecode (scoped audit only). It copies the verified JAR into the
acceptance directory; no Minecraft launch is performed by this runner.

Accepted runner result: R206_FORGEGRADLE_REOBF_ACCEPTANCE_PASS, exit 0,
12 packaged classes.

Acceptance artifact remains on the user's Windows PC:
%TEMP%\slinkywinky-r206-official-reobf-acceptance\
slinkywinky-forge189-r206-REOBF-TEST.jar

Artifact SHA256:
f331bb3367236b15846288b3afb3c4d054d0e1b5bab941290e78dff121769e87

No original commercial Slinky binary, public OpenSlinky native compiled
object, or user-private original callback bytes were distributed.

## Real isolated Minecraft 1.8.9 development client: PASS

A separate offline Gradle runClient invocation launched Minecraft 1.8.9
in the temporary project's run directory with Java8 and the previously
verified first-party R204 JNI Release DLL. Only read-only diagnostic
JVM flags were supplied:

    -Dslinkywinky.renderProbe=true
    -Dslinkywinky.startupProbe=true
    -Djava.library.path=<local R204 JNI Release directory>

Actual Minecraft/Forge log contained:

- Forge Mod Loader has successfully loaded 4 mods
- [SlinkyWinkyPreview] initialized - read-only
- [SlinkyWinkyPreview] JNI_STARTUP_PROBE_PASS
- [SlinkyWinkyPreview] RENDER_EVENT_PROBE_PASS frames=30

The test Java process owned a Minecraft 1.8.9 window; the legacy
ForgeGL renderer reported AMD Radeon RX 7900 GRE.
The JNI startup probe called a first-party native library, and
the mod's render listener received genuine Forge game events.

Important: Gradle runClient used the development classpath rather
than independently loading the packaged reobfuscated JAR. Actual
in-world Sprint state, original event 17/21 rates, and GUI fidelity
were not tested.

## Intentional test-client termination and log caveats

After obtaining the positive startup/render receipts, the test
window received a targeted Windows CloseMainWindow request. It
closed its window but the Java VM remained alive; the same
identity-verified test process (PID 21060) was explicitly stopped.
No other Java process or SpawnPK process was terminated.

Because the game was stopped while runClient waited for it, the
interactive Gradle runClient task eventually printed BUILD FAILED.
This marks the intentional end of the game process, NOT a failed
earlier ForgeGradle build, and does not negate its logged
initialization and render event receipts.

The historical development environment also emitted warnings
about binary patch/signature metadata and a
---- Minecraft Crash Report ---- diagnostic header, after which
Forge completed loading and the required probes passed.
The session was not confirmed clean under natural shutdown or
long-duration operation.

## Remaining integration and parity gates

1. Validate the reobfuscated JAR in an isolated *non-development*
   Forge 1.8.9 installation.
2. Test the opt-in read-only Sprint observation with a real world
   and player. The current RenderTick-based input still substitutes
   getWalkSpeed for the original native movement event speed.
3. Establish true event 17/21 timing, JNI owner-thread handling,
   session transitions, and clean unload/reinject lifecycle.
4. Verify the original ClickGUI against authenticated reference
   pixels, input ownership and settings, separately from Forge
   source compatibility.
5. Continue reconstructing gameplay callback implementations,
   subject to authorization, source provenance and applicable
   game-server rules.

Offline coverage is unchanged:
66 mapped descriptors / 171 candidate native subscriptions /
16 first-party offline module policies / 22 callback bindings /
149 unimplemented native subscriptions.

PR #2 remains draft/unmerged and PR #1 untouched.
Final-head GitHub Actions must be checked separately.
