# R202 — color popup close-cross and six-sector gradient source convergence

**Project:** `Trexzo/slinkywinky` / `reconstruction/r1-core-fidelity` / [open draft PR #1](https://github.com/Trexzo/slinkywinky/pull/1). PR #2 is separately maintained and **not changed** in this task.

## What was actually wrong

R200/R201 implemented a practical local color-picker preview with **Apply** and **Cancel** buttons in its footer. Those controls did **not match** the examined C++ candidate reconstruction: [Skid-Technology/OpenSlinky `src/engine/gui/gui_popups.cpp`](https://github.com/Skid-Technology/OpenSlinky/blob/main/src/engine/gui/gui_popups.cpp).

The candidate native source has:
- A shared 20px title row with a **right-hand X**. The `case 2` color picker sets `res=0`, and the common footer invokes `set_popup_open(gui,true)` when the X is clicked. The `apply` flag runs the callback for the selected RGB and records unique recent colors.
- A 24px color preview/hex row, six interpolated hue sectors, saturation and lightness bars with rounded 5px tracks, current-color circular markers (radius 10px, 2px outline), a `Recently used` section and eight outlined rounded swatches.
- **No separate Apply/Cancel footer for this color picker.**

The independent Java candidate [`ClickGuiScreen.java`](https://github.com/meowflags/OpenSlinky/blob/main/SlinkyClient/src/main/java/dev/slinky/client/gui/ClickGuiScreen.java) likewise renders an editable hex field, three color slider rows with 10px circular outlines, and eight recent colors. These are **candidate reconstructions**, not authenticated original vendor source code. They provide strong convergent implementation evidence but cannot by themselves prove pixel-perfect original behavior.

## Changes in R202

- `menu_color_modal_geometry.hpp`: replace synthetic `cancel/apply` rectangles with one 20px source-style title `close` hit rectangle; reduce oversized modal height from 312 to 294 logical pixels, removing the reserved footer gap. Width, global scaling, label placement and overall animation remain **approximate first-party layout**; the C++ candidate dynamically computes popup height in NanoVG rather than using our fixed GL11 layout.
- `menu_hit_test.hpp`, `menu_hit_test.cpp`, `windows_retained_menu_preview.cpp`: remove `ColorPopupCancel`/`ColorPopupApply` synthetic hit types and their actions. Add `ColorPopupClose`, which routes only to `menu_.close_color_popup(true)` within the current source-indexed modal and owner-authenticated synthetic WGL pointer path. It commits **only a local preview override** and never Minecraft gameplay data. The separately admitted synthetic Escape path remains a safe local cancel; this is not established as original-Slinky Escape equivalence.
- `windows_menu_gl11.cpp`: paint the title X instead of false action buttons, draw six genuine GL11 smooth-interpolated hue gradient sectors (one gradient for saturation, one for lightness), draw colored circular HSL knobs with a two-pixel dark border and 5px outlined recent-color cells. This replaces the previous 96 flat-color strips and 2px bar markers. GL state restoration is covered by native WGL tests, and no shader, NanoVG dependency or original binary is embedded.
- `tests/test_windows_retained_pointer.cpp`: exercise genuine WGL frame source-swatch→picker→HSL drag→recent black→**title-X click commits**→reopen→synthetic Escape cancels→edit hex `1a2b3d`→**title-X commits**→reopen→synthetic Escape closes without exiting the main menu. Verify there are **no hit-tested fake footer buttons**, no underlying native profile mutation, no live vanilla input permission, and exact GL-state restoration.
- `tests/test_menu_color_popup_preview.cpp`: source-informed, always-active C++ geometry checks for a centered title X with no overlap with hex/HSL/recent geometry and fail-closed viewport constraints.

## Verification and boundaries

**Verified Windows x64 MSVC Release:** complete project build passed and **96/96 CTest tests PASS**, including the R202 title-X native WGL control, portable geometry test, R160 no-unverified-live-input guard and JNI/native agent attach. The new enum needed a clean targeted rebuild first; that passed, followed by the full Release build and complete suite. The native test confirms source swatch → hue held-drag → recent black → title X commits black → reopen → synthetic Escape cancels → hex entry `1a2b3d` → title X commits it, plus negative checks that the former footer areas are no longer actionable. No original Minecraft input event was delivered or suppressed.

**Isolated original vanilla Minecraft 1.8.9:** read-only acceptance exited **0**, `R99_ORIGINAL_189_RETAINED_MENU_RECEIPTS_PASS`; R179 source-eligible slider draws **60**, exact hits **0** and foreground **0**; R176 **63** passive observations, foreground **0**, numeric and bind hover hits **0**. This is background-only rendering acceptance, not a physical click or exclusive keyboard/mouse input test.

The native GL11 implementation is an original, first-party approximation: **no screenshot pixel-parity proof, matching NanoVG font metrics, exact native popup dimensions, animation curve or physically delivered original-game input ownership** has been established. Most original Slinky gameplay modules are still unimplemented, and this UI work does not change that. Keep PR #1 draft and do not merge into unverified original-game input paths.

## Next prioritized work

Compare captured native WGL popup frame against reference images/actual decompiled graphics parameters at matched viewport scaling; then address remaining mismatched rounded gradient edges, font metrics and popup reveal animation, before treating GUI fidelity as complete. In parallel, pursue independently attested real foreground Minecraft event ownership and module-by-module behavior acceptance—never infer live readiness from synthetic WGL fixture success.
