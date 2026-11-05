# R65 — read-only renderer callback fixture and owner-thread admission

**Date:** 2026-11-01. **Branch:** `reconstruction/r1-core-fidelity`. **PR:** #1 (draft).

## Purpose and limits

R64 identified a **candidate** for the original Minecraft 1.8.9 render call based on public source/method descriptor, and added a verified-JAR offline audit. The original client JAR has **not** been scanned on the authorized Windows desktop because its remote connection is offline. R65 therefore tests callback behavior on an independently generated Java 8 fixture rather than claiming a verified live render hook.

No graphics API, Minecraft class or game input is accessed or modified. No Minecraft binary, commercial Slinky resource or original client library is committed. No Forge runtime dependency is added.

## New code

- `loader/java/dev/slinkywinky/instrument/RenderCallbackProbe.java`: test-only ASM 5.2 fixture weaver restricted to exact internal class name `dev/slinkywinky/attach/R65Fixture` with a single public instance `a(FJ)V` method. It inserts a static callback before the original method body; the callback **only** counts accepted frames. It does not render, synthesize input, perform actions, retain game references, access native libraries or allocate GL resources.
- `RenderCallbackProbe.armFixture(epoch,limit)`: explicit positive monotonically advancing session epoch and per-session limit of 1–64 callbacks. A second arm while active is refused.
- `onRender(float,long)`: only the original owner thread can contribute a count. Invalid partial ticks, negative timestamps or a foreign thread permanently disable the current armed session. The callback is guarded by an explicit opt-in test arm and does not throw into the fixture's original method.
- `closeFixture(epoch)`: requires original owner and exact epoch; records accepted-count/invalid summary and revokes all active callback admission.
- `tests/java/dev/slinkywinky/attach/RenderCallbackSmoke.java`: creates the synthetic classfile in memory, loads the instrumented fixture in a disposable custom classloader, invokes real Java bytecode and verifies callback bounds. Tests reject wrong class/method/overload/static form, invalid limits, rearming without teardown, replayed epochs, invalid callback values and wrong-thread close. Loads the original class bytecode separately and checks its original SHA-256 fingerprint remains identical. **This tests test-classloader isolation; it is not proof that a live Minecraft JVM retransformation restores bytes.**
- `CMakeLists.txt` and `tests/attach_smoke.py`: compile the synthetic callback fixture with the pinned ASM already bundled for offline Java 8 tests. This class is **not** included in `slinkywinky_instrument.jar`; it remains separate from the production agent. The Python end-to-end harness requires both `R65_READ_ONLY_RENDER_FIXTURE_PASS` and `R65_RENDER_THREAD_AND_BOUNDS_NEGATIVE_CONTROLS_PASS`.

## Verification matrix

- JVM-only fixture callback, frame count cap and owner thread: Windows Java 8 CTest, requires success.
- Original unmodified fixture bytes: compare SHA-256 and load as a separate class definition; does not exercise live retransformation.
- Windows native injector, JNI, C++ core: existing tests must remain green, including 20/120/continuous stop flows.
- Original Mojang 1.8.9 renderer name/class and 1.8.9 world HUD: **unverified** until installed-JAR audit and interactive in-game acceptance.
- Actual `RenderCallbackProbe` in Minecraft: **not enabled**; production Java agent remains tick-only.
- Render GUI drawing, GL state preservation and source-equivalent Slinky behavior: **not implemented**.

## Next integration gates

1. Execute the **R64** original SHA-1-gated Java 8 offline tool against a local installed Mojang client JAR. Verify the inferred class/method from the version-exact caller, and reject ambiguity; do not upload the proprietary game JAR.
2. Independently prove that callback invocation occurs on the actual Minecraft renderer thread, beyond a synthetic fixture. Never rely on CI alone for live-game claims.
3. Require a dedicated window/GL initialization and complete GL state save/restore before drawing any first-party menu. Keep all player-control, combat and packet operations disconnected from GUI.
4. Test close/restore both during ordinary menu frames and disconnect/context loss; retain the opt-in, fail-closed and exact fingerprint gates.
5. Keep PR #1 draft until actual vanilla 1.8.9 acceptance. Treat 1.7.10 and third-party launchers as separate ports.

This document records a synthetic callback exercise, **not completion of an injectable replacement Slinky client**.
