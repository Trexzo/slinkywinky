# R67 — guarded Windows OpenGL 1.1 menu drawing

**Branch:** `reconstruction/r1-core-fidelity` · **PR:** #1, draft · **Date:** 2026-11-01.

## Actual implementation

R67 introduces a first-party fixed-function drawing adapter:
- `include/slinkywinky/windows_menu_gl11.hpp`
- `src/windows_menu_gl11.cpp`
- `tests/test_windows_menu_gl11.cpp`
- Windows-only `slinkywinky_menu_gl11` static library and `isolated_wgl_menu_state_restore` CTest.

`draw_menu_gl11(MenuRenderFrame, expected HGLRC, expected HDC)` draws a background, orange accent, category indicator, up to eight alternating item rows, expandable-row markers and UI-only preview toggles using GL1.1 opaque quadrilaterals. It has **no** text/font renderer, module execution, packet path, input injection, target processing, or foreign binary assets.

Before issuing draw commands, the function requires exact equality to the current WGL graphics and device contexts, a valid frame epoch and sequence, a 320–8192 by 240–8192 viewport that matches the active OpenGL viewport, a recognized public catalog tab, and only in-category module entries. A mismatched/absent context or invalid viewport returns false.

The scoped fixed-function guard verifies free attribute, client-attribute and matrix stack capacity, saves `GL_ALL_ATTRIB_BITS` and `GL_CLIENT_ALL_ATTRIB_BITS`, pushes the modelview/projection/texture matrices, performs first-party drawing, then pops all stacks and restores the original matrix mode. No frame buffer is cleared; the adapter does **not** call `wglMakeCurrent`, switch graphics contexts, modify the GL viewport, alter shader programs, create textures, or retain game objects. R67 uses the existing immutable `MenuRenderFrame` from the R63 native owner-thread lifecycle.

**Note about the first CI failure:** a strict capacity check initially assumed GL attribute stacks start at depth one. In OpenGL, the server/client attribute stacks initially have depth zero, while matrix stacks start at one. Windows Release CI caught the resulting valid-context rejection. Commit `977bc170` corrected the distinction; subsequent Windows Debug `isolated_wgl_menu_state_restore` passed, including complete fixed-function snapshot comparisons. Final status should use the latest exact-commit CI results, not the first failed run.

## Real isolated Windows graphics acceptance

`tests/test_windows_menu_gl11.cpp` creates its **own hidden Win32 window**, chooses a standard double-buffered WGL pixel format and activates its own WGL context. The test sets nondefault GL state (blending, depth, texture enable, fog, culling, scissor, alpha and lighting; blend factors, current color, projection/modelview/texture matrices) before attempting to draw.

It rejects null, wrong or absent contexts, wrong device handles, wrong viewport, invalid menu epoch and wrong category/catalog data without altering state. It then draws two frames—including an enabled UI-preview switch—and checks that the original GL state matches before and after, including matrix data and the attribute/matrix stack depths. An unrelated thread with no current WGL context cannot use the renderer. `glGetError()` must report no error after each accepted draw.

The test runs on `windows-latest` in Debug and Release; it uses a disposable graphics window and **never runs or attaches Minecraft**.

## Remaining incompatibilities and acceptance requirements

This is **not** a complete OpenGL-context preservation guarantee for Minecraft or third-party render pipelines. GL1.1 push/pop cannot certify preservation of LWJGL-managed Java state caches, modern shader programs, VAOs, framebuffer bindings, active texture-unit selection, driver resources, or arbitrary modifications from other threads. The function assumes it is called outside `glBegin/glEnd` on an admitted compatibility-profile rendering thread. The actual Minecraft 1.8.9 renderer class/method must be independently mapped and validated in a user-owned original JAR, not guessed from MCP names.

The authoritative Windows desktop was offline at R67 development time, so no original in-world game acceptance has been performed. The production Java instrumentation agent remains tick-only; `draw_menu_gl11` is **not wired into** Java Attach, JNI, a Minecraft render method, or the standalone GDI menu preview. The initial WGL smoke establishes isolated fixed-function drawing and local state restoration only.

**R68 gates:** verify original 1.8.9 class mapping; validate an owner-thread read-only in-game render callback; add a full GL/LWJGL state and context-loss strategy; only then connect the first-party GUI and verify it visibly draws and shuts down correctly in vanilla Minecraft. Do not merge the draft PR or claim original Slinky parity, support for alternate launchers, or working gameplay modules.
