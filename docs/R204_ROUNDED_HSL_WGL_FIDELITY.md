# R204 — rounded HSL gradient edges with real-WGL pixel isolation

**Scope:** `Trexzo/slinkywinky`, branch `reconstruction/r1-core-fidelity`, open draft PR #1. Independent OpenSlinky PR #2 untouched.

## Exact source mismatch

The user-supplied candidate C++ OpenSlinky reconstruction corresponds to [`gui_popups.cpp`](https://github.com/Skid-Technology/OpenSlinky/blob/main/src/engine/gui/gui_popups.cpp) and was compared alongside the Java candidate. Its `case 2` color popup paints six hue sectors with `nvg_RoundedRectVarying`: the first sector has **5px rounded left corners**, the last sector **5px rounded right corners**, and the four internal sectors remain rectangular. Its shared `hsl_bar` paints the Saturation and Lightness bars using a 5px `draw_rect_rounded` around a NanoVG gradient from `(x,y)` to `(x+w,y+h)`.

The R203 first-party GL11 renderer had accurate 15px track heights and six color sectors but painted **all three bars as full rectangles**. This was confirmed in the actual captured R203 native WGL PNG: hue pixels at the top-left and top-right corners were painted, where the candidate's rounded shape should expose the dark popup background.

## First-party implementation

- `include/slinkywinky/menu_color_modal_geometry.hpp`: adds `menu_color_bar_span` — a pure, guarded, renderer-neutral vertical-span function for a 5px rounded rectangle. At either extreme x, the visible span contracts to the middle 5px of a 15px bar; at 5px inward and in all middle sectors it covers the full 15px height. Rejects nonfinite, reversed/out-of-rectangle or nonpositive input. Does **not** alter the existing rectangular mouse hitbox, so native mouse ownership remains source-indexed and its earlier acceptance remains intact.
- `src/windows_menu_gl11.cpp`: replaces the unrounded GL11 fill quad with `color_popup_rounded_gradient`. It uses the same six Hue HSL endpoints as R202 with horizontal interpolation, clips the combined outer edges using the source-shaped quarter-circle span, and approximates NanoVG's diagonal Saturation/Lightness gradient along the full-bar `(x,y) → (x+w,y+h)` vector. Draw is first-party `GL_QUADS` with subpixel-scale columns and smooth per-vertex colors. No NanoVG library, original fonts, vendor shaders, assets or game hooks are added. The existing R203 GL-state guard checks caller shade mode and line width restoration.
- `tests/test_menu_color_popup_preview.cpp`: always-active mathematical tests for both 5px endcaps, full-height center, partial 2.5px arc position and invalid inputs.
- `tests/r204_color_modal_rounded_visual.py`: opt-in Pillow test comparing exact first-party 676×630 screenshots with versioned R203. It checks the change region, strict corner pixels, unchanged swatch/title cross/history/background and stable source frame size, then writes lossless PNG receipts.

## Actual framebuffer evidence

The images were captured from the user's **disposable, private GL11 WGL test context** via the existing `GL_BACK` framebuffer readback, not a screenshot of another application, the desktop or Minecraft. The WGL pointer/keyboard ledger is synthetic; actual game input remains disabled.

- [R204 source color modal, rounded](screenshots/r204-color-popup-rounded-wgl.png) — PNG 18,690 bytes; SHA-256 `be6ed16e89c145bc8412d654fc5edfd89ab476f5373e4d5a31684236e881c08e`.
- [R204 modal after HSL hue drag](screenshots/r204-color-popup-hsl-wgl.png) — PNG 17,679 bytes; SHA-256 `27cd4f29d91e193894ecc74e76365f3d6b80ea70dea85a9a41cb4d171140f3a8`.

The strictly version-matched R203 → R204 initial frame diff reports **1,332 changed pixels** with bounding box `(225,283)–(451,367)`, confined to the three HSL bars. Specific RGBA screenshots show:

| Pixel (x,y) | R203 RGB | R204 RGB | Rationale |
|---|---|---|---|
| (225,283), Hue upper left | (246,127,124) | (20,20,20) | First-sector curved corner |
| (450,283), Hue upper right | (246,124,127) | (20,20,20) | Last-sector curved corner |
| (225,320), Saturation upper left | (185,185,185) | (20,20,20) | Rounded end |
| (225,366), Lightness lower left | (1,1,1) | (20,20,20) | Rounded end |
| (230,252), RGB preview swatch | (139,124,246) | unchanged | Source color unaffected |
| (442,228), close title X | (153,153,153) | unchanged | Other modal UI unaffected |
| (225,400), recent black | (0,0,0) | unchanged | History unchanged |
| (100,100), background | (26,97,176) | unchanged | No desktop/other UI effect |

`R204_ROUNDED_HSL_EDGES_EXACT_PIXEL_PASS` and `R204_ONLY_HSL_GRADIENT_REGION_CHANGED_PASS` were observed on the user’s Windows PC in the opt-in visual check. The screenshot diff is **not** an authenticated original-Slinky comparison. It proves our source-grounded rendering change and output isolation against our earlier WGL fixture only.

## Verification and limitations

**Verified on the user's connected Windows PC:** the complete MSVC x64 Release build and **96/96 CTest PASS**, covering the R204 rounded geometry checks, source-indexed native WGL pointer fixture, original-state restoration, Java/JNI, agent attach and the existing R160 no-unverified-live-input-hook safeguards. The standalone strict R203→R204 PNG comparison produced `R204_ROUNDED_HSL_EDGES_EXACT_PIXEL_PASS` and `R204_ONLY_HSL_GRADIENT_REGION_CHANGED_PASS`, with **1,332** changed pixels confined to the three HSL bars, and stable preview/title/history/background sample pixels.

**Isolated original vanilla Minecraft 1.8.9 read-only acceptance PASS (exit 0)**; `R99_ORIGINAL_189_RETAINED_MENU_RECEIPTS_PASS`, **60** source-visible slider draws, **63** passive observations, **0** exact mouse hits and **0** foreground input observations (numeric and bind both **0**). This remains background-only native rendering acceptance; it is **not** an exclusive physical keyboard/mouse or gameplay feature demonstration.

Remaining open: real signed/original vendor screenshot comparison with aligned fonts and viewport, original NanoVG edge antialiasing, interpolation and animation timing, dynamic popup layout/font metrics, independently verified actual foreground keyboard and mouse ownership, and nearly all 67 original gameplay behaviors. The mouse hitbox retains its original rectangular geometry on purpose, as the candidate calls its control hit function on the rectangular `hb`/ `hsl_bar` target. Do not claim this standalone WGL refinement is 1:1 original Slinky or real gameplay functionality.

## Reproduction

```powershell
$p='C:\Users\Felix\AppData\Local\Temp\SlinkyWinky-R172-Chat'
$env:SLINKYWINKY_R203_COLOR_MODAL_PPM="$p\build-r184-clean\r204-modal-original.ppm"
$env:SLINKYWINKY_R203_HSL_CHANGED_PPM="$p\build-r184-clean\r204-modal-hsl.ppm"
& "$p\build-r184-clean\Release\slinkywinky_retained_pointer_tests.exe"
python "$p\tests\r204_color_modal_rounded_visual.py" --old "$p\docs\screenshots\r203-color-popup-original-wgl.png" --new "$env:SLINKYWINKY_R203_COLOR_MODAL_PPM" --new-hsl "$env:SLINKYWINKY_R203_HSL_CHANGED_PPM" --out "$p\docs\screenshots"
```

The screen receipts and pixel guard are optional developer checks and are not executed during normal gameplay.
