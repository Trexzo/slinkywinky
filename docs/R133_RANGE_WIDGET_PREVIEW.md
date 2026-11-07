# R133 — Two-handle range widget reconstruction in the native ClickGUI preview

**Branch:** `reconstruction/r1-core-fidelity`, draft PR #1 (unmerged).  
**Safety boundary:** Separate Windows GL1.1 preview only. No original Minecraft keyboard/mouse interception, JNI game mutation, packets or cheat-module execution.

## Decrypted source basis

The user's candidate `decrypted files.zip` contains `re/clickgui.tpl.html`. Its *reconstructed* slider/range template uses a right-half track, nearest-handle selection on pointerdown, a snapped step, and a constraint that the active endpoint cannot cross its partner. In the typed recovered catalog, there are exactly **4** Range widgets (Aim Assist Range/FOV, Backtrack Target distance, Pointers Transition between). Initial numeric defaults, min/max and steps are **source-fixture guesses**, not fully authenticated original registration values.

R133 follows this template's nearest-handle and noncrossing semantics while deliberately keeping all effects within first-party menu preview state.

## Implementation

- `RecoveredRangePreview { lower, upper }` is keyed by immutable widget-catalog identity and stored in an optional 385-entry preview array. It never alters the original widget fixture.
- `MenuNumericTrack` now supports both `Slider` and `Range` in one renderer-matched, bounded geometry path; `menu_numeric_value_at_x` clamps normalized pointer position to the source fixture's min/max.
- The hit tester returns the exact source-global widget index and nearest of the two handles (ties favor lower, as in the supplied template).
- `MenuMouseGesture` captures the handle identity on pointer-down. Moving across the other knob stays in the *same source track* without switching ownership; leaving the track, losing focus or observing a changed snapshot still invalidates the drag. A valid mouse-up commits only the captured handle.
- `MenuState` validates visible five-item page, selected category, expanded module, finite numeric range/step, handle identity 0/1, and modal/popup exclusion; it snaps the requested value to the source step, then limits the active endpoint to its partner's position.
- The GL11 range fill/knobs use a transient copy of the source widget with the new lower and upper preview values. The data is not written to `ClientSettings`, any gameplay profile, or a live Minecraft client.
- Like R132, the first-pass interaction **commits on release**, not continuously each pointer-move; reference-fidelity animation and live tooltip updates remain separate work.

## Regression

New `native_recovered_range_preview_safety` CTest exercises the three panel modes, count of four reference range controls, visible-page search for Aim Assist Range, nearest-handle identity, dragging lower across upper, preventing crossover, reopening upper endpoint, dragging upper across lower, stale gesture invalidation on a state update, NaN/out-of-bounds/invalid-handle denial, refusal during Unload modal, and refusal in the wrong tab.

## Verification boundary

Code and tests are committed to the draft branch. No Windows Release/CTest, OpenGL image comparison, or vanilla 1.8.9 original-client run has been confirmed for this exact head because the connected Windows machine is offline. Do not infer game parity, native calling semantics, or completion of the 67 gameplay modules from these tests.

Next GUI work: native-style numeric tooltips/live drag, multiselects, color pickers, text fields and buttons, profile persistence, source typography, animation, safe in-game input ownership, and full reference audit.
