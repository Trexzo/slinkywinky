# R134 — Recovered multiselects with source option lists and bounded popups

**Repository:** Trexzo/slinkywinky · `reconstruction/r1-core-fidelity` · draft PR #1, unmerged.

## Evidence / authenticity boundary

The user-owned candidate archive `decrypted files.zip` contains `re/clickgui.tpl.html`, a reconstructed HTML transcription of native GUI behavior. Its `it.k==='m'` branch keeps a multiselect list **open** after an individual option is toggled, unlike dropdowns, which close after choosing a single value. The template uses **28px option rows** and shows selected entries using the accent color. The current typed fixture `src/recovered_settings.cpp` records exactly **7** `MultiSelect` widgets, including Teams/Search color (16 choices) and Weapons/Any item in (9 choices).

The candidate source archive is **not authenticated as an official commercial Slinky build**. Multiselect initial selections are not available as trusted bitsets in our fixture, so R134 intentionally defaults its *local preview selection* to **none**. It does not claim to reproduce original default selections.

## Implemented

- `MenuState::open_recovered_multi_preview()` accepts only a visible, expanded, correct-tab source catalog widget of `MultiSelect` type. It rejects hidden/off-page, wrong-category and modal-owned requests.
- `recovered_multi_overrides_` is a 385-slot array of optional, per-widget 32-bit masks. Each bit tracks an option among the recovered 4–16-option strings. Toggling one option changes only that bit and leaves the popup **open**. No other module settings, game hooks, packets, profiles or input state are modified.
- The existing GL11 popup and hit-testing paths now support a bounded **six-row** viewport, with absolute source-option identity preserved across mouse-wheel/keyboard page scrolling. The full 16-choice Teams list and 10-choice Inv Manager dropdown can be reached without drawing outside the panel.
- This scrolling viewport and scrollbar are **first-party adaptations**, not byte-verified original Slinky behavior; the supplied reconstructed HTML template allows taller open lists and does not by itself establish the exact native overflow policy.
- Selected multiselect options are shown in the collapsed control as clipped comma-separated source labels and highlighted individually in the popup.
- Mouse press/release is still guarded by `MenuMouseGesture` on the owning thread. Closing, scrolling or changing a selection invalidates stale held-click snapshots. An open popup owns keyboard and wheel input; navigation/module hotkeys cannot pass through.

## Regression

New CTest target: **`native_recovered_multiselect_preview_safety`**.

The test covers:
- All seven multiselect records have bounded 4–16 option catalogs.
- Auto Tool's four choices across standalone, bounded and compact preview modes: popup opening, option toggling without closing, multi-bit selection, deselection, no click-through, wrong-module refusal, and unchanged gameplay bindings.
- Teams' 16-color popup in the 900×630 two-column standalone panel: six-row viewport, access to color option 15 via scrolling, stable selection, out-of-viewport selection refusal, stale mouse frame invalidation and modal exclusion.
- Regression for Inv Manager's ten-choice *single-selection* dropdown: the final choice is reachable and selecting it closes the popup.

**Known narrow-mode limitation:** In a 900×630 window, the compact/bounded 1-column overlay may clip the seventh displayed Utility card. Those layouts therefore do **not** yet demonstrate opening the Teams popup at the end of the Utility category. The next GUI task should correct per-panel module scrolling/visible-card admission, not count this as full parity.

## Validation state

Source files, CMake test registrations and docs are committed to the draft branch. The candidate-archive pattern itself was independently inspected in this turn: multiselect toggling keeps the list open and the template uses 28px rows. **Windows CTest, CI, actual OpenGL captures and vanilla Minecraft 1.8.9 runtime behavior have not been verified for the new head** because the connected Windows desktop was offline.

Remaining GUI work includes compact-panel accessibility, 13 colors, six text fields, two action buttons, profile save/load and binding, exact typography/animation/hover, native input ownership and restoration. All 67 gameplay modules still require separate functional reconstruction and acceptance.
