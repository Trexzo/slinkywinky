# R135 — late-category module access in narrow native GUI preview panels

**GitHub:** Trexzo/slinkywinky · branch `reconstruction/r1-core-fidelity` · draft PR #1, unmerged.

## Problem discovered during R134

At a 900×630 preview viewport, the 900px standalone panel renders a two-column grid, while the bounded and compact overlays render only one column. Previous module scrolling always stopped with **eight module cards remaining**, even though one-column panels cannot physically display eight full-height module cards—especially when a recovered settings card is expanded. Late-category controls such as **Teams / Search color (16 options)** could remain permanently clipped.

This is a native preview accessibility defect, not proof of an original Slinky layout. The user's candidate `decrypted files.zip` ClickGUI template specifies independent multi-column layout/padding and scrolling behavior, but does not authenticate the exact runtime clipping policy.

## R135 implementation

- `MenuState::scroll_by_visible(delta, visible_limit)` adds an explicit, validated 1–8-item viewport hint. It limits the requested category scroll to `total-visible_limit`, allowing the last few modules to be positioned high enough to expand safely. `MenuState::scroll_by(delta)` retains its previous eight-card semantics for existing callers and tests.
- `MenuProfileBridge::scroll_by_visible` retains synchronization of the visible profile flags; it does not change any gameplay settings, module enablement, profiles on disk or native Minecraft input.
- The standalone first-party Windows preview's **mouse wheel** selects a bounded visible hint based on the *actual panel geometry*: eight for multi-column; three, two or one for a one-column panel depending on logical height. This is a conservative preview layout choice, not a claim of byte-exact source logic.
- Existing render and hit-test clipping gates remain in place. The last category modules must still have their cards/popup fully visible before any input is admitted.
- The multiselect test now exercises the full Teams 16-color popup in all three modes by first scrolling late-category modules into a safe region.
- A new `native_narrow_menu_adaptive_scroll` CTest checks legacy backward compatibility, invalid visible limits, last-module exposure across all three modes, actual card containment, popup opening, popup invalidation after wheel scrolling and continued ability to reopen a visible control.

## Known boundaries

This improves **wheel access** to late-category modules, not complete keyboard-focus navigation in every narrow viewport or a general scrolling canvas with animated layout. Very small windows may still fail the full-card visibility requirement and correctly reject unsafe input. R132/R133 sliders/ranges and R134 multiselect interactions still need Windows CTest, real GL screenshots and original Minecraft 1.8.9 acceptance.

**Verification:** Code and new tests are committed; no completed Windows Release build, GitHub CI results or native game acceptance was available for this head because the user's connected Windows desktop was offline. Do not treat this as proof of complete GUI or gameplay-module fidelity.
