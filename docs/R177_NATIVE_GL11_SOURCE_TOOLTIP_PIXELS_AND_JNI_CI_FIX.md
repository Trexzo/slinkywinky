# R177 — real GL11 tooltip pixel proof, and JNI-only CI configuration repair

**Repository:** `Trexzo/slinkywinky`, branch `reconstruction/r1-core-fidelity` (draft PR #1). Independent upstream C++ reference integration PR #2 is intentionally untouched.

## Why R177 was needed

R176 established an exact-original Minecraft 1.8.9 runtime receipt for 64 retained WGL GUI frames and 63 read-only hover callbacks, with `foreground=0 numeric=0 bind=0`. Its evidence proves that the original game entered the passive hover path but **does not** prove that a tooltip visibly appeared under a real cursor. The production native mouse-input and keyboard-input ownership is still unproven and disabled.

R176 had an unanticipated GitHub Actions regression: **Windows JNI preview artifact** failed to configure with `No target "slinkywinky_loader"`, while the Offline C++ core, original Minecraft 1.8.9 audit and injectable Java8 agent workflows succeeded. The standalone JNI configuration sets `SLINKYWINKY_BUILD_ATTACH=OFF` and does not define the Java8 attach loader. R176 had registered a CTest referencing `$<TARGET_FILE_DIR:slinkywinky_loader>` under the broader `SLINKYWINKY_BUILD_JNI` guard.

R177 narrows the R176 attach-JAR-dependent CTest registration behind `if(SLINKYWINKY_BUILD_ATTACH)`. Its standalone JNI-only build remains able to configure, build the native library, and register the genuinely disposable WGL JNI tests. In the full attach-enabled build, the original Java8 hover receipt test remains registered and passes.

## R177 actual pixel-level native tooltip acceptance

A new **test-only** JNI endpoint `R177RetainedTooltipPixelsSmoke.pixelDigest()` is compiled only into the disposable `slinkywinky_r166_pointer_test_jni.dll` under its separate `r166_disposable_only` directory. The production `slinkywinky_jni.dll`, Java 8 attach agent and original-client classes are unchanged by this test.

The endpoint uses the same first-party C++ `WglRetainedMenuPreview` and GL11 draw implementation as production. It admits only a valid WGL owner, exactly pinned 676×630 context/viewport, and the source-backed expanded **Auto Block → Main → Range** slider. It independently reconstructs the recovered widget index, source numeric slider geometry and the corresponding 20px numeric-tooltip rectangle using the same `menu_numeric_tooltip_geometry` function used by the native renderer. It then clears a disposable framebuffer, draws the actual retained GUI, and reads only the pixel area of this tooltip via `glReadPixels`. A stable 64-bit digest represents the actual rendered RGBA bytes, **not a screen capture of the connected Windows desktop**.

`R177RetainedTooltipPixelsSmoke.java` tests, using Java 8 and the actual native WGL DLL:

- No pixel readback before the disposable owner/session is created or before the source slider is selected and expanded.
- A clean baseline pixel digest over the exact source tooltip rectangle.
- A source-validated numeric hover on the actual expanded slider **changes the GL11 tooltip pixels**.
- Two identical hover observations produce the same source-area digest; native tooltip pixel output is stable.
- Losing the synthetic foreground flag or moving outside the source client viewport restores the baseline pixel digest; recovering foreground restores the source-hover digest.
- A foreign JVM thread or changed viewport cannot obtain or alter the native tooltip pixels; original serialized settings remain unchanged.
- Disposal removes access to the framebuffer.

CTest `r177_java8_jni_source_tooltip_pixel_proof` compiles the disposable Java fixtures and executes real C++ WGL rendering, source-tooltip-region framebuffer readback, and corresponding negative controls. The fixture is excluded from both production agent JAR and the offline audit JAR. `tests/test_r119_live_input_guard.py` explicitly prohibits production references to the new disposable pixel test.

## Acceptance evidence and remaining limitations

R177 acceptance includes independent `SLINKYWINKY_BUILD_JNI=ON, SLINKYWINKY_BUILD_ATTACH=OFF` CMake configure and native Release build, an attach-enabled CMake configure, this new CTest, plus complete **clean, outside-Temp Windows Release and Debug** regression suites before publishing the commit. The four GitHub workflows are checked after push.

An actual foreground client window hovered over the reconstructed original-game overlay is **still untested**. This machine was observed to be actively receiving desktop input while R177 ran. We therefore avoided forcibly moving the user's cursor or stealing desktop focus. A future isolated in-game foreground test must prove exact foreground owner, source tooltip target, full game class restoration, reversible user-focus restoration, and the rendered tooltip under the real cursor.

**R177 does not enable any original-game click interception, event suppression, gameplay module, packet sending, automation, or live mouse gesture routing.** All existing CI/workflow guards remain active. PR #1 stays draft and unmerged.
