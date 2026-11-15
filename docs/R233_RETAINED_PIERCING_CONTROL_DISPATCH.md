# R233 — Retained GL11 Piercing clicks and dialog input fence

Date: 2026-11-15. Chat 1: draft PR #1 only.

## Gap in first-party native ClickGUI

R125 implemented shared first-party hit geometry and portable mouse
gesture tests for the eight Piercing preview controls (index 11,
sort index 0; seven Boolean controls 1–7). The standalone Windows
preview also routed committed clicks to the existing local editor.
The retained WGL dispatcher, however, handled Aim/Target dropdowns,
recovered toggles and other source hits but omitted the
`MenuHitKind::PiercingControl` case entirely. Painted Piercing
settings therefore could be clicked in the retained overlay
without changing their local preview, despite using the same
source hit model.

The original candidate source's setting hierarchy, control rows,
five-control pages and seven switches are already transcribed into
recovered_widgets, and the R125 semantics were independently tested.

## Implementation

- The retained WGL pointer dispatcher now accepts a Piercing hit
  ONLY after exact painted source hit admission, paired same-target
  Down/Up, owner-thread/WGL/foreground checks and an in-range source
  index (11, control 0–7).
- Focuses the actual source module, selects its on-page control,
  and invokes the existing `ActivatePiercingControl` action. This
  mirrors the separate R125 Win32 preview and modifies only the
  `PiercingPreview` sandbox model, never gameplay/client state.
- Closes a separate modal bypass: Piercing selection, activation,
  keyboard move and preview-state import now all consult the same
  `modal_input_owned()` predicate as the rest of the native controls.
  A pending Unload/Profile/dropdown confirmation prevents background
  changes and preserves the selected source value.
- Source-labeled sort toggles between Hurt time and Health; seven
  source-visible Boolean switches toggle individually, including
  the last control on the second settings page.

## Regression tests

- Portable Piercing suite checks that an open Unload modal blocks
  direct select/move/activate/preview import, with unchanged sandbox
  state and restored selection after explicit close.
- Retained Windows GL11 fixture navigates to Piercing, expands its
  original source card, calculates verified painted hit coordinates
  through the shared settings layout, clicks the sort and first
  Boolean, changes to page 2, clicks control 7 and checks both
  selected control and isolated local values after each repaint.
- The same fixture checks owner-ledger paired release, profile
  snapshot nonmutation and complete OpenGL/renderer restoration.

## Explicit limitations

No Minecraft process, hardware input, original combat code,
resolver or gameplay is touched. This implementation is a
first-party reconstruction of the local GUI preview, not native
Piercing gameplay admission or a claim of pixel-perfect fidelity.
Chat 2's PR #2 stays independent and unmerged.

## Exact-commit CI verification

All three GitHub workflows passed against implementation commit
`9308733dcdf9a723aaf5071017483e65245f1dd0`:

- Offline C++ core: **6/6 jobs PASS**, covering Windows Debug/Release,
  Ubuntu Debug/Release, Linux sanitizers and Linux JNI smoke:
  https://github.com/Trexzo/slinkywinky/actions/runs/43766202773
- Windows JNI preview artifact: native x64/Java 8 roundtrip PASS:
  https://github.com/Trexzo/slinkywinky/actions/runs/43766202738
- Injectable Windows Java 8 agent: native loader/JVMTI and actual
  Java 8 Attach API test suite PASS:
  https://github.com/Trexzo/slinkywinky/actions/runs/43766202723

This confirms the isolated WGL click route and existing offline
source-typed UI model only, not original Minecraft runtime input,
physical hardware controls, or gameplay Piercing implementation.
