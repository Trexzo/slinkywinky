# R77 — full disposable Java 8 Attach → renderer callback → JNI WGL → native GL11 menu

**Scope:** end-to-end **test-only**, cross-process integration of the exact original Minecraft 1.8.9 *class and method names* with a first-party native menu renderer. No Minecraft JAR or running Minecraft JVM is used, modified or attached.

## What is now exercised

1. A new disposable Java 8 process (`R77WglRenderTarget`) loads generated `ave` and `bfk` classes through the system classloader, creates its **own** small WGL window through the R76 native test DLL, and invokes the unmodified `ave.av → bfk.a(FJ)V` path.
2. The separate `R77WglAttachLauncher` enforces three environment opt-ins (`SLINKYWINKY_R74_DISPOSABLE_FIXTURE=1`, `SLINKYWINKY_R76_WGL_FIXTURE=1`, `SLINKYWINKY_R77_DISPOSABLE_FIXTURE=1`), a test-only agent JAR file name, a numeric PID and the exact disposable Java main-class descriptor.
3. The R74 **test-only** agent permits the new `R77_DISPOSABLE_WGL_ONLY` mode only after the exact synthetic-main-class and opt-in check. The existing R73 structural preflight for generated `ave` and `bfk` passes before the agent retransforms the synthetic `bfk.a(FJ)V` method with a direct callback to `R77WglRenderReceiver.onRender(FJ)V`. The original R74 mode and its callback remain independent.
4. In the original main/renderer Java thread, eight callbacks call JNI to the same WGL context through the R76 fixture DLL. The **eighth callback itself** invokes the first-party C++ `WglMenuOverlayHost::draw` / GL11 menu. Its native adapter rejects a foreign thread and verifies viewport, attribute depths, matrix mode, color and HGLRC/HDC restoration.
5. The fixture checks that all sixteen original `bfk.a(FJ)V` bodies still ran, that bytecode retransform restores the exact original SHA-256 with the callback transformer removed, and that twelve more original frames run with **no additional JNI render callbacks**.
6. The disposable Java process destroys its own WGL context and exits. CTest `native_r77_attached_render_jni_wgl_menu` checks all markers and exit status.

The R77 agent mode cannot be used on a game JVM: the actual Attach launcher and the agent both require the disposable main-class identity. The `slinkywinky_r74_test_agent.jar` and `slinkywinky_r76_wgl_fixture.dll` are build/test-directory objects and are **excluded from the preview release artifact**.

## Why this is meaningful but not yet live-game acceptance

R74 established synthetic original-name Java 8 Attach and restoration. R76 independently established Java 8 JNI native WGL context ownership and first-party GL11 menu drawing in a disposable native window. R77 joins those two paths **inside the actual modified synthetic renderer callback**, rather than sampling from the caller after the render method returns.

It still does **not** verify that a live vanilla Minecraft 1.8.9 renderer thread has a current WGL context when `bfk.a(FJ)V` is entered, nor that inserting any actual UI into Minecraft is safe. It is not a Forge mod. It does not enable gameplay modules or install a production overlay.

Before real in-game activation, separately verify R75's SHA-pinned production callback and restoration in the original game and inspect the callback's WGL context and GL state under game load. Do not merge PR #1 while desktop-only acceptance is outstanding.
