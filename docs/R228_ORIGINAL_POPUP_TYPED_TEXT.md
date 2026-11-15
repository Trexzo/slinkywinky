# R228 — Original Select item text entry and empty-result fidelity

Date: 2026-11-15. Chat 1 / draft PR #1, preserving Chat 2's PR #2.

## Pinned source evidence

Skid-Technology/OpenSlinky@fd9c592:
- src/engine/gui/gui_helpers.cpp: text_filter_alnum allows ASCII letters,
  digits, and spaces; it appends at buffer length <=99, for a maximum
  of 100 characters, preserving the original typed case.
- src/engine/gui/gui_layout.cpp: handle_gui_input reads up to four
  independently supplied typed characters from an input event and
  processes Backspace separately from printable text.
- src/engine/gui/gui_popups.cpp: draw_tracked_text paints the edit
  buffer and caret, including while empty. The list popup draws no
  result rows when the filtered match count is zero.

## First-party implementation

- The bounded native search filter and model now accept 100 source-valid
  ASCII bytes (previously 32).
- The model has an ABI-neutral append_search_preview_popup_text API that
  processes at most four supplied characters per event. It ignores
  unsupported punctuation and preserves case for display. Existing
  fallback LWJGL2 keycode-only fixture events still work.
- The retained WGL fixture exposes a new opt-in typed event overload,
  using the exact same WGL owner, viewport, foreground, paired key
  ledger, and MenuKeyboardRouter gates. A provided typed payload
  overrides keycode guesses and cannot mutate a non-owned session.
- Native GL11 no longer substitutes an invented Search... placeholder:
  it paints the current buffer (even empty) plus a visible caret.
  For long queries, it measures and renders a trailing substring
  that fits in the search field without modifying the original text.
- A no-match search yields zero 25px result cells and a shorter modal,
  preserving the title close X, text field, and the source admission
  fences. No hidden option can be selected.

## Regression coverage

- Portable source tests: 100-char boundary, batch length restriction,
  mixed case, invalid punctuation, deletion and cancel, zero matches,
  absence of ghost hit targets and rejected option commits.
- Original retained WGL fixture: explicit mixed-case typed payload,
  invalid input that must NOT fall back to keycode guessing, no-match
  and restored matches after Backspace, GL11 dialog and selection,
  no background navigation, input quarantine and unchanged profile.

This is first-party isolated GUI reconstruction, not real injected
Minecraft keyboard/OS input, source-perfect GUI animation, or the
completion of original Slinky module implementations. No Minecraft
process is launched by this change.

## Verification

Record precise CI results only after the fresh commit finishes all
Windows Debug/Release, Ubuntu Debug/Release, sanitizer, JNI and
Java 8 Attach tests.