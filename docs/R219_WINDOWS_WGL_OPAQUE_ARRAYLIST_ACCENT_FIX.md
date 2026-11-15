# R219 — Windows CI WGL ArrayList accent blending repair (Chat 1 / PR #1)

Date: 2026-11-15. GUI owner: `reconstruction/r1-core-fidelity`. Native integration owner: independent draft PR #2 `integration/openslinky-fd9c592-reference`.

## Root cause from actual GitHub Actions evidence

PR #2's R222 head (`10eaaa666d1d70f70eed07a162631430438fc721`) has a reproducible **Windows WGL** failure, not a demonstrated native Strafe-policy failure. The Windows Release CTest `isolated_wgl_menu_state_restore` failed on the exact assertion `paintedArraylistAccent == F35D12`; the Java 8 `r209_retained_jni_arraylist_owner_and_pixel` also failed on its accent pixel. The Windows JNI and standalone-loader workflows report the same WGL assertion. Linux reference and policy tests passed.

PR #2's isolated diagnostic probe on the GitHub Windows runner reported actual 676 × 630 framebuffer accent pixels `F05B10` in both first and second rows, against requested `F35D12`. Its standalone R220 probe found:
- `GL_BLEND=0`, dithering either way: `F35D12` (exact).
- `GL_BLEND=1`, dithering either way: `F05B10` (three-channel rounding).
- R221's opaque-mode probe rendered the **accent** exactly as `F35D12` while leaving its dark background blended.

This is a renderer state issue, reproducible under GitHub's pixel-format/software WGL path. It does not justify changing the expected source accent, accepting tolerances, skipping tests or weakening the guard.

## First-party fix

`src/windows_menu_gl11.cpp`, `draw_module_arraylist_gl11`: retain the existing R207 active-only runtime snapshot and all validation; preserve the translucent dark row background. Set `glDisable(GL_BLEND)` **only** around the 3px `rgb(accent_rgb)` / `quad` accent strip, then `glEnable(GL_BLEND)` for subsequent glyphs and rows. The enclosing `ScopedFixedFunctionState` already saves and restores the original GL attributes, blend enable/function and matrix state. This is not a global game GL-state modification.

`tests/test_windows_menu_gl11.cpp`: preserve the prior exact source-color assertion on row 1 and add the same strict exact-color assertion for the second active row. The original `same(before,snapshot())` checks verify full GL state restoration. The test still rejects non-active row `Timer`, source-invalid modules and unowned contexts.

## Local verified tests

The exact modified Windows Release source compiled successfully with MSVC, including `slinkywinky_menu_gl11`, its dedicated `slinkywinky_gl11_state_tests`, `slinkywinky_jni.dll` and the optional `slinkywinky_r214_gui_overlay.dll`. Targeted CTest:

- `isolated_wgl_menu_state_restore`: PASS, including exact two-row RGB assertions and GL state restoration.
- `r209_retained_jni_arraylist_owner_and_pixel`: PASS, actual Java 8 ↔ JNI ↔ WGL active-row readback.

The GitHub Windows CI environment remains the authoritative cross-driver acceptance gate. **Local test success alone does not prove the new runner pass; review the subsequent PR #2 head workflow run before marking this resolved.**

No PR #2 C++ policy code, module manager, game input queues or native event callbacks were edited by the Chat 1 fix. The original injectable client and live in-world HUD remain separate acceptance tasks; both PRs should remain draft/unmerged.
