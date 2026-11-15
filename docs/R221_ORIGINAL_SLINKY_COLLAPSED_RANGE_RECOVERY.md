# R221 — Original Slinky coincident-range handle recovery

Date: 2026-11-15. Chat 1, draft PR #1, original-native GUI fidelity.

## Source-proven behavior

The pinned OpenSlinky reference (Skid-Technology/OpenSlinky@fd9c592,
src/engine/gui/gui_controls.cpp, draw_range_slider_row) uses separate
left- and right-of-midpoint input areas for the two range handles.

Our R220 implementation correctly pushes the opposite endpoint whenever
one handle crosses the other. That necessarily permits both endpoints
to become *equal*. The previous menu_hit_test.cpp instead compared
distances to the lower and upper values and chose lower on a tie.
When the values are equal, those distances tie at **every** track position.
Consequently, no mouse hit could ever choose the upper handle again;
the user could not reopen a collapsed range toward the right.

## Minimal first-party correction

- Select handle 0 for pointer positions at or left of the two-handle
  midpoint, and handle 1 for positions right of it. Unlike distance ties,
  this still assigns distinct input areas when both endpoints coincide.
- Preserve R220 endpoint push, source-indexed widget ownership, the
  original DOWN-selected handle through the source-pinned gesture,
  frame and owner checks, step/bounds, and unrelated module/profile state.
- Add genuine first-party press/move/release checks for a collapsed
  interior range reopening toward the right *and* toward the left in
  StandalonePreview, BoundedOverlay, and CompactOverlay.

## Verification

A clean temporary clone of PR #1 at R220 was configured and built with
Windows x64 CMake Release, then its **63/63 locally configured CTests
passed**. After the R221 two-file correction, the full locally configured
C++/Windows suite passed again: **63/63**, including
native_recovered_range_preview_safety and retained GL11 source mouse.
Build exit 0, CTest exit 0.

This local CMake configuration did not enable every opt-in Java 8/Forge
integration test from previous R218 evidence; **the historical 107-suite
is not claimed green for R221**. No original Minecraft process or
physical mouse was exercised. This remains an isolated GUI-reconstruction
improvement, not proof of original injectable-client fidelity, gameplay
module parity, or in-world activation.

No source files in Chat 2's separate PR #2 were edited. Both PRs remain
draft/unmerged.
