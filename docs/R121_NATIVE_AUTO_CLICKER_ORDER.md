# R121 — Native Auto Clicker callback order and section boundaries

This change uses the user's candidate decrypted archive's static native GUI metadata (not unverified JavaScript defaults), as audited in R120.

## Native Auto Clicker correction

The recovered `src/gen/mod_auto_clicker.cpp` W_* callback presents **nine** controls, in this order:

| Section | Native sequence |
|---|---|
| In-game | Click pattern (Jitter / Butterfly); Target CPS; Randomize; Simulate exhaust; Allow breaking blocks |
| Inventory | Click in inventory; Randomize speed |
| Conditions | Holding weapon; Not using item |

The R116 JavaScript-derived fixture had the first two controls in the wrong order and reversed the section membership of Randomize and Randomize speed. R121 aligns the first-party 378-entry read-only preview catalog with the verified native nine-control order and section names. Numeric ranges, default values and booleans shown remain **unverified illustrative values** from the earlier reconstruction.

## Section-aware paging and rendering

Previously, five-control pages displayed only the first recovered section heading; when a page crossed a group boundary, the second group heading disappeared. R121 draws a new heading at each group transition inside the page, increases the card's actual logical height by 20 pixels per additional heading, and shifts the footer to match. Both the native renderer and pointer/wheel hit-testers use `menu_expanded_height` and `menu_module_card_rect`, so the display height and clickable regions share a single geometry source.

The R121 native regression checks the complete nine-control order, types and section assignments; first/second page pointers; and Auto Clicker second-page geometry. The second page displays four controls across Inventory and Conditions with an extra 20px section heading. A pointer into the lower expanded pane stays non-actionable while bounded wheel paging targets the correct Auto Clicker catalog index.

## Remaining fidelity gaps

This is a focused original-source-backed correction, not completion of the ClickGUI. The other 58 registered module callbacks have not yet replaced the estimated R116 ordering; the archive's 424 native registration calls do **not** equal 424 simultaneously visible runtime settings. Conditional branches, original typography/animations, working profile persistence, physical GUI input in Minecraft and module execution remain unverified. The user-supplied decrypted archive is an unauthenticated candidate. Java 8 Minecraft input interception remains disabled under R119's source-pinned guard.

The next safe GUI batch should verify native Aim Assist (including duplicate conditional speed controls and a separate Multi-point group), Piercing (native six-option section rather than one guessed multiselect), and advanced GUI popup/modal behavior, with tests before enabling editing.

## R121 follow-up: three additional matching-size native callback modules

The same static callback-versus-fixture review found that **Auto Block (12 controls), Backtrack (12), and Bridge Assist (10)** have matching overall control counts and kinds but incorrect ordering and/or heading assignments in the reconstructed JavaScript fixture. The first-party catalog now presents all three in the exact recovered W_* callback order, without adding or dropping controls. The native callback places Auto Block's “Only within range” under **Force block animation**; Backtrack's “Maximum hurt time” belongs in **Main**, and “Apply head rotation” belongs in **Real position indicator**; Bridge Assist presents “Unsneak delay” before its “Select blocks” dropdown. Dedicated tests assert all 34 labels and groups.

**Provenance exception:** The Backtrack color registration exposes a color control without a recovered display label. The existing **“Color”** caption remains a first-party presentation fallback, **not** an attested native label. Numeric bounds and displayed boolean values remain unverified examples, with module actions still preview-only.
