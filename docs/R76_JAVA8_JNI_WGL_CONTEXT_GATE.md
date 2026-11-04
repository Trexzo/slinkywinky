# R76 — Java 8 JNI → WGL render-context admission (isolated fixture)

**Scope:** test-only **live Windows OpenGL context** verification from a Java 8 thread. This is a concrete step toward native menu integration, but **not an in-game Minecraft hook** and **not an in-game ClickGUI**. Keep PR #1 draft.

## New code
- `WglFrameContextProbe` (`include/slinkywinky/windows_wgl_frame_probe.hpp`, `src/windows_wgl_frame_probe.cpp`) is a small thread-safe read-only WGL observer. It requires a current **non-null** `HGLRC` and `HDC`, valid origin-zero viewport dimensions, and an exact native thread identity. Each session accepts at most **eight samples**. Once a context/viewport/thread change is detected, that epoch is invalid permanently.
- No WGL resource creation, `wglMakeCurrent`, `glViewport`, rendering, input or game actions occur inside the **observer**. Only `wglGetCurrentContext`, `wglGetCurrentDC`, and `glGetIntegerv(GL_VIEWPORT)` are used.
- The **test-only** `slinkywinky_r76_wgl_fixture.dll` creates an invisible disposable Win32 WGL window and context, makes it current on its own Java thread, then exposes JNI test controls. Unlike the observer, the fixture deliberately calls `wglMakeCurrent` and `glViewport` to simulate context creation/loss and viewport drift. It must **never be loaded into a Minecraft JVM**.
- `R76WglJvmSmoke` runs Java 8 → JNI → WGL → native observer within one expendable process and checks: no-context rejection, duplicate-open refusal without disposing the live context, exact eight-frame ceiling, cross-Java-thread rejection, context-loss rejection that remains sticky on restoration, viewport drift rejection and complete resource cleanup.
- CMake builds the disposable DLL only with `SLINKYWINKY_BUILD_ATTACH=ON` **and** `BUILD_TESTING=ON`. The new CTest is `native_r76_java8_wgl_context_admission`. Release/runtime JVM modes, game class transformations, and the existing `WglMenuOverlayHost`/menu drawing implementation are not connected to this test-only DLL.

## Why these boundaries matter
R75's exact original 1.8.9 `bfk.a(FJ)V` callback was verified using official Mojang bytecode and Java 8 synthetic tests; however the desktop is unavailable and the **actual game render thread may not have a current WGL context at entry**. No claim about Minecraft-owned graphics resources follows from a standalone fixture.

The existing R70 menu graphics host checks a supplied `HGLRC` and `HDC` before drawing, and restores GL1.1 state on the standalone fixture. R76 independently tests the **Java-to-native thread ownership boundary** needed before considering a connection between an actual callback and the native menu host.

## Remaining in-game validation
1. Validate R75's callback and exact SHA-256 restoration in a real **original** Java 8 Minecraft 1.8.9 instance.
2. Observe whether that callback's actual native/Java thread has the expected WGL context and viewport—without rendering first.
3. Only after both gates pass, test a bounded native menu draw with strict GL state/restoration and owner-thread teardown, then verify the game continues to render normally.

A successful R76 Windows JNI fixture test does **not** authorize attaching a graphics adapter to arbitrary processes, bypassing server rules, or enabling graphical hooks in the default client.
