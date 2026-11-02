# R69 — bounded native OpenGL overlay (disposable preview only)

**Status:** R69 is an offline preparatory milestone on draft PR #1. The production Minecraft injector remains tick-only; no game render method or input callback is installed.

## Problem found in R67/R68

The first-party GL1.1 drawing routine rendered a dark background across the **entire active viewport**. That was acceptable for its own standalone preview window, but would obscure the original Minecraft scene if called during gameplay. In addition, an in-game menu must not clear or replace the existing frame or leave a changed scissor box, blending mode, or matrix behind.

## Implemented R69 boundary

`draw_menu_gl11` now requires an explicit `MenuGL11Mode`:

- **`StandalonePreview`** (default): retains the existing full-window preview behavior and pixel-readback baselines.
- **`BoundedOverlay`**: issues only panel-scoped drawing. The panel is at most 676 px wide (844 when `wide` is selected) and 630 px tall, centered in the current validated viewport, with a minimum twelve-pixel border all around. In smaller viewports it clips the same first-party layout rather than painting beyond the requested bounds. The background and item rows are translucent, while text, controls, and accent marks remain opaque.

The bounded mode **does not call `glClear`**, `SwapBuffers`, `wglMakeCurrent`, or alter the GL viewport. It enables scoped alpha blending with `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA`, sets a bounded `glScissor`, translates only the protected modelview matrix, and draws the existing original UI geometry/text. The R67 attribute/matrix guard restores scissor enable+box, blend enable+factors, fixed-function matrices and other GL1.1 state when it returns. A missing/wrong WGL context, invalid frame, mismatched tab/name, invalid viewport, or unknown mode is denied before any geometry is emitted.

## Tests and visual inspection

The isolated `tests/test_windows_menu_gl11.cpp` now checks both rendering modes. It clears **its own disposable test framebuffer** to a distinctive blue background (not part of the library draw routine), records pixels on all four sides outside the prospective panel, draws the new translucent menu, and verifies those outer pixels remain byte-for-byte identical. It verifies an accent pixel inside the panel is distinct from the original scene, checks a separate interior pixel, and repeats after switching tabs. It also validates exact restoration of the GL scissor box, active matrix mode, transform stacks, existing GL enable flags and blend factors. An invalid enum value is explicitly rejected.

The standalone `slinkywinky_gl11_menu_preview.exe` has an **`O` key** to toggle the bounded-overlay demonstration. In overlay-demo mode, *the standalone preview application itself* draws a sky-blue test scene, then calls the bounded draw mode to show the background preserved outside the menu. `O` returns to the regular full-canvas preview. This is a first-party graphical test window only; it never attaches to Minecraft. Earlier controls (`1`–`6`, `C`, `W`, `G`, wheel, and `Esc`) remain preview-only.

## Explicit limitations and next requirements

This proves **isolated WGL fixed-function overlay compositing**, not correct rendering inside Minecraft. It does not preserve or validate arbitrary LWJGL Java state caches, external shaders, multi-texture state, FBOs or other modern OpenGL state. No Minecraft client `1.8.9.jar` has been obtained or scanned, and the authorized Windows desktop is still offline. The original class/method candidate remains unverified. No user-facing GUI is injected into the game and module switches cannot initiate gameplay actions.

For a true Minecraft renderer, the next integration milestone must separately obtain an exact original-client class/method fingerprint, prove the owner render thread and context are valid, account for external OpenGL/LWJGL state caching, ensure clean stop/disconnect/context-loss teardown, and verify visual output **in vanilla 1.8.9** on the authorized desktop before any merge or compatibility claims.
