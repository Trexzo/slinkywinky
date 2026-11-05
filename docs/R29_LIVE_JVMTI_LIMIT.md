# R29: Java 8 dynamic attach capability limitation and next gate

On the connected Windows 11 desktop, `slinkywinky_loader.exe attach PID JDK8_HOME` successfully attached the native C++ DLL to an actual Minecraft 1.8.9 Java 8 JVM, recognized the Minecraft class, and ran its disabled native core probe. **Forge is not part of the loader or DLL.** The target was launched through an isolated Forge development environment only to provide a real Java 8 Minecraft process for this test.

Opt-in diagnostic `--tick-probe` was also attempted inside a real Minecraft process after the game had started. Native agent's JVMTI `AddCapabilities(can_generate_breakpoint_events=1)` returned **98 / JVMTI_ERROR_NOT_AVAILABLE** in the JVM live phase. The agent returned failure and installed **no breakpoint or game hook**; ordinary attach remained unaffected.

Therefore neither a genuine game-thread callback nor combat, GUI, input or packet integration has been proven from late attach. Do not claim that Java 8's JVMTI breakpoint method works here or infer that the native loader is a complete client.

Viable future engineering paths require separate research and verification: a supported attach-time Java Instrumentation/retransform adapter, a different read-only native event integration, or startup-mode JVMTI capabilities (which would no longer be pure late attachment). Each needs isolated exact-Minecraft-version tests, safe cleanup and a proven mapping for obfuscated vanilla classes. None has been implemented by this milestone.

CI keeps validating the explicit Windows native attach package with a temporary Java 8 process, and adds a **negative control** requiring production-mode rejection of an unrelated Java JVM.

The experimental Forge client and Linux headless workflows remain optional manual test harnesses, not shipping dependencies. Earlier Linux headless graphical/asset failures remain documented.