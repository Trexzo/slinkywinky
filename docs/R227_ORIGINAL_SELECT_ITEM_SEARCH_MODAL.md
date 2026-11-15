# R227 — Native original Select item searchable popup

Date: 2026-11-15. Chat 1 / PR #1 draft. No Minecraft launched.

## Source evidence

Pinned Skid-Technology/OpenSlinky@fd9c592:
- src/engine/gui/gui_controls.cpp: n>=6 optional dropdown invokes
  open_list_popup with "Select item" and items+1, n-1. The X
  resets selection to index zero.
- src/engine/gui/gui_popups.cpp: popup_kind=4 uses a 30px text
  field, case-insensitive substring matching, two 25px columns,
  and a close X within the title bar.

## First-party change

- Inv Manager / Sort hotbar / Slot 1 renders a centered native
  GL11 Select item dialog instead of the interim scrolling popup.
- Pure source labels with a bounded ASCII/space search field,
  matching case-insensitively and retaining absolute source IDs.
  Source None remains excluded from result items.
- Shared geometry renders and hit-tests the source close X,
  search field and two-column results. Background clicks fail closed.
- The query lives in a mutex-protected ABI-neutral MenuState
  sidecar and clears when the popup closes or is committed.
- Paired synthetic LWJGL keyboard events edit the query or cancel
  with Escape. The navigation router blocks background Tab,
  arrows, Enter and Escape for any open dropdown.
- A query change invalidates a stale held press. Selection is
  accepted only if the source ID is present in the filtered list.
- ClientSettings, injected-game input, packet handlers, PR #2
  and native MenuState binary layout remain untouched.

## Source differences and limits

The original list search rebuilds at most every 0.25 seconds and
caps displayed matches to 30. This one identified ten-item
list recomputes at each snapshot and uses the same max 30.
Keyboard-driven result selection, real OS key ownership, physical
mouse interaction inside original Minecraft, actual injected
runtime testing and gameplay module behavior are not verified.

## Verification plan

Portable choice and multiselect tests now cover modal filtering,
None exclusion, Esc cancel, backspace, selected IDs and stale
presses across panel modes. A real WGL test exercises GL11 draw,
paired synthetic keyboard input, source hit geometry, result
selection and GL state restoration. Windows JNI/Attach, Ubuntu,
sanitizers and Debug/Release CI results will be recorded after
actual completion.