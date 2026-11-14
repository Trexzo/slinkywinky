# R203 — actual WGL color-popup screenshots and measured source geometry

**Repository:** `Trexzo/slinkywinky`, `reconstruction/r1-core-fidelity`, draft PR #1. PR #2 is independent and remains untouched.

## Reference-backed differences fixed

The candidate native C++ source [`gui_popups.cpp`](https://github.com/Skid-Technology/OpenSlinky/blob/main/src/engine/gui/gui_popups.cpp) explicitly constructs the Hue / Saturation / Lightness control tracks with `font_size_15_in_gui_popups` (15 logical pixels), and its `Recently used` swatch row with `row_height_25` (25 logical pixels). The independent Java source also places these rows in the same color-popup sequence, although its numerical constants are partially indirect.

Our R202 first-party `menu_color_modal_geometry` still used **17px** HSL tracks and **20px** recent-color cells. R203 corrects both to **15px** and **25px**. The existing GL11 draw, six-sector source hue gradient and native pointer hit test all share this geometry, preventing a new render/hit mismatch. The popup's fixed width/height, font family and animation **remain our approximations**; the C++ candidate calculates height dynamically.

## Captured real first-party render pixels

R203 enabled opt-in `GL_BACK` framebuffer capture **only** from the isolated 676×630 native WGL test window using the project's existing `save_gl_framebuffer_ppm`. These are real OpenGL frames, not mockups and **not screenshots of Minecraft or another user's desktop**. Two frames were captured: immediately after source-indexed GUI Accent color opens at `#8B7CF6`, and after held HSL hue drag changes the local color to `#7CB5F6`.

<details><summary>First-party screenshot receipts</summary>

- [Original popup before hue drag](screenshots/r203-color-popup-original-wgl.png) — SHA-256 `f07092d4fb0c775023d9bc8fa41ea7b3d21d2f8f22f07c360e0e94602068ff93`, 18,344 bytes
- [Popup after hue drag](screenshots/r203-color-popup-hsl-wgl.png) — SHA-256 `31ebec6b76a76c8b63e135ee56d76378114b7e4e9bafaac68a0c350eb9ac1565`, 17,380 bytes

</details>

Both PNGs are converted from those exact PPM pixel buffers, without retouching. `tests/r203_color_modal_visual_acceptance.py` is an opt-in, repeatable Python/Pillow visual receipt tool for the 676×630 native fixture. It checks pixel evidence (not cosmetic screenshot similarity):

- Actual swatch center: **(139,124,246) = #8B7CF6** before drag; **(124,181,246) = #7CB5F6** after it. Intermediate 5/6 hex drafts and the mouse-down/up ownership rules are separately covered by R201/R202 native tests.
- Five distinct hue-ramp colors with full red/green range, nonconstant saturation gradient and black-to-white lightness endpoints.
- The 8-slot history's initial **black** swatch and 20px source title close cross remain the same RGB across both captures.
- The isolated framebuffer clear at pixel (10,10) remains unchanged.

The raw full-frame comparison found **26,324 changed pixels** with a bounding box (98,105)–(578,525). Using the measured modal rectangle (211,211)–(465,419), **3,178 changed pixels** were within the popup and **23,146 outside**. The rest of the GUI is rendered during ongoing transition/animation, so this is **not evidence of a completely static background or original-Slinky pixel parity**. R203's visual test does not assert that outside-modal pixels are unchanged. The test is scoped to the known on-PC fixture viewport, not a general-purpose screenshot matcher.

## OpenGL state restoration hardened

The native first-party WGL `State` test snapshot now includes `GL_SHADE_MODEL` and `GL_LINE_WIDTH`. In the actual color-popup fixture R203 deliberately sets **GL_FLAT** and line width **3px**, paints the 6-sector gradient with its 2px outlined circular knob (the renderer uses `GL_SMOOTH` and `glLineWidth(2)`), and verifies both initial caller values are restored afterward, along with the previously verified matrix, scissor, blend, viewport and other GL state. No original Minecraft input hooks or native field layout changes.

## Reproduction

Run from the Windows repository root after a Release build with the same disposable WGL context:

```powershell
$env:SLINKYWINKY_R203_COLOR_MODAL_PPM="$PWD\build-r184-clean\r203-modal-original.ppm"
$env:SLINKYWINKY_R203_HSL_CHANGED_PPM="$PWD\build-r184-clean\r203-modal-hsl.ppm"
& "$PWD\build-r184-clean\Release\slinkywinky_retained_pointer_tests.exe"
python tests\r203_color_modal_visual_acceptance.py --before "$env:SLINKYWINKY_R203_COLOR_MODAL_PPM" --after "$env:SLINKYWINKY_R203_HSL_CHANGED_PPM" --out docs\screenshots
```

Pillow is required **only** for this opt-in PNG/report tool, not for normal native/Java CTest. Both images are static, explicitly selected captures; they contain no original vendor fonts, images, decrypted binary or actual game state.

## Verification and scope

**Verified on the connected Windows PC:** complete x64 MSVC Release build and **96/96 CTest PASS**, including native WGL visual control, Java/JNI, native agent attach and no-unverified-live-input guards. The opt-in readback tool passed `R203_SOURCE_HUE_SATURATION_LIGHTNESS_PIXELS_PASS` and `R203_WGL_MODAL_SWATCH_HSL_RECENT_AND_CLOSE_PIXEL_PASS`. The new adversarial GL shading/line-width test passed in the native WGL CTest and did not alter the existing source-indexed input routing.

**Verified on isolated original vanilla Minecraft 1.8.9:** supervised read-only process exited **0**, `R99_ORIGINAL_189_RETAINED_MENU_RECEIPTS_PASS`; source-eligible slider draws **60**, passive observations **63**, foreground **0**, exact input hits **0**, numeric/bind hits **0**. This is original-client *rendering-only* acceptance, not proof of physical foreground keyboard/mouse or gameplay module fidelity.

**Still incomplete:** authenticated original Slinky screenshots at identical viewport to serve as pixel-level ground truth; exact dynamic modal dimensions, NanoVG font metrics, rounded gradient segment ends, animation timing, physical foreground mouse/keyboard ownership and most gameplay module behavior. Passing the popup fixture cannot establish those separately. Keep PR #1 draft and live gameplay input disabled.
