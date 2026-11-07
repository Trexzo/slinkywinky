# R125 — Real mouse hit-testing for Piercing sandbox controls

**Project:** Trexzo/slinkywinky, native injectable architecture, existing draft PR #1.  
**Status:** Standalone Windows GUI only. Minecraft input hooking remains disabled.

## Purpose

R124 established editable preview state for Piercing's eight source-identified controls, but edits required F9/F10/F11. R125 wires actual left-button clicks to the **drawn dropdown and switch widgets** in the separate first-party GL1.1 preview. It does not execute Piercing, mutate gameplay state, hook original LWJGL input or add automation.

## Hit geometry and dispatch

- The source-backed Piercing card is catalog index 11. On a fully visible, expanded Combat card, the hit tester reads the **five currently visible** immutable settings pointers and uses exactly the same horizontal/vertical control geometry as `draw_recovered_widget`.
- Dropdown index **0** is clickable only inside its rendered control (full control width × 22 logical px). Boolean indices **1–7** accept clicks only within the respective right-aligned **34 × 18** logical-pixel switch. Settings labels, section headings, dividers, unused space, other modules' reference settings and clipped cards remain inert.
- When page 2 crosses from *Always hit through* to *Conditions*, the hit tester includes the same extra **20 logical pixels** as the GL renderer. The configured menu-panel transform applies to standalone, bounded overlay and compact overlay previews.
- Only after **mouse up on the same control** and verified foreground/window owner does `MenuMouseGesture` produce a committed hit. It now includes the **control index** when comparing press/drag/release identity, so dragging between switches cannot change an unrelated switch.
- The Win32 preview routes such a committed hit through `MenuNavigation` focus admission and `MenuState::select_piercing_control`, which refuses stale pages, invalid indices, wrong tab, collapsed card and off-screen catalog entry. It then invokes the existing isolated `ActivatePiercingControl` action. No other gameplay path is touched.
- The existing double-click suppression, focus-loss, capture-loss, stale-frame and wrong-thread protections continue to apply. The frame's copied Piercing values and selected index already invalidate gestures when the state changes mid-press.
- F9/F10/F11 keyboard editing and F12/Shift+F12 isolated sandbox persistence remain unchanged.

## Test evidence

Extended `native_piercing_preview_persistence` runs the same real hit-test and `MenuMouseGesture` model used by the standalone preview in three presentation modes. It checks:

1. Actual dropdown and switch control rectangles, including rejection of adjacent text labels and section headings;
2. A press dragged off a widget, a cross-switch release, and foreground-denied presses;
3. A fully committed dropdown or boolean click changes **only** the associated local sandbox setting;
4. A changed frame cancels in-flight input without mutating any other control;
5. Page-2 group-heading offset, control index 7, off-panel coordinates, forged page offsets, wrong tab and stale selection refusal.

**Limitations:** Mouse-controlled editing currently applies to Piercing's eight preview controls only. Native numeric ranges/defaults of other modules, official profile serialization, popup animations, slider dragging and fully integrated in-game mouse ownership are still not verified. The candidate decrypted archive is not an independently authenticated vendor release.

## Release controls

This change does not merge the draft PR, enable native combat behavior, install the original Minecraft input hook or modify the user's game files. GitHub CI must pass for the exact R125 commit before declaring verification complete.
