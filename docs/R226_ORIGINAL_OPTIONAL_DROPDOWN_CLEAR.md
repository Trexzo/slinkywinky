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

CI verification will be recorded only after the workflow completes.