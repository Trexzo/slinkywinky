# R131 — Generic source-list dropdown previews

**Repository:** Trexzo/slinkywinky, draft PR #1. Not merged.  
**Runtime boundary:** First-party Windows GL11 menu preview only; no original Minecraft input hook and no gameplay module writes.

## Scope and source

The current `src/recovered_settings.cpp` typed fixture (derived from the user's candidate decrypted archive, *not* authenticated original Slinky source) has **32 Dropdown records**, including three specialized controls already handled in R127: Aim / Mode, Target / Mode and Piercing / Sort targets by. R131 makes the remaining **29 recovered dropdown records** addressable through a generic source-identity path.

One recorded target-mode options string lists four entries while the dedicated preview mode currently uses only Single and Switch. That difference remains a **known provenance gap**, not something to silently reconcile.

## Implementation

- Dropdowns are keyed by the exact immutable widget-catalog pointer index, not by label or module name alone. This protects repeated labels, conditional Aim branches and independently numbered module pages.
- The source options string is split only at `|`, rejecting empty/malformed entries, out-of-range indices and more than twelve options. Current fixture dropdowns have 2–10 choices.
- `MenuState` keeps optional selected-choice overrides in a separate 385-slot array, with the fixture's estimated example index as the initial displayed value. No gameplay-setting, profile or packet state is changed.
- A dropdown can open only on its current expanded module card in the correct tab and current five-item page. The three dedicated controls cannot be shadowed by generic editing.
- The shared popup renderer/hit-test displays the recovered labels and selected accent using the R127 28px popup rows. A popup owns input until option selection, Escape, loss of focus or outside click dismissal.
- A snapshot carries all choice overrides and the popup identity. Any open/close/choice edit invalidates in-flight mouse gestures on stale frames. Modal Unload input remains separate.

## Regression and acceptance

The new `native_recovered_choice_preview_safety` CTest exercises direct Auto Clicker `Click pattern` dropdown hit-testing and selection across standalone, bounded and compact modes; exact source option labels; invalid option denial; same-target click completion; no click-through; local profile non-mutation; stale frame release denial; and cross-module refusal.

**Status:** R131 code and regression are pushed to the existing draft GitHub PR. Full Windows CTest, actual GL captures, GitHub Actions and vanilla Minecraft 1.8.9 acceptance have not yet passed/been observed on this head. The connected Windows desktop was offline.

**Next GUI gates:** recover exact source numeric slider/range values and make drag gestures safe, preserve multiselect/color/text semantics, complete persistent profiles and bind editing, certify font/animations/visual fidelity, and prove actual in-game keyboard + mouse ownership/restoration. Only then can the 67 gameplay modules be individually reconstructed and verified.
