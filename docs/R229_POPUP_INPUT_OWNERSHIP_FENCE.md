# R229 — Popup input-ownership fence across native GUI layers

Date: 2026-11-15. Chat 1, draft PR #1 only.

## Defect verified in the R228 source

The first-party MenuKeyboardRouter already treated an open popup as
exclusive. However, lower-level MenuState::select/scroll_by/
scroll_by_visible and MenuNavigation::select_tab/focus_visible_row
still accepted some background commands for non-color dropdowns.
Several of those silently reset dropdown_preview_ without clearing
the original search sidecar. Retained WGL apply/scroll also refused
color dialogs but could invalidate the painted hit frame before a
recovered-choice popup navigation failed.

## Change

- Reject tab changes, eight-row and visible-limit scrolling, module
  expansion, settings-page changes, enum switches, and general preview
  toggles whenever *any* dropdown owns the GUI.
- Preserve the active popup (including the typed search query and
  exact source module/page) after a refused operation. Do not
  silently dismiss, mutate a profile or switch focus.
- Guard specialized piercing controls and the inert unload modal
  from replacing an open dropdown.
- MenuNavigation's direct select_tab and focus_visible_row now use
  the same general popup fence as apply, rather than color-only.
- Retained WGL apply/scroll check before discarding last_drawn_frame_
  or cancelling the pointer gesture. A denied background operation
  therefore cannot destroy the actually painted popup hit surface.
- Runtime-toggle queueing requires no dropdown modal, even though
  the request itself is still the separate offline/admitted model.
- A profile keyboard-adjustable primary parameter cannot change
  through a direct bridge call while a popup is displayed.

## Regression evidence added

- Searchable Inv Manager Slot 1: all direct model, profile-bridge,
  navigation, scroll, settings-page and unload attempts are refused;
  nonempty query, source identity, settings offset, selected choice,
  tab, scroll and focus remain unchanged; Backspace still edits
  after rejection.
- Independently specialized Aim dropdown: same modal fence across
  navigation/model/profile, including a previously valid primary
  parameter edit; background NextTab becomes usable once closed.
- Native retained GL11/WGL test: reject modal-background apply,
  scroll and focus, verify the exact previous painted frame pointer
  remains available, then continue through the original key-paired
  filtered search, select a result and restore navigation on close.

## Scope and limits

Only local GUI/source-model input ownership was changed. No Minecraft
runtime was started, no real keyboard/mouse hooks were installed,
no module was activated, and Chat 2's PR #2 remains untouched.

Exact-commit CI status must be appended after workflows complete.