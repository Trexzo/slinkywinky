# R128 — Source-shaped Unload confirmation in the native preview

**Project:** Trexzo/slinkywinky, draft PR #1. Do not merge as a playable release.

## Source evidence

The user's `decrypted files.zip` candidate includes `re/clickgui.tpl.html`. Its reconstructed seven-tab layout names the final tab **Unload**, with red text. Clicking that tab opens a centered confirmation instead of selecting a seventh ordinary content page. The template's modal uses a **600px** maximum-width #141414 card with **20px** corners, three **20px** rows, **8px** padding, **12px** row gaps, a gray close X and a full-row Continue control. Its strings are **Are you sure?** and **Unload Slinky?**.

These visual constants are source-derived from a *reconstructed* template and not evidence of exact runtime equivalence to an authenticated vendor release.

## Implementation

- `MenuState` adds a separate inert `unload_confirmation_preview` state. It does not select the Unload pseudo-tab, does not alter any gameplay configuration, and cannot call a native unload.
- Immutable `MenuRenderFrame` snapshots and `MenuMouseGesture` comparisons invalidate held clicks whenever the modal state changes.
- The seventh nav tab now has its own hit type `UnloadTab`, unlike ordinary module tabs.
- `menu_unload_modal_geometry.hpp` calculates the recovered centered modal shape and the same explicit X/Continue rectangles for renderer and hit tester.
- The modal owns mouse input: only X or Continue are click-admitted, with absolutely no click-through to underlying tabs, cards, and toggles. Keyboard navigation is blocked while the confirmation is shown; Escape or Enter dismisses it.
- Continue closes the *preview dialog only*. The visible button is explicitly labeled **Continue (preview only)** to avoid pretending to perform a client unload. No JVM, DLL, Forge, Java Attach, packet or gameplay code is changed.
- Loss of foreground/window focus dismisses the modal.

## Validation

The new `native_menu_unload_confirmation_preview_safety` CTest tests all three standalone/bounded/compact panel modes, seventh-tab hit geometry, exact close/continue targets, drag-across rejection, modal click isolation, navigation refusal, popup state revocation and unchanged runtime settings.

**Verification status:** Changes and tests committed on the existing draft branch. Windows CTest, OpenGL screenshots, GitHub Actions and genuine Minecraft acceptance have **not** been confirmed for this R128 head.

## Still outstanding

Actual safe client unload/reinitialize, the complete original ClickGUI's remaining widget interactions/typography/animations/profiles, original 1.8.9 safe input ownership and the 67 modules' independently verified gameplay logic.
