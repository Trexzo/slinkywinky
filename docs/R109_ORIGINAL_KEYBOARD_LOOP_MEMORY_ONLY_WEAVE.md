# R109 — source-pinned original Minecraft keyboard queue callsite, memory-only

**State:** draft/unmerged PR #1; independent offline ASM preflight, no live Minecraft keyboard consumer or class retransformation. This is an incremental prerequisite for an opt-in injectable ClickGUI, not a Forge mod.

## Why this change exists

R107 proved which original `ave.s()V` method reads LWJGL keyboard events, including ordinary Minecraft gameplay/keybinding delivery. R108 established the first-party C++ routing policy with simulated exclusive delivery. Neither gave an independently reviewed bytecode patch boundary. R109 demonstrates in memory that exactly one admitted `Keyboard.next()Z` call can be substituted while leaving normal method instructions otherwise unchanged.

## Exact contract

`OriginalKeyboardLoopWeaveAudit` (in the **test-only offline** Java source tree) first checks the SHA-1 and size of the original Mojang Minecraft 1.8.9 client using the existing R64 verifier. Its `ave.class` must have SHA-256 `430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82`. R107 then independently requires the expected unique instance method `ave.s()V`, exactly one `org/lwjgl/input/Keyboard.next()Z`, original keyboard-event polling, and the normal `avb`/GUI dispatch instructions.

Only that single `INVOKESTATIC Keyboard.next()Z` call inside `ave.s()V` is rewritten to the deliberately unresolved **test-only** `dev/slinkywinky/r109/OfflineOnlyKeyboardOwner.next()Z` (same stack signature). A full per-method ASM instruction trace is recorded before and after, with all other method callsites, instruction opcodes, fields, operands and method signatures required to match. The original input byte array and its SHA-256 remain unchanged. The altered classfile is discarded in memory; no changed proprietary bytes are saved, embedded, executed, or distributed.

This does not yet implement a runtime keyboard queue owner or actual suppression of game input. In particular, a successful transformed-class preview **cannot** establish JVM linkage of a real shim, physical Insert/Escape behavior, zero input pass-through, correct mouse handling, or successful live retransformation/restoration.

## Source isolation

- `tests/java/dev/slinkywinky/attach/OriginalKeyboardLoopWeaveAudit.java`: exact Mojang original offline dry-run and instruction-preservation proof.
- `tests/java/dev/slinkywinky/attach/OriginalKeyboardLoopWeaveSmoke.java`: synthetic-only one-call rewrite test plus changed-owner/duplicate-queue/wrong-shape/wrong-hash/previously patched refusals.
- `tests/attach_smoke.py`: runs R109 synthetic refusals in the disposable Java 8 test process.
- `tests/official_189_render_audit.py`: runs R109 original-1.8.9 dry-run after the original R107 keyboard-shape check as part of the GitHub original-client audit.
- `CMakeLists.txt`: builds R109 only into `slinkywinky_render_audit.jar` and test-class fixtures; **not** into `slinkywinky_instrument.jar` or the production Java/native agent.

## Verification and restrictions

A locally verified original Minecraft 1.8.9 JAR produced `R109_ORIGINAL_189_KEYBOARD_NEXT_EXACT_ONE_CALLSITE_MEMORY_ONLY_PASS` and `R109_ALL_OTHER_METHOD_AND_INSTRUCTION_TRACES_UNCHANGED_PASS`. Synthetic positive/negative-control receipts also passed. Local Windows Release CTest **30/30 PASS** (including Java 8 Attach, JNI and isolated WGL); Python **55 tests run, one skipped, zero failures**. `jar tf` confirms that the new `OriginalKeyboardLoopWeaveAudit` is present in the offline `slinkywinky_render_audit.jar` but absent from `slinkywinky_instrument.jar`; the unresolved test-only marker is never shipped. Only the two existing SpawnPK Java processes remained. Do not merge PR #1 or enable gameplay/combat modules based on offline evidence.

## Next live gate

A separate, disabled-by-default, explicitly authorized input owner must be built and admitted on the exact original game-thread and classloader, with a UI-visible session lifetime. It must route actual key-down and key-up events without allowing GUI-owned keys to reach `avb` gameplay bindings or vanilla GUI dispatch, preserve non-UI input, reject focus loss and unknown sources, and prove bounded shutdown, no stuck keys, and source-pinned bytecode restoration. A human foreground keyboard acceptance remains mandatory before claiming usable ClickGUI.
