# R166 — Java 8 → JNI → native retained WGL ClickGUI, isolated integration

**Repository:** `Trexzo/slinkywinky`, branch `reconstruction/r1-core-fidelity`, draft PR #1.

## Delivered

R159 established a first-party `WglRetainedMenuPreview::pointer()` controller with source-indexed hit testing, module configuration, held-gesture cancellation, WGL owner/context/viewport pinning and no gameplay actions. R165 verified a *synthetic* Java 8 two-queue input isolation lifecycle. The previously missing end-to-end link was whether a Java caller could reach the actual native retained GUI pointer controller through JNI, without reconstructing its behavior in Java.

R166 adds **one separate, disposable Windows JNI fixture**, built solely from `tests/r166_retained_pointer_jni.cpp` into `slinkywinky_r166_pointer_test_jni.dll` under a dedicated `r166_disposable_only` build directory. It links the existing first-party native menu renderer. It does **not** modify `src/jni_bridge.cpp` or the injectable Java agent, and the runtime Java class `RendererWglEvidence` has no new production native methods.

The Java 8 test `R166RetainedPointerJniSmoke` loads *only* this exact disposable DLL path. The native DLL creates a hidden, disposable Win32 window, establishes a local WGL context, draws the *real* retained C++ menu, resolves the source-validated first module toggle from the same layout and hit-testing routines, and returns its client-pixel coordinates to Java. Java delivers pointer phases to the native menu and reads the actual serialized native profile.

## Tests

The integration fixture verifies:
- Pointer rejection before session admission and after disposal; one active session only.
- Rejection for missing **synthetic** exclusive dispatch or foreground, invalid phase/time, wrong WGL viewport and foreign Java thread.
- Real `Down` + `Up` toggles the first catalog module's `aim_enabled` from false to true, persists across OpenGL redraw and changes back after a second well-separated valid gesture.
- Suppression of a second click within the double-click window, `Cancel` followed by release, stale viewport release, and pointer move-out followed by release; rejected gestures leave the native profile unchanged.
- Same-thread WGL lifecycle cleanup and rejection of any subsequent Java JNI call to the disposed test session.

CMake exposes the additional test `r166_java8_jni_to_retained_wgl_pointer_profile` only under Windows `BUILD_TESTING` with JNI available. Production-source guard tests reject any accidental references to the R166 test-only Java class, and Java attach-package checks require the R166 smoke test class absent from both `slinkywinky_instrument.jar` and `slinkywinky_render_audit.jar`. The R166 DLL is segregated from the production DLL output.

## Security and fidelity limits

This is an isolated integration success, **not** a real input hook. Java's `exclusive` and `foreground` booleans in the fixture are test inputs, not authentic Minecraft input-ownership receipts. No original Minecraft 1.8.9 mouse or keyboard callback calls the test DLL. There is no WndProc subclass, LWJGL replacement, original class retransformation, packet manipulation, automated movement/attack, or game-client process interaction. The 67 source-named module catalog is not thereby complete.

The next milestone should build a real proof of game-thread input ownership and reversible original-bytecode restoration before introducing a production Java pointer callback. The draft PR remains unmerged until original Minecraft acceptance is available.
