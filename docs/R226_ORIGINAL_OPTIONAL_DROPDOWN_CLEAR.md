# R226 — Original Slinky large optional dropdown clear and selection

Date: 2026-11-15. First-party ClickGUI, draft PR #1.

## Pinned original C++ evidence

Skid-Technology/OpenSlinky@fd9c592 src/engine/gui/gui_controls.cpp
draw_dropdown_row: for optional lists with >=6 choices, the
rightmost 20px X clears to source index zero. The main selection
popup receives items+1, n-1, excluding None. Our reconstruction
instead showed None as a regular popup row and had no X control.
Inv Manager / Sort hotbar / Slot 1 is the single recovered ten-item
dropdown matching that optional source shape.

## First-party repair

- Source-identify the exact ten-option Slot 1 callsite; do not change
  the other 31 recovered choice menus or multiselects.
- Draw the separate 20x20 clear X in native GL11 only when a
  nonzero choice is visible in the actual settings card.
- Share exact X geometry with source-indexed hit testing and route
  its own appended hit kind to a new local-only model setter.
- Validate owner tab, source identity, current visible widget page,
  original selection and active modal state before clearing.
- Start the large popup at index 1 and bound wheel scrolling from
  1, while keeping absolute source option IDs. Scrollbar geometry
  accounts for the excluded first row.
- The model test exercises three panel modes, clear X, wrong module
  refusal, absence of X after clearing, rejected index zero,
  scrolling to the final choice, and source-local selection.

## Limitations

The pinned original shows a searchable Select item dialog for the
large selector. This round repairs the clear and omitted-None
semantics, but retains the interim scrolling popup. It does not
claim the complete original search dialog, actual physical mouse
input, injected in-game use, gameplay module activation, or source
ownership of the original JNI pipeline. Chat 2 PR #2 is untouched.

## Exact-commit GitHub acceptance

The implemented R226 source and corrected inherited regression were
verified at commit `6ea598e42fbc9a5de6be758b7a973a9de5107d2d`.
All three GitHub workflows completed with success:

- Offline C++ core:
  https://github.com/Trexzo/slinkywinky/actions/runs/43742306853
  Six successful jobs, including Windows Debug/Release, Ubuntu
  Debug/Release, JNI smoke and sanitizer tests.
- Windows JNI preview artifact:
  https://github.com/Trexzo/slinkywinky/actions/runs/43742306838
  Windows x64 JNI build and test completed successfully.
- Injectable Windows Java 8 agent:
  https://github.com/Trexzo/slinkywinky/actions/runs/43742306782
  Windows native loader/JVMTI build and Attach tests passed.

The initial candidate CI failed on one wrong test coordinate helper;
the second run identified a legacy regression expecting ten popup
rows including None. Both were fixed before this green commit.
No real original Minecraft client was launched for R226, and no
physical mouse/keyboard, full original searchable dialog, or live
gameplay behavior is claimed.