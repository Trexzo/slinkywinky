# R110 — Dormant Java 8 LWJGL2 keyboard queue ownership

**Repository:** `Trexzo/slinkywinky`, reconstruction draft PR #1. **Runtime admission:** disabled by default. **Original Minecraft hook:** not installed by R110. **Forge dependency:** none.

## What actually changed

`loader/java/dev/slinkywinky/instrument/KeyboardQueueOwner.java` is a first-party Java 8 input queue owner with the exact static `next()Z` shape required by R109. It is compiled into the injectable `slinkywinky_instrument.jar` but **no production code invokes `bind`, `arm`, `observeForeground`, or replaces the original `ave.s()V` callsite**. Unarmed behavior and external loader options remain unchanged.

The owner uses Java reflection to call the original LWJGL 2 `org.lwjgl.input.Keyboard.next()Z`, `getEventKey()I`, and `getEventKeyState()Z` methods without bundling an LWJGL replacement. It refuses an unexpected class name, loader, non-static or incompatible method. Source admission is a **caller-provided prerequisite**, not independently proven by this new class.

On an admitted owner thread, `VANILLA` mode transparently calls original `Keyboard.next()`, preserving LWJGL's active event metadata for Minecraft's later getters. `MODAL` mode instead drains event records without returning `true` to the original game loop and forwards only distinct key-down/up edges to the inert local preview callback. Repeated key-downs do not repeat actions; character-only key 0 is suppressed until proper text input exists. The loop drains at most 128 events per poll.

`DRAINING` withholds key-up from a closing GUI (including Escape/Insert) until all held keys have been released **and** an empty event queue has been observed. `REFUSED` blocks forwarding on focus loss, wrong source method/exception, or callback failure. A wrong-thread call does not claim ownership. A new modal epoch must be strictly greater than any previous epoch. `unbind` requires caller attestations of verified original class restoration, released physical controls, and a hidden menu, and actively refuses restoration while stale queue records remain. The active focus heartbeat expires after 250 ms unless renewed; a future live game integration must supply this verified heartbeat from the independent WGL/HWND owner.

## Evidence

The locally installed original vanilla 1.8.9 manifest selects `org.lwjgl.lwjgl:lwjgl:2.9.4-nightly-20150209`. Java 8 `javap` of the installed JAR confirms:

- `public static boolean next();`
- `public static int getEventKey();`
- `public static boolean getEventKeyState();`
- `public static char getEventCharacter();` (not consumed by R110)

`tests/java/org/lwjgl/input/Keyboard.java` is a disposable *test fixture only*, never in `slinkywinky_instrument.jar`. `tests/java/dev/slinkywinky/attach/KeyboardQueueOwnerSmoke.java` validates normal pass-through, exclusive modal draining, keydown/key-up, repeat suppression, arbitrary gameplay keys, character-only key 0, 128-event cap, close-key release, foreground refusal, wrong-thread refusal, stale epoch denial, source failure, callback exception, and explicit restoration.

`tests/attach_smoke.py` runs the R110 fixture in the Java 8 native-agent CTest. `CMakeLists.txt` stages the new input-owner Java class in the instrument JAR and the mock keyboard **only** in test artifacts. Original Minecraft Java or proprietary LWJGL classes are not copied to GitHub.

## Local verification

Windows Release build succeeded; full CTest **30/30 PASS** and Python **55 tests, one skipped, zero failures**. The Java 8 mock LWJGL queue gate passed all keydown/keyup, repeat, close-drain, focus-loss, source-error, callback-error and 128-event batch-limit checks. A fresh supervised isolated original vanilla Minecraft 1.8.9 run (PID **29964**) passed existing `R94_ORIGINAL_189_OPERATOR_VISIBILITY_RECEIPTS_PASS` show/hide/stop; it was **scripted operator control, not physical keyboard acceptance**, and the isolated JVM exited afterward. Jar-content checks confirm that production `slinkywinky_instrument.jar` contains only `KeyboardQueueOwner` and its nested classes, not the test-only `org.lwjgl.input.Keyboard` replacement or the R109 offline ASM audit. Two unrelated SpawnPK Java processes were unchanged.

## Crucial caveats and next gate

**This is not a physical-keyboard or no-gameplay-pass-through acceptance.** No live original-class keyboard redefinition, input interception or source-pinned read/write transaction runs in R110. R107 identified other normal gameplay dispatch in `ave.s()V` outside the one queue call. Replacing `Keyboard.next()` alone does **not** prove that those unrelated handlers cannot observe key state. The C++ R108 menu navigation also remains disconnected from this Java callback.

The next step is an explicitly authorized, bounded original-JVM source-pinned integration that proves actual GUI-owned key-down/up cannot reach vanilla keybindings/GUI, that non-UI keys are unaffected, that focus/viewport/session changes revoke ownership, and that all changed `ave` bytecode is restored and independently verified. A real foreground keyboard test remains a separate required acceptance. Do not merge or enable combat based on these fixture results.
