# R136 — Source-indexed native color controls (preview only)

**GitHub:** Trexzo/slinkywinky · `reconstruction/r1-core-fidelity` · draft PR #1, not merged.

## Source and authenticity

The user-supplied candidate `decrypted files.zip` contains `re/clickgui.tpl.html`, whose `it.k==='col'` branch presents a clickable colored swatch with an `input type=color`. This is a reconstructed HTML reference, **not authenticated official Slinky GUI code**, so it establishes the *type of interaction*, not the exact dialog implementation or color-picker appearance in the proprietary native client.

The source-derived GUI fixture currently has **13 Color records** with candidate `#RRGGBB` hex values. Their labels and swatches are source-derived, while default values should still be treated as unverified until native registration provenance is checked.

## Implementation

- `menu_parse_color_hex` rejects malformed, missing-prefix, truncated, extended and non-hex values; the source index must point to a real `RecoveredWidgetKind::Color` record with a valid hex default.
- `menu_color_swatch_geometry` mirrors the currently drawn 42×22 right-aligned GL11 swatch exactly. It checks module identity, correct selected tab, expanded/visible five-control page, recovered source identity and full card clipping.
- `MenuState::set_recovered_color_preview` validates a 24-bit RGB value, source-callsite identity, visible page and modal/popup exclusion. It writes an **independent, optional 385-slot RGB preview array**, never `MenuAppearance::accent_rgb`, a saved profile, a gameplay module or original client state.
- `MenuRenderFrame` snapshots RGB values; stale mouse-clicks are refused if a color changes while a pointer gesture is pending. GL11 draws a transient widget copy with the local RGB override.
- A successful same-widget mouse-up opens the **stock Windows ChooseColorW** picker through `comdlg32` in the standalone preview. `CC_RGBINIT|CC_FULLOPEN` starts from the currently displayed color. Cancel leaves the model unchanged; on successful selection, BGR `COLORREF` is explicitly converted into internal `0xRRGGBB` and source/visibility admission is repeated. The dialog is **not an injected game UI**, and its native Windows look is a first-party approximation of the HTML reference's color input.

## New test

`native_recovered_color_preview_safety` covers all thirteen source colors, strict hex parsing, the GUI Accent color `#8b7cf6` source fixture, exact color swatch hitbox in all three panel modes (using the R135 adaptive scroll path), rejecting label/wrong-module/wrong-tab/oversized RGB inputs, same-widget mouse-down/up, unchanged global GUI accent and module states, stale mouse-frame refusal, and modal exclusion.

**Caveat:** The Windows color-picker dialog itself cannot be exercised by a Linux pure-C++ unit test. Full Windows Release, CTest, interactive picker cancellation/selection, screenshot comparison and Minecraft 1.8.9 acceptance remain unverified for this commit because the user's connected desktop was offline.

## Next work

Six source text fields, two action buttons, profile-card actions and binding, original typography/animation/reference-diff, and in-game input ownership/restoration are unfinished. The 67 gameplay modules are not complete.
