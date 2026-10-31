# R38 — reproducible Windows Minecraft game-thread acceptance

The standalone injector's read-only tick hook was verified **twice in a single live Java 8 Minecraft 1.8.9 JVM**, including 20 callbacks and exact bytecode fingerprint restoration in each cycle.

To repeat on a controlled Windows development machine with the native agent already built:

```powershell
python tests/windows_minecraft_tick_acceptance.py `
  --workspace "C:\path\to\isolated\slinkywinky" `
  --jdk8 "C:\path\to\jdk8" `
  --gradle "C:\path\to\gradle-2.14.1\bin\gradle.bat" `
  --forge-project "C:\path\to\isolated\slinkywinky\experimental\forge189"
```

This manual-only test refuses to start if it finds another attachable Minecraft/GradleStart JVM, launches a disposable ForgeGradle 1.8.9 process in the specified workspace, attaches the **standalone** native injector and Java Instrumentation agent twice, requires all admission/tick/restoration markers, verifies both restored SHA-256 fingerprints match, and stops only the spawned test process tree. Existing `.minecraft` installations and unrelated processes are not modified. Forge is **not part of the injector**; it merely launches a real Minecraft JVM for acceptance.

This test does **not** establish packet/input processing, GUI functionality, original commercial fidelity, or safe native-agent DLL unloading. Production remains disabled; experimental `--hook-probe` requires explicit `SLINKYWINKY_EXPERIMENTAL_HOOK=1`.
