# R27: opt-in native JVMTI game-thread callback proof (no Forge)

`slinkywinky_loader.exe attach PID JDK8_HOME --tick-probe` enables an **experimental, read-only** JVMTI breakpoint for the unmodified Java Minecraft 1.8.9 `runTick()` method (or SRG `func_71407_l`). The process is selected explicitly. No Forge event bus is used.

The agent must first recognize Minecraft class data. It obtains the JVM's JVMTI environment, requests the `can_generate_breakpoint_events` capability, finds the exact candidate method in the loaded class and arms a breakpoint on its first bytecode location. **No bytecode rewrites, mouse actions, packet changes or camera changes occur.**

At the first in-game tick callback, it runs a native disabled-state C++ engine probe on that exact JVM thread. After 20 callbacks, it immediately removes the breakpoint and disables the JVMTI event, logging `GAME_THREAD_TICK_PROBE_PASS`.

This is a DIAGNOSTIC ONLY: Java breakpoints may affect performance and must not be used during actual multiplayer play. If the runtime has only obfuscated method names or JVMTI breakpoints cannot be safely installed, admission fails instead of guessing a method or changing JVM state.

The ordinary `attach PID JDK8_HOME` mode remains passive and does not install game-thread callbacks. A production game-thread event bridge and reliable vanilla 1.8.9 obfuscated-symbol resolver remain to be implemented before combat or GUI code can be activated.