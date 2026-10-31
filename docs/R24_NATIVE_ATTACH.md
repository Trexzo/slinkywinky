# R24: True injectable architecture (no Forge dependency)

**New primary deployment:** `slinkywinky_loader.exe` + `slinkywinky_agent.dll` + bundled `attach-classes` helper. Build with full **Java 8 JDK** (including `tools.jar`), 64-bit Windows/MSVC, and CMake `-DSLINKYWINKY_BUILD_ATTACH=ON`.

The native loader invokes the **supported Java 8 Attach API** for a chosen JVM process under the same user. `Agent_OnAttach` inside the DLL uses JVMTI to enumerate loaded classes and requires a Minecraft 1.8.9 class signature (`net.minecraft.client.Minecraft` or obfuscated `ave`) in normal mode. It probes the C++ core using a disabled/no-target decision, then returns. No Forge runtime dependency, no arbitrary remote-thread injection, no hidden hooks, no EULA/anticheat bypass or proprietary binary included.

**Commands:**

    slinkywinky_loader.exe list "C:\Program Files\Eclipse Adoptium\jdk-8.0.504.1-hotspot"
    slinkywinky_loader.exe attach PID "C:\Program Files\Eclipse Adoptium\jdk-8.0.504.1-hotspot"

Both commands require an installed **JDK8**. `attach` requires a numeric PID selected from `list`. The native DLL and attach Java classes sit next to the loader executable. The helper rejects non-Minecraft process descriptors in production. Test-only mode needs `SLINKYWINKY_ATTACH_TEST=1` and deliberately exercises an inert Java test VM.

**Current acceptance boundary:** A successful loader attach means native code is loaded into the JVM and the JVMTI/native core probe works. It **does not yet install game-thread callbacks or combat hooks**, render a GUI, or prove identical Slinky behavior. The DLL cannot be safely live-unloaded on demand just because the attach command returned; the JVM owns the loaded native agent until process exit.

The previous Forge project is **experimental test infrastructure only**; it remains under `experimental/forge189` and is not built/required by the new injectable targets.

## Build

    cmake -S . -B build -A x64 -DSLINKYWINKY_BUILD_ATTACH=ON -DSLINKYWINKY_BUILD_JNI=ON
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure

The optional `native_agent_attach` CTest launches a disposable local Java 8 process and attaches the new native DLL without Minecraft.