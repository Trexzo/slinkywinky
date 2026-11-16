# R234 — Retained WGL module bind and recovered UTF-8 text input

Date: 2026-11-16. Chat 1; draft PR #1 only.

## Original reference / discovered gap

Pinned source: Skid-Technology/OpenSlinky@fd9c592,
`src/engine/gui/gui_layout.cpp::handle_gui_input`.
The original GUI distinguishes text-field editing (input modes 2/3)
and keybinding capture (mode 1). Text takes the bounded four-byte
`InEv.ch[4]` payload and handles Escape/Enter/Backspace separately;
bind capture takes a physical key code and Escape clears it.

In the first-party reconstruction, R197 restored the native
`BindChip` and `RecoveredTextField` mouse hit path and the local
models already supported edits. However the retained GL11 fixture's
keyboard input path did not complete either interaction; unowned
Tab/arrow keys still routed to background navigation, and Escape
could request closing the GUI.

## Native implementation

- Extend the ABI-preserving, read-only `modal_input_owned()` predicate
  to source module bind-capture and recovered text focus. The
  keyboard router, retained WGL apply and scroll admission, and
  direct profile/navigation checks now quarantine background actions
  while either editor is active. No MenuState layout change.
- When a verified source bind chip owns input, use the same bounded
  synthetic LWJGL2 to preview VK mapping as Profiles. Bind keys
  commit through existing `MenuState::commit_bind_capture()`, which
  edits only the local per-module preview key and clears capture.
  Unknown keys are inert. Escape clears the key, never closes the
  in-game GUI. Original Minecraft keybindings are not touched.
- When a recovered Text widget owns input, verify WGL context,
  viewport, thread, foreground, epoch and paired synthetic-key
  delivery before routing bytes. Parse at most four typed-event bytes
  up to NUL. Validate the complete sequence as 1–4-byte printable
  UTF-8 scalars before any write; malformed/truncated values are
  rejected without partial edits or keycode-based fallbacks.
  Existing 128-byte source text capacity and scalar-aware backspace
  semantics stay in `MenuState`.
- Backspace erases one UTF-8 scalar; Enter and Escape explicitly
  release text focus without triggering unrelated background menu
  actions. A textless keydown never manufactures ASCII.
- Key releases never repeat edits; successful events invalidate
  painted input and demand a real GL11 repaint before future clicks.

## Tests

- Portable recovered bind preview: background tab, settings page,
  scrolling and Unload are denied while capture is pending, with
  unchanged prior key. After explicit cancel, Unload is allowed.
- Portable recovered text preview: background page/tab, scrolling and
  Unload are denied while focused, and restored only after blur.
- Menu keyboard router: synthetic Tab, Enter and Escape are exclusively
  owned by both new input modes, with no gameplay event forwarding.
- Existing native Windows R197 GL11 bind fixture now commits F6
  through a verified keyboard down/up pair, checks exact preview
  code and restores background navigation afterward.
- Existing native Windows R197 recovered text fixture now uses
  the actual painted Auto Tool field: typed ASCII, UTF-8 ä and
  emoji, rejected invalid UTF-8, NUL termination, no phantom text
  from a bare scan code, UTF-8-aware Backspace, Enter blur, GL11
  repaint and profile snapshot conservation.

## Limitations

This is a first-party source-informed local GUI preview. No original
Minecraft process is launched, no gameplay keyboard hook is enabled,
no original module is executed, and no original profile I/O occurs.
Chat 2 PR #2 remains independent and unchanged.

## Exact-commit CI verification

All workflows passed on corrected R234 implementation commit
`b9cfd6767ff3066fa904590c385d38d701ce7cb1`:

- Offline C++ core: **6/6 jobs successful**, Windows Debug/Release,
  Ubuntu Debug/Release, Linux JNI smoke and sanitizers:
  https://github.com/Trexzo/slinkywinky/actions/runs/43858820970
- Windows JNI preview artifact: Windows x64 native bridge and
  Java 8 JNI roundtrip successful:
  https://github.com/Trexzo/slinkywinky/actions/runs/43858821023
- Injectable Windows Java 8 agent: native loader/JVMTI,
  original-style Java 8 JVM and actual Attach API tests successful:
  https://github.com/Trexzo/slinkywinky/actions/runs/43858821032

The first R234 commit had a missing recovered-settings catalog include
in the new keyboard-router regression; the next commit fixed it.
The first Windows GUI verification then caught a faulty fixture
assertion whose expected string contained literal escape sequences,
despite the pre-Enter test already verifying actual UTF-8 bytes.
The final assertion was fixed without changing GUI runtime logic.
All three workflows passed on the corrected commit.

This checks disposable first-party GL11 input and Java loader fixtures,
not physical Minecraft mouse/keyboard ownership, real game
module execution, or saved original profiles.
