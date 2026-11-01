# R61 — Opt-in continuous native observation with explicit owner-thread stop

**Date:** 2026-11-01. **Branch:** `reconstruction/r1-core-fidelity`. **PR:** #1, draft. **Status:** implementation committed; CI and in-game runtime acceptance are separate gates. **Not a playable Slinky client.**

## Motivation and limits

The previous original-vanilla 1.8.9 injection lifecycle was limited to 20/120/600 client-thread callbacks and restored Minecraft's original tick method afterward. R61 adds a **long-running, read-only, explicitly stop-driven** observation mode, using Java 8 Attach/Instrumentation and first-party native JNI. Forge is not part of the deployment.

The continuous mode is deliberately **not indefinite**: it retains an independent maximum of **36,000 actual game-thread callbacks** (approximately 30 minutes at 20 TPS, but never a wall-clock guarantee). A native validation failure aborts earlier. The mode remains *zero-action*: no attack, input synthesis, rotations, network packets, target steering, reach changes, or combat operations are enabled.

## Commands

Select an explicitly known Minecraft 1.8.9 PID using the standalone loader's existing `list` operation. With a complete Windows x64 Java 8 JDK and built first-party injector:

```powershell
$env:SLINKYWINKY_EXPERIMENTAL_HOOK = '1'
$env:SLINKYWINKY_CONTINUOUS_OBSERVATION = '1'
.\slinkywinky_loader.exe attach <MINECRAFT_PID> '<JDK8_HOME>' --hook-probe=continuous
```

When finished, explicitly request a stop against the **same exact PID**:

```powershell
.\slinkywinky_loader.exe attach <MINECRAFT_PID> '<JDK8_HOME>' --stop-observation
```

The stop command does **not** require the start-side environment flags and cannot start a new session. After the first continuous client tick, JNI creates an isolated, PID-scoped **Windows named event** (`Local\\SlinkyWinky-ObserveStop-v1-<PID>`). The standalone native loader opens that existing stop-only event with `EVENT_MODIFY_STATE` and signals it. **No second Java Attach API operation is performed while the hook is active**; the earlier reentrant Java stop design consistently stalled Windows CI and has been retired. The Java agent explicitly rejects `hook-stop` and `minecraft-stop` modes. The independent stop event is not the read-only HUD telemetry IPC and cannot send input or packet commands. Native JNI polls the stop event with a zero-millisecond wait on the existing client thread.

At the **next game tick**, and only from that exact original Minecraft client thread, `onGameTick` performs its last validated zero-action observation; closes the owned C++ context; revokes native status/HUD publications; releases all retained Java reflection/game references; emits the observation summary and `CONTINUOUS_STOP_ON_CLIENT_THREAD_PASS`; and starts the existing bytecode-restoration worker. That worker must verify the exact previously captured SHA-256 original class fingerprint before another hook may be admitted.

**The stop command acknowledges a request, not completed restoration.** When the game is stalled, no more game ticks may arrive, so explicit stop cannot force native cleanup on another thread; there is no forced unsafe unload. The process exit remains the fallback in such a condition. If class restoration fails, reattachment is permanently locked until process exit. A stop request does not unload the JVM-owned native DLL.

The three R59 finite modes (20 default, 120, 600) are unchanged and **must reject `--stop-observation`**. Continuous mode starts only with both explicit opt-in variables. The extra `SLINKYWINKY_CONTINUOUS_OBSERVATION=1` gate is independent of the R59 `SLINKYWINKY_EXTENDED_DIAGNOSTIC=1` gate.

## CI regression coverage (disposable target only; never launches Minecraft)

- The bounded 20/120 test still requires native-accepted sample counts and the original bytecode-restoration marker.
- Continuous start without the new opt-in is refused.
- Stop requests with no active continuous session are refused.
- The continuous session in a synthetic Java 8 mapped target advances beyond 20 callbacks. The standalone loader signals the dedicated Win32 event directly (without reattaching to Java).
- The original client thread emits `CONTINUOUS_STOP_ON_CLIENT_THREAD_PASS`, and the cleanup worker restores the original class bytes.
- Exactly **three** restoration markers and three native summary records are required across 20, 120, and stop-driven sessions; the continuous accepted count must be greater than 20 and below 36,000.
- A second stop after completed cleanup is refused because the named event is closed. Unsupported loader arguments and incorrect/missing session PIDs are denied.
- `HookAdmissionSmoke` verifies that the earlier reentrant Java agent stop mode is explicitly refused without affecting a running class mapping. Runtime ownership, event close and class restoration are checked by the Windows end-to-end fixture.

These checks demonstrate the opt-in state-machine and owner-thread lifecycle in a disposable **simulated Java target** if CI passes; they do **not** prove actual persistent behavior in an original vanilla Minecraft world. The R60 manually authorized in-world observation harness remains necessary. Actual GUI, HUD integration, game-action modules, supported additional launchers, real native DLL unload and 1:1 commercial reference behavior remain unverified.

## Safety and provenance

No original decrypted DLL, proprietary resources, Minecraft game binary, fonts, or original injector are committed. The supplied decrypted sample remains a read-only local reference with unverified release provenance. The official Slinky website and documentation inform the feature specification, but cannot certify recovered binary authenticity or working parity.
