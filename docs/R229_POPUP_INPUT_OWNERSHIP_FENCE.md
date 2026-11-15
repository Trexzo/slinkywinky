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

## CI verification

All three GitHub workflows completed with success at exact R229
implementation commit `116fe9b4fab0fdbb8be577231f77b05631222dc7`:

- Offline C++ core — **6/6** successful jobs, covering Windows and
  Ubuntu Debug/Release, Linux sanitizers, and Linux JNI smoke:
  https://github.com/Trexzo/slinkywinky/actions/runs/43748488986
- Windows JNI preview artifact — Windows x64 native JNI and
  Java 8 roundtrip tests passed:
  https://github.com/Trexzo/slinkywinky/actions/runs/43748488997
- Injectable Windows Java 8 agent — Windows native/JVMTI loader,
  Java 8, and actual Attach API tests passed:
  https://github.com/Trexzo/slinkywinky/actions/runs/43748489061

The initial R229 CI at `76484d8` identified two inherited assertions
expecting background tab selection and scrolling to dismiss an open
dropdown (R127 and R135). Commit `116fe9b` updated those regressions
to assert the source popup remains alive after denied navigation
and explicit close restores normal scrolling/tab selection.

This verifies the first-party offline model and disposable WGL
window. Original Minecraft hardware input and in-game module
integration were not exercised. PR #2 remains independent.