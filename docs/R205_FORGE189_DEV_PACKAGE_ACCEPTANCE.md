# R205 — Hash-pinned Forge 1.8.9 development-only packaging acceptance

Date: 2026-11-14. Repository: Trexzo/slinkywinky, draft PR #2
(\`integration/openslinky-fd9c592-reference\`). Original GUI/runtime PR #1
(\`reconstruction/r1-core-fidelity\`) remains untouched.

## Why this milestone

R204 added a read-only Java8/JNI/native Sprint observation roundtrip
and opt-in Forge RenderTick preview, but only the Java sources compiled
against the cached Forge 1.8.9 APIs. An actual Gradle-built reobfuscated
mod JAR or a live Minecraft launch had not been verified.

This milestone adds an explicit, repeatable **development-only JAR**
packaging and bytecode audit on the user's authorized Windows computer.
It is not a production Minecraft mod build, injection workflow or
verified real-game execution.

## Source and pinned local dependencies

New first-party script:
  tools/run_r205_forge_dev_package.ps1

It builds only five first-party Java8 sources:

- java/dev/slinkywinky/GameHost.java
- java/dev/slinkywinky/HostSettings.java
- java/dev/slinkywinky/NativeEngine.java
- java/dev/slinkywinky/SprintObservationProbe.java
- experimental/forge189/src/main/java/dev/slinkywinky/forge/ForgePreview.java

No original commercial Slinky binary or third-party Java classes are
packaged. JDK8 compiler reported \`javac 1.8.0_504\` on the connected PC.
All three mapping/library JARs are **required to match exact SHA256**
before the script executes javac:

| Library | SHA256 |
|---|---|
| Forge \`forgeBin-1.8.9-11.15.1.2318-1.8.9.jar\`, stable_22 development mappings | \`e9eff72c1684f87767e42cf9edf391b74c4a2c5cabc51f8be29f2a20c5f5892d\` |
| LWJGL 2.9.4 nightly 20150209 | \`833e721817f70d1445eec13d8ce5a86e12af70efac5014356302e2bbc68b3fe2\` |
| Guava 17.0 | \`8c36a80ea613d0b6b8040a17cf837c5bbe3677bc1b06a058a6c174fdb787ebbc\` |

The script is offline and never downloads a dependency, launches
Minecraft, invokes an external build plugin or modifies the user's
existing game installation. It refuses to overwrite previous evidence.

## Actual Windows result

The **final accepted** local package was created under:

  %TEMP%\slinkywinky-r205-forge-dev-final\

with the name:

  slinkywinky-forge189-r205-DEV-ONLY.jar

Verified local JAR SHA256:

  6d5157533cb48c7a224c66784f910885da7fd65cc8ec5bcd1c13f88a109eea66

Reported marker:

  R205_FORGE189_DEV_PACKAGE_PASS classes=12

The 12 generated Java8 classes include ForgePreview, GameHost,
HostSettings, NativeEngine, SprintObservationProbe and their nested
classes. \`jar tf\` verified that all required classes are present and
no third-party packages were bundled.

\`javap -verbose\` verified ForgePreview's **classfile major version
52** (Java 8) and the runtime-visible \`@Mod\` annotation descriptor
\`Lnet/minecraftforge/fml/common/Mod;\`.

A bytecode audit across all packaged classes checked the absence
of common direct game-state setter invocations, including
\`KeyBinding.setKeyBindState\`, \`setSprinting\`,
\`setPlayerWalkSpeed\`, \`setVelocity\`, and \`sendPacket\`.
No such invocations were found. This is a scoped compiled-call
check, not an assurance of absolute runtime safety.

Local negative controls additionally passed:

- Existing output directory was refused (no overwrite).
- Deliberately incorrect dependency path/hash was rejected before build.

The result and dependency fingerprints were written to:

  %TEMP%\slinkywinky-r205-forge-dev-final\R205_LOCAL_DEV_PACKAGE_EVIDENCE.txt

No compiled third-party JAR or native private binaries were uploaded
to GitHub.

## Why this JAR is NOT installable production Forge

The pinned \`ForgeBin\` classes expose **development names**. This
source-only package has **not** been reobfuscated into production
Minecraft 1.8.9 member/class mappings. It is not appropriate to copy
into a normal production Forge mods folder expecting it to work.

The ForgeGradle project has no \`gradlew.bat\` wrapper; a Gradle
executable is also not on PATH. A cached Gradle 6.9.4 distribution
was found and probed using Java8 in **offline** mode. Configuration
failed because its resolver reported no cached usable
\`net.minecraftforge.gradle:ForgeGradle:2.1-SNAPSHOT\` version,
even though some ForgeGradle files exist in the local cache.

No unsupported Gradle distribution was installed, no remote
dependency was fetched, and no full ForgeGradle \`reobfJar\`,
\`runClient\` or actual Minecraft client startup was performed.

The development-only JAR hash may vary between invocations because
ordinary JAR/class timestamps are not normalized. This script is
**reproducible as an offline build procedure**, not asserted to
yield identical package bytes across separate timestamps.

## Remaining 1:1 reconstruction gaps

1. Obtain and verify a compatible pinned ForgeGradle toolchain and
   fully reobfuscate/validate a proper test JAR.
2. Launch it only in an isolated authorized Minecraft 1.8.9 dev
   environment and verify observation receipt, JNI DLL loading and
   unload/focus/world transitions.
3. Replace the Forge RenderTick speed proxy with accurate
   event-17/21 timing and inputs before parity conclusions.
4. Implement/validate actual session-owned game actions separately
   and keep user-physical input and world cleanup safe.
5. Continue comparing ClickGUI to authenticated original visuals
   and finish remaining module behavior/subscriptions.

Coverage is **unchanged** from R203/R204: 66 mapped module identities,
171 candidate native subscriptions, **16 first-party offline module
policies, 22 offline callback bindings and 149 not yet implemented**.
The existence of this JAR does not establish more in-game modules.

PR #2 stays draft/unmerged; PR #1 was not edited.
GitHub Actions final-head status is independent of the local
successful build and must be checked before CI claims.
