# R113 — Native Slinky-style ClickGUI visual fidelity

**Branch:** `reconstruction/r1-core-fidelity` (draft PR #1, unmerged)
**Scope:** Render-only native WGL/OpenGL 1.1 preview and its existing first-party mouse-hit test geometry; no new Minecraft input, gameplay hooks, Java agent behavior or Forge integration.

## Reference and rationale

The previous early-test UI was intentionally a diagnostics mock-up: an oversized “SLINKYWINKY” title, uppercase blocky 5×7 bitmap glyphs, an orange horizontal strip/underline, wide rectangular module rows, and “NOT IMPLEMENTED” statuses. These are not how the documented Slinky ClickGUI looks. R113 uses Slinky’s publicly visible GUI references:

- [Getting started and screenshots](https://docs.slinky.gg/basics/get-started/)
- [GUI module appearance settings](https://docs.slinky.gg/modules/visual/gui/)
- Documented official static image filenames include `nav.hxxCX4fS_Z1ckBvz.webp`, `collapsed.BfdVLPGh_Z25HoFu.webp`, `expanded.CvnwaSZe_1K3k5y.webp` and `profiles.BVi9pNfi_ZoANip.webp`.

These references guide the first-party visual implementation, **not a claim of a byte-for-byte or pixel-perfect clone**. All text/code/shape logic is newly written. No vendor fonts or copyrighted graphic assets are bundled.

## Source implementation

- `src/windows_menu_gl11.cpp`: compact rounded top navigation with selected gray pill, red *Unload* label, charcoal background, solid rounded module cards (including explicit triangle-fan closure), mixed-case module titles, gray keybind `NONE` badges, rounded orange/gray switches and (+/-) expand buttons. Expanded cards show an original preview setting pill. Removed large diagnostic branding and implementation-status descriptions.
- `src/windows_menu_system_font.cpp`: antialiased local Windows Segoe UI bitmap glyphs acquired via GDI and emitted as GL1.1 grayscale geometry without textures, third-party files or runtime game/JVM state. The original 5×7 glyph renderer is a fallback when a local Windows system font cannot be initialized.
- `include/slinkywinky/menu_visual_copy.hpp`: 65 original explanatory module captions matching the existing public-name taxonomy. Captions are purpose descriptions, **not implemented-feature claims**.
- `include/slinkywinky/menu_visual_geometry.hpp`: shared geometry for nav, row body, expanded section and row spacing.
- `src/menu_hit_test.cpp`: hit areas use the very same geometry as the renderer, with strict clipping/unsupported-module refusal, inert red Unload, and no input delivery outside the admitted native menu.
- Default accent adjusted to a vivid orange `0xFF6016` in the first-party appearance model and preview toggles. Existing saved appearance values remain backward compatible and are not overwritten.
- `CMakeLists.txt` adds the native system-glyph renderer and GDI linkage. Mouse tests, screen-pixel tests, default-accent tests and preview defaults were updated to reflect the new visual geometry. No changes were made to actual combat modules or interception logic.

## Actual generated visual evidence

The screenshots below are **readbacks of the actual first-party WGL test renderer** in its own temporary OpenGL 1.1 context, not mockups, game screenshots, website images or screenshots of unrelated windows. They were generated via three opt-in framebuffer captures in `tests/test_windows_menu_gl11.cpp` and converted to PNG without altering image content.

| Scenario | Screenshot |
| --- | --- |
| Collapsed Combat view | [R113 combat preview](screenshots/r113-combat-preview.png) |
| Preview-only enabled Aim Assist (orange toggle) | [R113 orange toggle preview](screenshots/r113-enabled-preview.png) |
| Expanded Aim Assist + preview-only setting | [R113 expanded card preview](screenshots/r113-expanded-preview.png) |

The enabled and expanded screenshots come from different test frames; the expanded screenshot deliberately does not attest that its demo toggle remained enabled.

## Safety, correctness and outstanding gaps

The original real Minecraft 1.8.9 Java 8 client is still handled through the separately supervised, source-pinned R94 show/hide/stop acceptance. R113 affects the renderer already used by that codepath; its screenshots here are generated **outside** Minecraft. The actual real-game ClickGUI appearance and physical-click alignment must still be checked independently.

All bounded and compact overlay paths maintain original WGL context, DC and viewport identity checks, scissor isolation and complete saved GL state restoration (including matrices, blending and current color). Focus, keybinding, hit-test and mouse-gesture boundaries remain independent. Visual preview switches do **not** toggle actual gameplay modules.

**Remaining mismatches:** the Profiles page is visibly labeled a placeholder because no real profile-card action wiring exists yet; full native source-client controls and their dynamic settings arrangement remain incomplete; the older separate GDI-only standalone preview does not yet mirror all of the new GL11 styling. This is a meaningful visual correction, not final reconstructed Slinky feature fidelity. The project remains injectable, not a Forge mod; PR #1 stays draft/unmerged until real acceptance.

## Verification

Local Windows x64 Release build **PASS**. Full CTest **30/30 PASS**. Python **55 executed, one skipped, zero failures**. Isolated WGL framebuffer capture, fixed-function GL state restoration, visual/mouse-hit geometry, native Segoe UI text measurement **PASS**. A supervised single vanilla Minecraft 1.8.9 JVM (PID **23232**) completed the original R94 **scripted operator show/hide/stop** acceptance and then exited normally. That is **not physical input acceptance**. Remote GitHub CI is checked separately after push.
