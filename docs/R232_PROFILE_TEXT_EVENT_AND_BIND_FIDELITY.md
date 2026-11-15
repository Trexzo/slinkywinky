# R232 — Source-typed profile names and extended bind fidelity

Date: 2026-11-15. Chat 1 / draft PR #1 only.

## Pinned source
Skid-Technology/OpenSlinky@fd9c592,
src/engine/gui/gui_layout.cpp::handle_gui_input:
- Text editing consumes only bytes carried by input event ch[4],
  terminated by the first NUL, and handles Backspace, Enter and Escape
  separately. It does not derive text from keycodes.
- Keybind capture is a separate input_mode. It forwards the
  independently supplied key, with Escape clearing the value.

## R232 first-party changes
- Removes the name-field's guessed printable character fallback
  from LWJGL2 keycodes. A keydown with no typed characters cannot
  invent a letter, number or space (including Ctrl+A or Alt+letter).
  Exactly 1–4 event bytes are considered; at most the prefix before
  the first NUL is admitted. Explicit payloads over four bytes are
  rejected. Each byte is still checked by MenuState's strict
  28-byte, ASCII-only, no-slashes source-local name policy.
- Extends verified synthetic LWJGL2-to-preview VK capture mappings
  to punctuation (minus, equals, brackets, semicolon, quote,
  grave, backslash, comma, period, slash), plus separate left/right
  shift, control and alt. Escape still clears a bind and never
  requests the game's GUI close. Unknown keys leave capture pending.
- Removes an unrelated 8-glyph display length rejection that made
  source-valid BACKSPACE and BACKSLASH unbindable. The existing
  preview whitelist continues to reject invalid codes.
- Reclaims enough space in the shared profile chip hit/render
  geometry to display full valid key names and the 14-character
  PRESS A KEY... capture state. Bounds and minimum interactive
  sizes remain enforced; the profile title is clipped before the
  chip instead of painting across the control.

## Offline regressions
- Native retained Windows GL11 test: no phantom text from keycode
  alone, invalid punctuation rejected, oversized typed payload
  rejected, first NUL stops an otherwise valid input, Backspace
  restores the draft, text editing stays menu-owned.
- Retained WGL capture: unknown scan code preserves pending capture;
  Backspace binds as nine-glyph BACKSPACE with a wide clickable chip;
  semicolon binds as COLON, right shift as RSHIFT, Escape clears.
  Repeated clicks are independently timestamped beyond the
  existing double-click safety window.
- Portable Profiles test: accepts BACKSPACE while preserving the
  independent bind capture, checks full-width normal/capture
  chip geometry on all three panel modes and subsequent F6 binding.

## Scope
No Minecraft process, physical input hook or real profile filesystem
action. These are original-reference-informed improvements to
first-party offline/native WGL preview only. Chat 2 PR #2 untouched.

## CI
Pending exact-commit Windows/Ubuntu/sanitizer/JNI/Java8 Attach checks.
