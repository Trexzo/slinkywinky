# R91 — explicit one-frame original-Minecraft GL11 overlay admission

**Status:** first-party source and isolated Windows/JNI/WGL fixture accepted locally. The original Minecraft 1.8.9 **live overlay was NOT executed or visually inspected in R91**, because a separate Minecraft-like JVM was already running on the Windows host. The R90 original-game read-only render/WGL diagnostic **is** already independently PASS. Do not conflate the two.

## Objective and authorization

R91 adds exactly one opt-in path from the R90 source-pinned original Mojang 1.8.9 renderer callback to a native OpenGL 1.1 text-and-geometry menu. It is **not Forge**. It does not intercept input, use a texture/font bundle, access Minecraft gameplay data, activate combat modules, send packets or persist native graphics state. The actual one-shot menu is visibly labeled `NATIVE MENU / PREVIEW ONLY`.

The standalone loader only accepts `--render-overlay=8` with the exact JNI DLL. The Java Attach helper requires **all three** explicit environment variables `SLINKYWINKY_RENDER_OVERLAY=1`, `SLINKYWINKY_RENDER_CONTEXT=1` and `SLINKYWINKY_RENDER_CALLBACK=1`, and refuses an Attach-test bypass or a non-Minecraft target. The R91 opt-in is separate from R78 passive context mode, which never draws.

## Runtime trust boundary

1. A manually authorized operator may run `tests/windows_vanilla189_isolated_manual.py --run --render-overlay-acceptance`, with the existing `--workspace`, `--jdk8` and `--loader` paths. The harness refuses another Minecraft JVM **before starting a sandbox**. It launches only a separate original 1.8.9 client using local Mojang JAR/libraries and Java 8, no Forge and no account login; creates a unique exclusive stdout log; and terminates only its own child on completion.
2. The existing R81/R90 checker verifies the exact original local JAR SHA, original loaded-class provenance, stable PID creation timestamp, fresh target JVM stdout and ordered release receipts, then performs **R73 preflight → R75 bounded callback → R78 eight-frame WGL context → R91 one-frame overlay**. Each subsequent Attach requires the prior session's verified class restoration and lock release.
3. The R91 callback uses the already established `RenderCallbackFence`: only a validated `ave.av → bfk.a(FJ)V` call on the owner client thread can invoke any native WGL query. The Java WGL observer requires eight valid, current, unchanged context/viewport samples. It attempts **one** overlay on sample eight only, before returning to Mojang rendering.
4. The production `slinkywinky_jni.dll` function `nativeDrawReadOnlyOverlay` verifies exact current `HGLRC` and `HDC`, viewport, bounds and caller state, opens a short-lived `WglMenuOverlayHost`, draws a copied, inert `MenuState` through the existing `draw_menu_gl11(...BoundedOverlay)`, closes on the same thread and checks retained viewport, GL color, matrix mode and attribute stacks. The shared native renderer already restores the full fixed-function server/client attributes and matrix stacks and clips drawing to a centered inset. It neither switches GL contexts nor owns Minecraft's context.
5. The target prints `R91_ORIGINAL_RENDER_OVERLAY_DRAW_PASS frames=1` only after the eight stable WGL samples, successful native return and verified renderer-class restoration. Missing/malformed/duplicate/out-of-order receipts, `R91_RENDER_OVERLAY_UNVERIFIED`, or a replaced JVM/source class fail closed. No separate Attach or menu draw runs after an unverified restoration.

## Verified results from the connected Windows machine

- Full Java 8 x64 MSVC native/JNI/Attach/WGL CTest before the additional direct JNI fixture assertion: **25/25 PASS**.
- Added a test that links **the actual production JNI DLL** and invokes the same native entrypoint on a disposable owner-thread WGL context, with pixel-readback evidence that the bounded panel is drawn and that pixels beyond the panel remain intact. The test requires fixed-function GL state restoration and rejects wrong context/viewport and cross-thread calls. `isolated_wgl_menu_state_restore`: **1/1 PASS** on this R91 source.
- Supervised acceptance receipt and opt-in negative controls: **24 Python tests run, 1 OS-specific skip, zero failures**, including four-stage simulated overlay Attach and cross-stage marker rejection. All simulated tests run **without a Minecraft JVM**.
- The R90 real-game bounded callback/WGL proof remains established independently as **8/8 render callbacks on Client thread#1 and 8/8 current stable WGL samples**, with restored source-pinned renderer code.
- **R91 live original-game attempt:** blocked *before launch and Attach* because a separate Minecraft-like JVM was already running on the Windows computer. No R91 target-side draw receipt, real Minecraft frame capture or game-window visual-health result exists yet. No other user-owned Java client was touched.

## Limits / next step

A passing isolated production-JNI pixel test is not an original-Minecraft visual confirmation. A single transient frame also does not constitute an interactive ClickGUI. To claim in-game R91 acceptance, run the manual isolated harness only when no other Minecraft JVM exists, verify all four fresh target receipts and separately inspect the game window without bypassing any gate. Do not merge PR #1 or activate gameplay controls based on this test alone.
