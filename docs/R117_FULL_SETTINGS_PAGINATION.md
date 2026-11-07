# R117 — Bounded full settings pagination from decrypted GUI

**Target:** Trexzo/slinkywinky, `reconstruction/r1-core-fidelity`, draft PR #1.

## Purpose and provenance

R116 imported all **378 presentation-only setting widgets** from the user's
candidate decrypted `decrypted files.zip`. R116 initially showed only the
first five controls in each expanded module. R117 makes **every** recovered
control reachable, without representing any of the fixture's estimated
defaults or slider ranges as native-verified gameplay state.

The catalog remains **67 modules**, with its existing seven-tab navigation
and 900px/1500px recovered grid. No vendor DLL, loader, fonts or JavaScript
are redistributed. Source data is read-only static C++ metadata.

## Implementation

- `MenuState` now owns a bounded **per-catalog-index settings offset**.
  Offsets advance or retreat by five records. Invalid direction, closed
  card, wrong tab, out-of-range index, first/last boundary and empty
  settings set are refused. Closing a card resets its offset to zero.
- `MenuNavigation` provides `NextSettingsPage` and
  `PreviousSettingsPage` as first-party, owner-thread-only actions.
  There is **no game OS key or physical mouse binding** for these actions
  in this release: they are driven by the native preview navigation model
  and the isolated WGL tests. The original Slinky client may use different
  scrolling interactions.
- `MenuItem.settings_offset` is copied into each immutable
  `MenuRenderFrame`. Old render frames cannot quietly change after
  navigation. `MenuMouseGesture` checks the offset while validating a
  press/release, and cancels a click if the page changed.
- `recovered_widget_page` scans the immutable 378-entry dataset and
  exposes at most five pointers on a five-aligned, in-range page.
  Malformed offsets return zero and clear all pointers.
- Shared `menu_visual_geometry.hpp` now uses the **remaining controls
  on the selected page** to size expanded cards. A shared frame validation
  rejects unaligned, off-end, non-collapsed and non-source-backed offsets
  before either drawing or native click hit testing.
- The native GL1.1 renderer displays the actual five-record page,
  the first control's recovered grouping, and an accurate footer such as
  `PAGE 2 / 5  CONTROLS 6 - 10 / 21`. No input is dispatched to
  recovered sliders, toggles, text fields, colors, buttons or multiselects.

## Test coverage

- Exhaustively walk all **378 widgets across all 67 module catalogs**,
  ensuring every record is visited once, that the final partial page
  terminates correctly and invalid offsets never expose stale pointers.
- Navigate multi-page **Aim Assist (21 records)** and an unsupported
  module (**Criticals, 7 records**) using the owner-bound navigation
  controller, refusing gameplay toggle of source-only modules.
- Verify previous-page/next-page boundaries, collapse reset, wrong-tab
  and unexpanded-state refusal, immutable render frame snapshots,
  invalid forged offsets, and cancellation of in-flight mouse clicks
  across page changes.
- Draw all three screenshot states using the actual first-party **900×630
  WGL OpenGL** context, asserting full GL state restoration, scissor and
  wrong-owner safeguards.

The screenshot evidence is read directly from our renderer's temporary
OpenGL framebuffer, not a generated mockup or an official Slinky screenshot:

- [Aim Assist — page 1 of 5](screenshots/r117-aim-first-settings.png)
- [Aim Assist — page 2 of 5](screenshots/r117-aim-second-settings.png)
- [Aim Assist — final page 5 of 5](screenshots/r117-aim-last-settings.png)

## Known limitations

The user-supplied decrypted candidate is not independently authenticated
as a signed official binary. Recovered widget labels/groups are reference
hints; numeric defaults/ranges in the archive's JS reconstruction remain
estimates. **Pagination is implemented in the first-party GUI model, not
wired to the injected original Minecraft keyboard, OS mouse wheel or
physical input**, and is not a claim of pixel-perfect native Slinky.
The modules' game behavior, server packets, profile file operations and
live settings mutations remain unverified. PR #1 stays draft/unmerged.

## Verification receipts — November 7, 2026

- Complete native Windows x64 Release build **PASS**.
- CTest **31/31 PASS**, including exhaustive catalog pagination,
  navigation ownership, mouse-gesture cancellation and isolated WGL
  state restoration.
- Python regression **55 run, one skipped, zero failures**.
- Original supervised vanilla 1.8.9 Java 8 JVM **PID 35576**:
  R94 scripted operator visibility/show/hide/stop **PASS** and exited.
  This test does **not** verify physical keyboard or mouse controls,
  nor source-widget functionality inside the actual game.
- GitHub CI status must be checked independently for the R117 commit.
