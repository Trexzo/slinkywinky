# R22 - Windows Minecraft 1.8.9 startup and opt-in JNI probe

On an authorized Windows 11 machine with Temurin JDK8, Visual Studio/MSVC and an installed Forge 1.8.9 development environment, the repo was cloned into a new temporary directory. The original `.minecraft` installation was left unchanged.

Results: MSVC x64 Release build and CTest **8/8 passed**. The Forge 1.8.9 Gradle `build` task succeeded locally. The isolated ForgeGradle `runClient -x getAssets` launched Minecraft, registered mod id `slinkywinky`, printed `[SlinkyWinkyPreview] initialized - read-only`, reported successful loading of 4 Forge mods, then exited with status 0. The observed log did not show a Minecraft crash.

**What is still unverified:** A manual F9 interaction, the JNI library actually called inside Minecraft, playable combat, settings GUI, and equivalence to any original Slinky version. The success above is mod initialization and clean process exit, not behavioral parity.

New in R22: the **opt-in** JVM system property `-Dslinkywinky.startupProbe=true` causes the mod initializer to load the newly built `slinkywinky_jni` library and perform a safe, no-target, zero-action decision roundtrip. It rejects ABI mismatches and active decisions. It prints `JNI_STARTUP_PROBE_PASS` only if the native call returned a correctly disabled decision. It does not touch Minecraft game input, rotate, attack or send packets.

The probe is **off by default**. Run it only in an isolated development JVM with a matching 64-bit native DLL on `java.library.path`. A missing or mismatched DLL produces a visible failure only when deliberately enabled.