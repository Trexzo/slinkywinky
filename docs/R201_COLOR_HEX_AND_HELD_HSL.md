# R201 — native retained WGL color hex editing and held HSL drag

**Scope:** `Trexzo/slinkywinky` / `reconstruction/r1-core-fidelity` / draft PR #1. Independent C++ PR #2 is not edited.

## Gaps closed after R200

The R200 native GL11 color popup offered a painted HEX display and clickable HSL bars, but **HEX was not editable** and an HSL track only changed its local preview when the mouse was released. Its mouse-only modal hit fence also left one subtle gap: the independent keyboard router could still translate Tab, arrows, and Enter/Escape into background navigation unless the retained WGL wrapper blocked that action separately.

R201 connects first-party simulated LWJGL keyboard events through the same R190–R193 synthetic, exact WGL/epoch/owner-thread ledger used by retained pointer tests, and adds held-mouse HSL adjustment without changing Minecraft input behavior.

## Six-digit hex editor

`MenuColorPopupPreview` now owns an in-memory `hex_editing_` flag and six-character `hex_buffer_`. Selecting the source-grounded `ColorPopupHexField` clears the draft and displays `#|`; printable ASCII hex digits append until a maximum of six. RGB/HSL is **unchanged for partial drafts**. Exactly six valid digits update the local RGB swatch and synchronized HSL; Backspace and another valid sixth digit update it again. Invalid hex glyphs and input over six are rejected; Enter finishes only the field edit, leaving the dialog open. A subsequent Escape outside an active edit closes the modal but never the actual retained GUI. Clicking an HSL bar or recent swatch exits hex edit, as does closing the dialog.

The test fixture maps only fixed, documented LWJGL2 **physical codes** for top-row 0–9, A–F, Backspace, Enter, Escape (and uses Tab to prove background navigation stays blocked). This is **not** Unicode/IME keyboard input, paste, text selection, or layout-independent desktop text handling. The test stream is synthetic and never touches an OS key hook, Java agent, original Minecraft input queue, or gameplay setting.

## Modal input ownership

`MenuKeyboardRouter::event` now checks `MenuNavigation::color_modal_open()` on every rising key. While the modal is active it maintains its exact paired key pressed/released ledger but translates **no** navigation or top-level GUI close action. Only `WglRetainedMenuPreview::fixture_keyboard_event` can forward a newly admitted, foreground-attested **synthetic rising** key to the first-party `MenuState::color_popup_key` model; a repeat key-down does not generate another digit.

`MenuNavigation::{select_tab,focus_visible_row,apply}` also deny actions directly while the popup owns the surface. The existing R200 `WglRetainedMenuPreview::apply` and hit-test gating remain in force.

## HSL while mouse is held

`WglRetainedMenuPreview::pointer(Move)` now updates the local popup HSL **during** held motion, after `MenuMouseGesture::move` has verified the same original painted HSL track and input ownership. The new `color_hsl_ratio_at` helper is shared by held Move and release, ensuring one logical-coordinate conversion. A full GL11 redraw while still holding the synthetic mouse button is supported and exercised. Leaving the verified track cancels the gesture, rather than extrapolating ownership to unobserved areas. This is currently stricter than many native drag controls: pointer capture outside the bar and pixel-perfect source timing remain future comparison gates.

## Binary compatibility, tests, and limitations

No data members were added to `MenuState`, `MenuRenderFrame` or `WglRetainedMenuPreview`; new editing fields live exclusively inside the separately allocated R200 popup sidecar. Existing native ABI and JNI/JVM-facing object sizes remain unchanged. No gameplay modules were enabled.

R201 adds Release-active model assertions to `tests/test_menu_color_popup_preview.cpp` and extends the real disposable WGL regression in `tests/test_windows_retained_pointer.cpp` to:
- Open the exact source GUI Accent color widget.
- Hold/drag the hue bar, verify RGB changes on the Move edge **before Up**, and redraw while still held.
- Select the painted hex hitbox, reject background Tab navigation, type `1a2b3c` through paired synthetic physical key Down/Up edges, verify RGB is still black before the sixth digit, Backspace and change the final digit to `d`, end with Enter and Apply `0x1A2B3D`.
- Reopen and hit Escape, ensuring the color modal closes while the native menu stays open and the color override and native serialized profile remain unchanged.
- Preserve exact GL11 state and restore the synthetic key/mouse ledger without ever authorizing original vanilla input suppression.

**Not complete 1:1 Slinky**: the modal size, fonts, animation, color blending, input timing, proper text selection/IME, clipping/capture behavior, screenshot fidelity, live foreground-exclusive Minecraft keyboard/mouse ownership, and the original gameplay modules all remain unverified or incomplete. Source references are candidate reconstructions rather than authenticated original vendor releases.

## Verification

The R201 updated Release-targeted color model and native WGL pointer tests passed during targeted Windows/MSVC checks. A clean Windows rebuild was required after an enum addition because stale incremental objects produced a false previous slider-hit mismatch; the clean native pointer test passed and targeted tests remained green after the hex and drag changes.

**Verified on Windows:** the full MSVC x64 Release build and CTest suite passed **96/96** tests, including the extended R201 native WGL fixture, R198 color popup model, Java/JNI and native agent-attach checks, and the R160 no-unverified-live-input-hook guard. Earlier, an incremental build showed a stale slider enum hit mismatch; a full clean targeted rebuild resolved it and the final complete Release build succeeded. The standalone native WGL test exercises source swatch → HSL held MOVE → redraw while held → UP → HEX focus → paired digits `1a2b3c` → Backspace → `d` → Enter → Apply → reopen → Escape with expected local profile invariants.

**Verified on isolated original vanilla Minecraft 1.8.9:** process exit **0**; `R99_ORIGINAL_189_RETAINED_MENU_RECEIPTS_PASS`; R179 source-visible rectangle **60** draws, **0** exact mouse hits; R176 **63** passive observations, **0** foreground, numeric or bind hits. No original foreground input or physical click certification occurred. Original game input remains disabled and unmodified.

The mutable color modal remains a pure local preview, with no first-party gameplay module activation and no assertion of pixel-perfect original-Slinky fidelity.
