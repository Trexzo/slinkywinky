# R197 — native retained ClickGUI text focus and preview bind-chip dispatch

**Branch:** `reconstruction/r1-core-fidelity` in [Trexzo/slinkywinky draft PR #1](https://github.com/Trexzo/slinkywinky/pull/1). PR #2 (independent OpenSlinky comparison) must remain untouched.

## Gap identified in R196

The reconstructed source-indexed `MenuHitKind::RecoveredTextField`, `BindChip`, and `BindHold` were already rendered and correctly hit-tested by the first-party GUI. The first-party `MenuState` already had safe, local-only implementations for `focus_recovered_text_preview`, `begin_bind_capture`, and `toggle_bind_hold` (R137/R140). However the retained native WGL `WglRetainedMenuPreview::pointer()` switch had **no cases** for these hit kinds. A genuine test-fixture mouse click therefore fell through to default refusal and never focused or armed the source preview control.

R197 wires the three cases into these **existing** source-bounded `MenuState` actions, after `navigation_.focus_visible_row(hit->catalog_index)`. These methods enforce current tab, module visibility, owner, popup/modal exclusion, source-widget identity and, for HOLD, that a module already has a valid preview bind. The preview values are local UI state and **never register OS or Minecraft keybinds**, edit client gameplay settings or emit input events.

## Changes and test coverage added

- `src/windows_retained_menu_preview.cpp`: three missing native pointer-dispatch cases: **BindChip** starts local “PRESS A KEY” capture, **BindHold** toggles only a valid local preview HOLD flag, **RecoveredTextField** focuses the exact visible text widget.
- `tests/test_windows_retained_pointer.cpp`: a new disposable WGL regression drives real retained GL11 frames through R190–R193's **synthetic** mouse ledger and tests a click on the rendered Aim Assist bind-chip, and a click on Auto Tool's source-indexed Search item text field. The test checks the post-repaint `bind_capture_index` / `focused_text_widget` values, invalidation of old painted geometry and unchanged native profile/enable flags. It deliberately does not claim that genuine keyboard text input or physical binding capture has been implemented.
- `tests/test_r119_live_input_guard.py`: source-only contract checks ensure all three native dispatch cases remain connected to first-party preview-only code and the native WGL regression is present.

The retained GUI class layout is unchanged. There are no new JNI/Java game agent entry points, no event hooks and no original LWJGL keyboard or mouse suppression. The existing `R119/R160` source guards continue prohibiting synthetic fixture APIs in the live Minecraft agent.

## Verification status — not run yet

As of this R197 GitHub-only change, the linked Windows Desktop Commander device `DESKTOP-LTXXO4K` was **offline**. The C++ Windows Release build, 95-test CTest suite, Java/JNI agent-attach tests, native OpenGL/WGL interaction test and isolated vanilla Minecraft 1.8.9 acceptance have **not yet been rerun for R197**. The last complete 95/95 PASS and 64-frame original-client read-only PASS apply to **R196 commit ff018208**, **not** to these R197 commits.

The R197 test code was grounded in the existing source-hit geometry and locally validated preview methods, but is **pending real compilation and execution**. Do not merge or enable live input until the Windows suite, foreground input ownership gates, and source/reference fidelity requirements are satisfied.

## Next steps when the Windows PC is online

Build the original branch with the existing x64 Release CMake target, run `ctest --test-dir build-r184-clean -C Release -j 6 --output-on-failure`, run the standalone retained-pointer test directly for the `R197_WGL_NATIVE_BIND_CHIP_FOCUS_AND_TEXT_FIELD_PASS` receipt, and rerun isolated original-client read-only acceptance. Diagnose any failures before claiming R197 passed. Future work must separately add source-grounded text editing and bind commit/cancel handling, proper color-picker interaction, and visual comparison to original Slinky screenshots, rather than assuming text focus proves those features complete.
