# R205 — source-accurate eight-color history row layout and WGL pixel evidence

**Branch:** `reconstruction/r1-core-fidelity` on [Trexzo/slinkywinky draft PR #1](https://github.com/Trexzo/slinkywinky/pull/1). Independent C++ PR #2 was not modified.

## Grounded discrepancy from reference

The candidate native C++ reconstruction's `src/engine/gui/gui_popups.cpp`, during `case 2` color-popup rendering, creates a 25 logical-pixel `Recently used` row. The common popup calls `begin_content(..., 12.0f)` to establish an inset, then the color branch uses:

```cpp
R16d rr = add_cell(gui, row_height_25);
float sz = (rr.w + offset_neg_56) * factor_0_125_b;
float step = corner_radius_8_in_gui_popups_c + sz;
// cell x increments by step, repeated eight times.
```

The C++ symbols describe a **12-pixel content inset**, **eight 25px-high swatches**, **(content_width − 56)/8** swatch widths, and **8-pixel** between-swatch spacing.

The previous R203/R204 first-party WGL popup instead used **18-pixel insets and 6-pixel gaps**, although the total row height was correct. Both C++ and Java reconstructions provide useful candidate comparisons, but neither is independently authenticated as original Slinky; do not infer verified 1:1 source parity.

## Changes made

- `include/slinkywinky/menu_color_modal_geometry.hpp`: source-guided 12-pixel insets; `recentContentWidth=width-24`; `recentWidth=(recentContentWidth-56)*0.125`; x increments by `recentWidth+8`. The GL11 renderer and synthetic WGL hit testing already use this **same immutable geometry**, so all eight click targets stay aligned. No additional members, native object-size changes, Minecraft input hooks or game profile changes.
- `tests/test_menu_color_popup_preview.cpp`: Release-active mathematical assertions for exact source padding, eight swatch widths, nonoverlap, precisely 8-pixel gaps, and 12-pixel right inset.
- `tests/r205_recent_color_row_visual.py`: opt-in strict Pillow audit of a **versioned R204 first-party screenshot** versus an R205 color-modal `GL_BACK` readback at 676×630. The check fails if pixels change outside the recent-color row, and pins the changed black swatch boundaries, the unchanged color preview, HSL slider, title cross, and background.
- Two new real first-party WGL PNG receipts were generated from the clean-built disposable OpenGL test fixture. No desktop screenshots, original client graphics, decrypted binaries, or vendor font assets were committed.

## WGL screenshots and measured change

- [R205 initial modal](screenshots/r205-color-popup-recent-spacing-wgl.png), 18,651 bytes, SHA256 `b53adb5b7a34e5f0db52d7673b25ebb149b55a37e5c56d137bc116349e0a3e46`.
- [R205 after held HSL drag](screenshots/r205-color-popup-recent-spacing-hsl-wgl.png), 17,643 bytes, SHA256 `c00e1023a32b3d44a598f2a5747c2b5146a10dc410ea3e0dc41d670edf246fa5`.

The strict R204→R205 compare reports **1,210 changed RGB pixels**; bounding box `(220,382)–(456,409)`; **all 1,210 inside the color-history row** and **zero outside**. The program verified exact before/after color samples at the relocated first and last recent swatches, while the purple `#8B7CF6` preview, HSL track, close-cross and other menu/background sample pixels were unchanged.

The very first incremental Windows build produced **zero visual difference** despite the source change: this was stale output in the existing checkout under `AppData\Local\Temp`. We rejected that evidence, then ran a **clean MSVC rebuild of the retained-pointer test and its dependencies**, recaptured both PPMs, confirmed native WGL click acceptance, and only then measured the actual 1,210-pixel geometry change. Windows Build Tools warns `MSB8029` about the Temp build directory, which should be moved to a stable location before relying on incremental builds.

The standalone visual checker emitted:
- `R205_SOURCE_RECENT_PADDING_AND_8PX_SPACING_PIXELS_PASS`
- `R205_PIXEL_DELTA_CONFINED_TO_COLOR_HISTORY_ROW_PASS`

These are precise **first-party fixture** comparisons, not authenticated pixel-perfect comparisons with original Slinky.

## Test and acceptance gates

**Verified on the connected Windows PC:** a fresh clean targeted MSVC build passed, the changed source-geometry and native WGL CTests passed, and the full Windows x64 Release build with **96/96 CTest PASS** completed (including Java/JNI, native agent attach, WGL input fixture, GL-state restoration, and the R160 fail-closed live input guard). The optional local Pillow fixture independently passed `R205_SOURCE_RECENT_PADDING_AND_8PX_SPACING_PIXELS_PASS` and `R205_PIXEL_DELTA_CONFINED_TO_COLOR_HISTORY_ROW_PASS` with 1,210 row-local changed pixels.

**Isolated original vanilla Minecraft 1.8.9 read-only acceptance PASSED (exit 0)**: source-visible slider draws **60**, passive observations **63**, **foreground observations 51**, exact mouse hits **0**, numeric and bind hits **0**; R99 retained overlay receipt confirmed. This is a materially different focus count from the previous zero-foreground runs, but it is **not** a verified physical click, GUI event ownership, or gameplay behavior: the acceptance JSON explicitly reports `visual_game_acceptance=false`, `in_game_menu_verified=false`, and `scope="R81 original vanilla render receipts only"`. No original keyboard/mouse event was delivered or suppressed.

Physical foreground mouse/keyboard exclusivity and all gameplay module behaviors remain outside R205's evidence.

## Remaining work

This change corrects one source-measurable visual mismatch. Popup title/section font identity and metrics, NanoVG dynamic layout, original animation/fade timing, edge antialiasing and full screenshot alignment are not established by this source alone. Original-game input and the majority of the 66/67 catalogued module functions remain unverified or incomplete. Keep PR #1 draft, live event suppression disabled, and PR #2 independent.

## Reproduce the visual audit

```powershell
$p='C:\Users\Felix\AppData\Local\Temp\SlinkyWinky-R172-Chat'
$env:SLINKYWINKY_R203_COLOR_MODAL_PPM="$p\build-r184-clean\r205-modal-original.ppm"
$env:SLINKYWINKY_R203_HSL_CHANGED_PPM="$p\build-r184-clean\r205-modal-hsl.ppm"
& "$p\build-r184-clean\Release\slinkywinky_retained_pointer_tests.exe"
python "$p\tests\r205_recent_color_row_visual.py" --old "$p\docs\screenshots\r204-color-popup-rounded-wgl.png" --new "$env:SLINKYWINKY_R203_COLOR_MODAL_PPM" --new-hsl "$env:SLINKYWINKY_R203_HSL_CHANGED_PPM" --out "$p\docs\screenshots"
```
