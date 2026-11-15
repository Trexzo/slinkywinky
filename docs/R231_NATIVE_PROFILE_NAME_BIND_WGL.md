# R231 — Native Profiles name, Create and binding WGL input

Date: 2026-11-15. Chat 1 / draft PR #1 only.
Source comparison: Skid-Technology/OpenSlinky@fd9c592,
src/engine/gui/gui_layout.cpp::handle_gui_input uses separate
text-edit (up to four typed bytes per event), Backspace/Enter/Escape
and keybind-capture input modes.

## Previously missing in first-party retained GL11 preview

ProfileNameField, ProfileCreate and ProfileBindChip already had
native GL11 render geometry and source-constrained pointer hit
descriptors, but the retained mouse release dispatcher did not
handle any of the three. The standalone local MenuState model
supported them only through direct test calls. Synthetic LWJGL
keyboard events still traveled through background navigation while
profile name editing or profile-bind capture was active.

## R231 implementation

- Adds exact typed-hit dispatch for ProfileNameField, ProfileCreate,
  ProfileBindChip in windows_retained_menu_preview.cpp. Each action
  uses existing in-memory MenuState preview validation; no profile
  filesystem paths are accessed.
- Extends the ABI-neutral modal input predicate to profile name focus
  and bind capture. Background tabs, scrolling, settings changes,
  profile adoption, unload and navigation are refused in these modes.
- Uses existing WGL/viewport/thread/foreground checks and paired
  synthetic keyboard ledger before any text or bind edit can occur.
  Text entry accepts source input-event characters, up to four
  characters per event, and relies on the existing 28-byte profile
  name, printable ASCII, slash exclusion, duplicate-name and six
  local-card constraints.
- Name-field Backspace erases, Escape blurs, Enter invokes
  in-memory profile creation. The painted Create button also
  creates via the same method. Explicit rejected characters cannot
  accidentally turn into keycode-based fallback text.
- Keybind capture maps an exact bounded set of synthetic LWJGL2
  key codes to the existing preview Win32 VK whitelist, including
  alphanumerics, F1–F12, arrows and Escape=clear. Unsupported keys
  leave capture pending. Tab during capture becomes a profile
  binding, never a background tab change.
- All successful typed-key edits invalidate the prior painted
  pointer frame and require a successful GL11 redraw. Key releases
  never produce a second mutation.

## Regression tests

- Portable Profiles tests reject background model/profile operations
  while profile name focus and bind capture own input.
- Keyboard-router tests verify name and bind modes exclusively
  quarantine Tab, Enter and Escape, without game delivery.
- Retained native Windows GL11 fixture uses actual painted field,
  Create and chip hit locations, paired synthetic key edges,
  mixed-case typed input, rejected punctuation, New profile
  creation, TAB and F6 binding, Escape clear, profile snapshot
  stability, and GL state restoration.

## Limitations

This is a first-party in-memory GUI preview. It does not write
or load real original profile files, install a live input hook,
launch Minecraft, or validate original-game hardware input. PR #2
remains independent.

## Exact-commit CI verification

All three GitHub workflows passed against corrected source commit
`cfe5c1a57d7d34ec213f1b559c48af75c7c4a509`:

- Offline C++ core: **6/6 jobs successful** (Windows Debug/Release,
  Ubuntu Debug/Release, Linux JNI smoke, ASan/UBSan):
  https://github.com/Trexzo/slinkywinky/actions/runs/43757591073
- Windows JNI preview artifact: Windows x64 native JNI and Java 8
  roundtrip tests successful:
  https://github.com/Trexzo/slinkywinky/actions/runs/43757591081
- Injectable Windows Java 8 agent: native loader/JVMTI build,
  JVM tests, and actual Attach API tests successful:
  https://github.com/Trexzo/slinkywinky/actions/runs/43757591042

Initial implementation commit `3ae8050` failed only the new
Windows retained-GL11 fixture because that test clicked the same
ProfileBindChip repeatedly within the existing R98 500ms
double-click guard. Commit `cfe5c1a` spaced the independent edits
farther apart without changing the underlying gesture safety
policy. The full matrix then passed.

These runs verify an isolated first-party native GUI fixture,
not original Minecraft or hardware keyboard/mouse acceptance.
No real profile files or other Chat 2 source were changed.