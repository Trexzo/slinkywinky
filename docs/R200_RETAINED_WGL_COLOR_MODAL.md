# R200 — color picker integrated into the retained native WGL ClickGUI

**Project:** `Trexzo/slinkywinky` • `reconstruction/r1-core-fidelity` • [draft PR #1](https://github.com/Trexzo/slinkywinky/pull/1). Separate OpenSlinky source audit/integration PR #2 remains untouched.

## Functional gap closed

R136 already recovered 13 color-widget swatches, including GUI → Accent color at module catalog index 32, and provided a strict GL11 render/hit-test geometry. R198–R199 independently reconstructed the six-digit hex/RGB/HSL and eight recent-color state from the user's supplied Java and C++ reference archives. **But the real retained WGL menu refused a click on `RecoveredColorSwatch`.** The state model was neither visible as a popup nor reachable through the renderer.

R200 connects the same source-indexed native swatch to a first-party GL11 modal, with a hue bar (six HSL hue sectors), saturation and lightness bars, eight recent-color cells, current color and hex *display*, and Apply/Cancel buttons. All geometry is shared between the GL11 renderer and the renderer-neutral hit test (`menu_color_modal_geometry.hpp`). The actual WGL owner-thread modal click path now supports these controls as isolated GUI preview state.

## ABI safety / important fix during implementation

An initial approach added `MenuColorPopupPreview` to `MenuState` and a new snapshot field to `MenuRenderFrame`. This **changed the binary size of `WglRetainedMenuPreview`** and produced a real Windows test failure (exit `0xc0000409`). It was **not** committed.

The corrected implementation preserves all existing `MenuState`, `MenuRenderFrame` and `WglRetainedMenuPreview` data members and sizes. It reuses the existing `MenuDropdownPreview` slot with a new `ColorPicker` enum kind as a modal and source ownership token. Mutable per-picker HSL/recent-color values live in an isolated, mutex-protected sidecar map keyed to the `MenuState` instance; `MenuState::~MenuState` erases its sidecar. The GL11 host passes a **copied, optional** first-party picker state to a new renderer overload, while retaining both previous GL11 function overloads. No new WGL or JNI-facing fields are introduced.

A **clean MSVC rebuild** resolved stale-object duplicate symbols from the earlier layout. The pre-existing native WGL regression then passed again before R200's new end-to-end test was enabled.

## Safety and behavior

- A color modal opens only for an authentic source `RecoveredWidgetKind::Color` whose owning module is on the selected tab, expanded, within the current settings page and present in the actual visible catalog.
- A color modal owns the mouse hit surface: the nav bar, toggles, modules, sliders and background cannot receive clicks through it. Invalid or stale repaint geometry never creates future clicks.
- The new `scroll_visible(delta,visible_limit)` owner-thread controller uses the existing narrow-preview scroll contract. This fixes a compact GUI accessibility gap where the GUI module's color swatch was clipped when its card sat too low; it does **not** introduce live OS mouse wheel capture.
- Clicking HSL bars changes only the popup's 24-bit RGB preview; recent-color cells select the existing eight-color history. Apply commits **only** the currently visible source-indexed color preview using the original R136 admission guard. Cancel closes without changing the underlying color setting, while retaining the source-backed recent-color history behavior.
- The native profile serialization, gameplay enabled flags, global `MenuAppearance::accent_rgb`, JNI agent, game class instrumentation, event queue, and original LWJGL2 mouse/keyboard remain untouched.
- The popup's six-digit hex string is currently **display-only**. Keyboard editing, exact native popup geometry/animation and source-comparable screenshot parity are still incomplete; do not call this a 1:1 original Slinky picker.

## Tests

The existing disposable first-party WGL integration fixture `tests/test_windows_retained_pointer.cpp` now includes `verify_r200_native_color_popup`:
1. Navigate to the source-pinned GUI module on the Visual tab and scroll its three-row compact pane so the swatch is entirely visible.
2. Verify the actual last **painted** source hit type is `RecoveredColorSwatch` at the GUI accent widget; use the same R190–R193 paired synthetic input ledger and original WGL context/viewport to click it.
3. Verify the native modal frame's source owner and initial `#8B7CF6`, and that background tab navigation and hit tests are blocked.
4. Click the HSL hue bar; repaint; verify the RGB preview changes while profile serialization does not.
5. Click an initial black recent-color cell; repaint; verify RGB 0; Apply through the native modal; repaint; verify only source color override becomes black.
6. Reopen, change lightness, and Cancel; repaint; verify the previously applied color remains unchanged.
7. Verify detached input ledger restoration, zero vanilla Minecraft input suppression and exact GL11 state restoration.

`tests/test_r119_live_input_guard.py` separately forbids the new read-only `fixture_color_popup_rgb` method from the live Java agent/JNI bridge.

## Verification status

The R197–R199 pending Windows backlog was successfully rebuilt and retested against the current branch (**96/96 CTest PASS**) before R200 changes. During R200, the corrected ABI-safe implementation passed a clean targeted WGL regression rebuild, and the new source-indexed full color modal WGL test passed.

**R200 Windows x64 Release build PASS; complete CTest 96/96 PASS**. The native retained WGL regression includes the exact `R200_NATIVE_WGL_COLOR_POPUP_HSL_RECENT_APPLY_CANCEL_PASS` receipt, plus the prior R192–R197 input/repaint safeguards. A clean targeted MSVC build also passed after the ABI-preserving correction. The entire suite included Java 8/JNI and native agent attach, source guard checks, menu rendering and GL11 state restoration.

**Isolated original vanilla Minecraft 1.8.9 read-only acceptance PASS, process exit 0**. The acceptance reached `R99_ORIGINAL_189_RETAINED_MENU_RECEIPTS_PASS`, `R176_ORIGINAL_VANILLA_189_HOVER_COUNTERS_PASS`, and source geometry checks, including **60** source-eligible slider draws, **63** passive observations and **zero foreground observations** (`foreground=0 exact_hits=0 numeric=0 bind=0`). The existing original render-only acceptance therefore remains intact. No real foreground mouse or keyboard input was exercised, and original gameplay input delivery remains disabled.

No claim of byte-for-byte, pixel-perfect, or full 67-module original Slinky parity is made. The current R200 popup layout and painting are **first-party approximations** derived from Java/C++ source semantics. Exact original modal dimensions, font treatment, animation, hex keyboard editing, drag-while-held HSL behavior, and same-frame visual comparison remain separate work.
