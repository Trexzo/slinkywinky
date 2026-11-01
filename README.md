# SlinkyWinky — native injectable reconstruction

**Development status: native Java 8/JVMTI injection foundation. Not yet a playable Slinky client.**

The primary Windows architecture is a standalone loader executable and C++ JVMTI agent DLL attaching to an existing Minecraft 1.8.9 JVM **without Forge**. The previously developed Forge preview remains an **optional testing harness**, not a shipping dependency.

See [native injection architecture, build and explicit limitations](docs/R24_NATIVE_ATTACH.md).

The tested C++ combat decision engine, native C API, Java JNI smoke bridge, session and input ownership regressions remain available. They are not yet connected to live vanilla Minecraft client hooks or an in-game GUI. The supplied candidate decrypted DLL is a local research reference, not redistributed or verified as an official artifact; OpenSlinky+ 2.2 is excluded.

## Build

On Windows x64 with a complete Java 8 JDK and MSVC:

```powershell
cmake -S . -B build -A x64 -DSLINKYWINKY_BUILD_ATTACH=ON -DSLINKYWINKY_BUILD_JNI=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The injector targets a chosen process explicitly and checks it is Minecraft. It never silently scans for and modifies arbitrary JVMs. A successful agent attach **does not yet mean a working client**; live combat, game-thread hooks, GUI, clean unload and true original parity still require implementation and verification.

## Vanilla 1.8.9 standalone runtime acceptance

R48 verified **two** consecutive read-only native attachments into an isolated **original obfuscated Minecraft 1.8.9** JVM, not a Forge development client. The standalone loader admitted class `ave`, executed zero-action C++ observations on the `Client thread`, closed the session, and restored matching bytecode fingerprints. See [R48 evidence](docs/R48_VANILLA_RUNTIME_ACCEPTANCE.md).

The existing late JVMTI breakpoint approach remains unsupported on Java 8 (`JVMTI_ERROR_NOT_AVAILABLE 98`). The working hook uses Java Instrumentation and JNI and is explicitly developer-gated. It defaults to 20 ticks; R59 adds optional, separately authorized **120- or 600-callback** read-only windows (`--hook-probe=120` / `--hook-probe=600` with `SLINKYWINKY_EXTENDED_DIAGNOSTIC=1` in the attaching process). See [R59 diagnostic bounds and acceptance](docs/R59_EXTENDED_NATIVE_OBSERVATION.md). The actual client is **not yet playable**: live world/target observation, real combat decisions, ClickGUI/HUD, full native-agent unload and verified reference parity are incomplete.
