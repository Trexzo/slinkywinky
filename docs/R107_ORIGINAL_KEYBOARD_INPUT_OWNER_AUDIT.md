# R107 — read-only original Minecraft 1.8.9 keyboard input-owner audit

**State:** staged on SlinkyWinky draft PR #1, not merged. No Java input transformer or Win32 keyboard/mouse hook is installed. R106 remains the last production behavior change.

## Why

The existing R106 Insert visibility classifier proves eligible original-game focus but does not stop the same key reaching Minecraft's ordinary LWJGL/Java processing. Input eligibility and modal no-click-through are separate safety properties; do not describe a visible overlay or Windows foreground status as event suppression.

## Exact source evidence

The test-only `KeyboardInputCallsiteAudit` reads the exact unmodified Mojang 1.8.9 client JAR with size 8,461,484 bytes, SHA-1 `3870888a6c3d349d3771a3e9d16c9bf5e076b908`, and `ave.class` SHA-256 `430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82`. It never loads Minecraft classes or launches a game.

ASM 5.2 confirms the single original input owner `ave.s()V` and a unique `org/lwjgl/input/Keyboard.next()Z` call. The method contains two `getEventKey()`, three `getEventKeyState()`, one `getEventCharacter()` calls, and normal Minecraft keybinding dispatch via `avb.a(IZ)V` and `avb.a(I)V`, both before and after the queued keyboard call. Original GUI (`axu`) calls are also present.

**Interpretation:** This confirms that Minecraft has ordinary gameplay/keybind and GUI dispatch paths in the same tick method. It **does not** prove which individual event reaches a specific path at runtime, and it cannot certify that a SlinkyWinky overlay blocks or consumes any of them.

## Source and tests

- `tests/java/dev/slinkywinky/attach/KeyboardInputCallsiteAudit.java` is strictly offline. It rejects unverified JARs, non-original classes, a missing/duplicated event loop, wrong event-method counts or missing normal dispatch.
- `KeyboardInputCallsiteAuditSmoke.java` creates **only synthetic classfiles** and verifies expected admission plus 9 negative controls and invalid class bytes.
- The standalone verifier is compiled into the existing *offline* render audit JAR, not the injected instrumentation JAR. Synthetic tests are compiled under attach-test-classes.
- The Windows Attach CTest runs R107 synthetic refusals. The existing official Mojang 1.8.9 offline CI audit now additionally invokes R107 after independently validating the original renderer. No original game JAR or proprietary bytecode is committed.

## Verification

Locally, the full original client classpath audit printed `R107_ORIGINAL_189_KEYBOARD_EVENT_PATH_AUDITED owner=ave.s()V next=1 eventKey=2 eventState=3 eventChar=1 gameBindingsBefore=2 gameBindingsAfter=2 guiDispatchPresent=true` and `R107_NORMAL_GAMEPLAY_DISPATCH_PRESENT_NOT_MODAL_INPUT_CERTIFICATION`. The synthetic negative controls passed. **Local Windows Release CTest: 29/29 PASS** (including `native_agent_attach` with the new R107 generated-bytecode denials); **Python: 55 tests run, one skipped, zero failures**. GitHub CI is not considered accepted until its final commit completes.

## Remaining admission before any physical ClickGUI input

Implement an independent source-pinned experimental event-owned path which can prove a GUI-owned key or click is not also delivered to `avb` normal gameplay bindings or vanilla GUI handlers, including both keydown/key-up, repeated keys, focus loss, viewport changes, new render owner epochs and cooperative bytecode restoration. The implementation must be disabled by default, allow external owner stop, and leave non-GUI gameplay input unchanged. A fresh isolated original client and actual foreground human input test are required; a synthetic fixture or SendInput receipt alone is insufficient.

Until then, combat/movement features remain disabled and PR #1 remains draft/unmerged.
