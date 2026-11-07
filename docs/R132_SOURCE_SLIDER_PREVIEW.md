# R132 — Safe source-indexed slider control in native ClickGUI preview

**Status:** committed to draft PR #1 (not merged); build and CTest results on this head are not yet verified. The connected Windows desktop was offline during this work.

## Reference fidelity

The user's candidate `decrypted files.zip` yields a typed GUI fixture in `src/recovered_settings.cpp`: **115** single-value slider records and **4** separate two-handle ranges. These fixtures preserve recovered labels, order and candidate numeric bounds/step/examples. As previously established, numeric defaults/ranges in this typed fixture are **not authenticated native values**; they must not be copied into gameplay configuration as exact originals.

R132 addresses the **115 single-value sliders** and explicitly leaves range controls read-only until a separate endpoint-ownership implementation is safe.

## Change

- New `MenuNumericTrack` uses the renderer's exact right-hand numeric track placement and section-heading offsets. It rejects hidden/clipped pages and wrong tab/module/widget identities.
- `hit_test_menu()` admits only the 18px-high bar/knob corridor of an actual visible `Slider` (not its label, heading, range field, or other numeric widget).
- The existing `MenuMouseGesture` owns mouse press/drag/release, validates foreground, same target, time, owner thread and immutable render frame. Dragging within the track commits only on a verified **mouse-up**; moving outside or losing foreground cancels without mutation.
- Source-indexed `MenuState::set_recovered_slider_preview()` rejects nonfinite, out-of-range, wrong-tab, wrong-module, hidden-page, modal-owned and other invalid edits; it rounds to the recovered positive step using bounded double arithmetic.
- The renderer snapshots up to 385 optional numeric values and displays the selected value by adjusting an **ephemeral widget copy**. No `ClientSettings`, profile, original Minecraft input or native DLL state is modified.

## Regression

New CTest: `native_recovered_slider_preview_safety` in `tests/test_recovered_slider_preview.cpp`.

It covers all three panel modes, 115/4 inventory classification, exact source callsite and pointer geometry, click/drag/release, wrong-module/out-of-page refusal, NaN/Inf/range denial, step quantization (Auto Clicker Target CPS), snapshot invalidation, modal block, unchanged module activation, and out-of-track coordinate clamping in the pure conversion helper.

## Remaining work

- Two-handle range widgets, with immutable handle identity and no crossover on drag.
- Keyboard-accessible numeric editing, editable values/labels, exact original mode-dependent ranges and proper persistent configuration.
- Source-fidelity validation (Inter fonts, animations, popups, profiles, source-callsites) and safe full Minecraft 1.8.9 input ownership.
- 67 gameplay modules are still **not** reconstructed/verified by this GUI-only progress.

Full Windows Release, CTest, OpenGL captures and actual vanilla 1.8.9 acceptance are **not yet demonstrated** for this commit.
