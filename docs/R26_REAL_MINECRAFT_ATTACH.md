# R26: independent injectable architecture accepted inside Minecraft JVM

**Validated 2026-10-31 on a connected Windows 11 desktop, Java 8, Visual Studio/MSVC x64.**

Source commit `a140c814` was cloned into a **separate temporary workspace**, leaving the user's existing `.minecraft` directory and other Java clients untouched.

1. Full x64 Release native C++ and Java 8 JNI build: **PASS**.
2. CTest **9/9 PASS**, including `native_agent_attach` which targets a disposable non-Minecraft Java 8 process via the official Attach API and calls native `Agent_OnAttach`.
3. Launched isolated Minecraft 1.8.9 Forge development instance to provide a real Minecraft JVM (Forge was the host **only**, not the injection mechanism).
4. `slinkywinky_loader.exe list JDK8_HOME` enumerated `GradleStart` process PID selected from this isolated run.
5. The standalone EXE attached `slinkywinky_agent.dll` to that Minecraft JVM with no Forge mod calls involved. Loader output: `ATTACH_RESULT=SUCCESS pid=20948 mode=minecraft`.
6. Target game log confirmed `[SlinkyWinkyAgent] loadedClasses=6439 minecraftClass=1` and `[SlinkyWinkyAgent] ATTACH_PROBE_PASS mode=minecraft (no gameplay hooks installed)`.
7. The isolated test launcher/process tree was terminated after the test; pre-existing Java clients were not touched.

**Verified only:** external native DLL attachment, JVMTI runtime access, Minecraft class recognition and disabled native core probe.

**Still not implemented:** hooking Minecraft 1.8.9 client events on its exact game thread, injecting a ClickGUI/HUD into vanilla Minecraft, packet/input interception, functioning combat modules and safe unloading. JVM native agents remain mapped until the process exits; repeated attach is not equivalent to a complete unload/reinject cycle.

Known legacy Linux headless Forge/Minecraft graphics startup failures are retained in earlier R14–R20 logs; the Forge tests are now **manual-only optional diagnostic workflows**, not the primary required CI. The successful independent Windows Minecraft injection test supersedes the headless Forge run as the proof of attach capability, **not** proof of finished gameplay.

No original commercial DLL, confidential assets or restricted binary contents are distributed.