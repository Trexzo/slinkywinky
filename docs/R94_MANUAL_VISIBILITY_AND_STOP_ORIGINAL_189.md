# R94 — finite original Minecraft 1.8.9 operator-controlled native menu

**Status:** verified locally on the connected Windows computer and inside a disposable **original Mojang Minecraft 1.8.9** client. The original-JVM report and separate process-scoped screenshots confirm **native menu toggle/toggle/toggle + cooperative operator stop, restoration and release**. Physical Insert-key input itself is **not yet live-accepted**. PR #1 remains **draft/unmerged**.

## Architecture and input boundaries

R94 is an **injectable Windows x64 native Java 8 Attach/JNI client**, not a Forge mod. All R73/R81/R90 original game source/JVM/PID and `ave.av → bfk.a(FJ)V` render-call ownership gates remain compulsory.

- `--render-visibility=manual` is a **distinct opt-in mode**, requiring all four environment flags `SLINKYWINKY_RENDER_MANUAL=1`, `SLINKYWINKY_RENDER_OVERLAY=1`, `SLINKYWINKY_RENDER_CONTEXT=1` and `SLINKYWINKY_RENDER_CALLBACK=1`. R94's optional isolated seven-stage acceptance harness independently authorizes prior R92/R93 gates using their own existing flags. No default game injection.
- The renderer uses the independently audited original render **RETURN** callback. After eight validated owner-WGL preflight frames, R94 starts with the compact first-party GL11 menu **hidden**. It can toggle visibility on a **foreground-only Insert key rising edge**. This polls `GetAsyncKeyState(VK_INSERT)` only on the original game render thread and only toggles when the WGL-owned game window is the Windows foreground root window and matches the current process. A held key while not foreground cannot become a synthetic keypress when focus returns. No Windows hook, Minecraft keybinding interception or keyboard/mouse event injection occurs. The Insert key requires further physical input and possible keybinding-collision verification; do not claim it is production accepted.
- The alternate **explicit operator controls** `--toggle-visibility` and `--stop-visibility` use separate named Win32 local auto-reset toggle and manual-reset stop events created only by the original in-game native owner-PID session. These commands do **not** use Java Attach, so do not request a JVM safepoint while rendering. Unknown PIDs or inactive sessions are rejected. The event names are PID-scoped, not a security boundary against malicious same-user Windows processes; this is a controlled local test.
- R94's `RenderFrameGate` admits at most **2048** original render callbacks and the cleanup worker has a **20-second deadline**. The three previous limits — R75's 8, R92/R93's 72, and R94's 2048 — cannot be confused. Manual stop is consumed on the original render thread; no GL draw is issued on the stop sample. The Java observer then stops the frame gate, closes it and drains all in-flight native draws, closes the two Win32 control-event handles, retransforms the original bytecode and verifies source-pinned runtime execution shape. The admission lock is only released after proven restoration and control-handle close. If any stage fails, the renderer is not certified.
- Strict R94 target receipts require **at least two toggles, positive shown and hidden samples, exact numeric reconciliation and `stop=operator`**, stable WGL over all accepted samples, and a final source-restored admission release. The operator may stop sooner than 2048 frames; an epoch that simply times out or reaches the cap with no operator stop must not be falsely reported as successful manual control.
- Gameplay/combat modules, clicks, network packets and profiles remain **OFF**. The panel is an inert preview; no module selection or gameplay behavior is enabled.

## Local verification

- Full Windows x64 MSVC Release native/Java8/JNI/Attach/WGL CTest: **25/25 PASS**.
- Python original-vs-fake target and exact-mode receipt suite: **30 tests executed, one platform-specific skip, zero failures**, including a mocked seven-stage original-game workflow and forged/mismatched dynamic operator counters.
- Production `slinkywinky_jni.dll` event open/toggle/stop/close and WGL owner-thread tests on a disposable Win32 GL fixture: PASS. It rejects duplicate start, wrong WGL context, foreign thread, repeated close, and nonexistent event after handle cleanup. The same suite validates R91/93 compact GL pixels and state restoration.
- Source-pinned Mojang 1.8.9 render bytecode entry/exit audit remains established, with original class restoration required.
- An initial real-game attempt reached R94, but the first control failed due to a **stale incrementally built launcher** still using an older Win32 event-name namespace. The loader was explicitly recompiled against the corrected shared header; the native fixture and subsequent full supervised original-game run PASS. Earlier rejected attempts did not bypass any JVM gates or modify unrelated clients.

## Real original vanilla game evidence

**Connected Windows isolated vanilla JVM PID 33004** passed all **seven** independently admitted stages. The target JVM reported:

```
R94_RENDER_CALLBACK_ARMED_LIMIT=2048
R90_RESTORED_SOURCE_PINNED_EXECUTION_PASS
R94_RENDER_CALLBACK_BOUNDED_PASS accepted=44 owner=Client thread#1
R94_WGL_CONTEXT_STABLE_PASS attempts=44 current=44 absent=0 invalid=0 changed=0 stable=true shown=25 hidden=11 toggles=3 stop=operator
R94_MENU_OPERATOR_CONTROLLED_PASS frames=44 toggles=3 shown=25 hidden=11 stop=operator
R81_ORIGINAL_RENDER_ADMISSION_RELEASED_PASS
```

The independent exact-receipt verifier returned `R94_ORIGINAL_189_OPERATOR_VISIBILITY_RECEIPTS_PASS`. The sanitized local report is `vanilla189-isolated/r94-manual-receipts-33004.json`. The supervising operator simulator sent **three separate explicit PID-scoped toggle commands and one stop command**, verifying that menu visibility changed through the new external command route **without a JVM reattach**.

Separate window-scoped `PrintWindow` images are saved **only on the connected Windows machine** in `vanilla189-isolated/r94-window-evidence/` with the prefix `r94-pid33004-frame`. The captured original Minecraft main menu visibly shows the compact native overlay on frame 25/30 and the vanilla main menu unobstructed by frame 34/37. An earlier frame 18 shows R91's full-size bounded overlay as one of the preadmission stages. These screenshots are not copied into Git. The original JVM test was then terminated by its isolated launcher; no other Minecraft process remained.

## Limitations and next gates

The accepted test verifies the **command-driven** user/operator controls, not the *physical Insert key* or full in-world interaction. The input polling includes a foreground restriction, but an independent keybinding/focus/edge test is still required. No FPS/stutter quantification, live world screen, persistent menu, enabled mod UI or production-ready implementation is established. R81's machine report still records `visual_game_acceptance=false` and `in_game_menu_verified=false`, because its receipt verifier does not authenticate the separate screenshots.

Do not merge PR #1 or enable combat based on this result. Next, verify physical Insert focus transitions and keybinding collisions on an authorized isolated original client, and build first-party keyboard/mouse interaction only behind a separately controlled, explicitly reversible admission.
