# R218 — Minecraft-owned source mouse lifecycle: focus loss, stale release, Escape cancellation

**Owner:** Chat 1 / draft PR #1, `reconstruction/r1-core-fidelity`.
**Date:** 2026-11-15.
**Scope:** strictly opt-in ForgeDev Minecraft 1.8.9 / Java 8, isolated TEMP-only title screen. Chat 2's PR #2 module manager remains independently owned and unmodified.

## Why this change was necessary

R215–R217 validated the reconstructed GL11 Sprint switch and paired Minecraft-owned `GuiScreen.mouseClicked`/`mouseReleased` → source hit-test → real PR #2 JNI module manager. However, the R215 manual-mode focus-loss check only called `nativeMouseCancel()`. The Java `ProbeScreen.sourcePressed` flag could stay `true` after the native gesture was canceled, leaving a stale Java-held edge. This did not prove an invalid native request occurred, but broke the intended paired-input lifecycle and made recovery hard to reason about.

R218 makes Java and native cancellation **one ordered owner-thread cancellation path at the GUI callback boundary**, without hooking LWJGL `Mouse.next`, `Keyboard.next`, `WndProc`, raw hardware input or any original injected Minecraft callsite.

### Changes

- The `ProbeScreen.observeFocus(foreground)` method runs only on its exact Minecraft GUI owner. A **false** focus sample cancels the native held `MenuMouseGesture` and clears the Java `sourcePressed` flag together, recording a bounded diagnostic cancellation count. A true sample allows only a **new Down** to start; an old release no longer has a matching Java or C++ Down.
- In **manual mode**, the existing ClientTick END event feeds the **actual** LWJGL `Display.isActive()` result into `observeFocus`, whether foreground or background. Both mouse-down and mouse-up independently sample `Display.isActive()` before admitting a source request; an event observed while inactive is not sent into the C++ request path. This path emits distinct manual-only diagnostics when an inactive display cancels a held press, when foreground is reobserved, when a real GUI callback passes a native source Down, and when a source Up reaches the JNI-acknowledged owner. These logs distinguish the manual verification entrypoint but are not proof of physical hand motion without an independently observed human test.
- A duplicate Down during an existing held GUI gesture is ignored without overwriting the pending native transaction. Existing R98 debounce, native source-hit on both edges, source sequence/generation and WGL context checks remain intact.
- `onGuiClosed` cancels the same held native source and Java state. The isolated automated script tests Escape **while holding** the real painted C++ Sprint toggle, then requires that the previous vanilla screen is restored and the original three real JNI acknowledgements remain unchanged. Exact owner-bound native menu/feed/JNI teardown still occurs after screen close.
- **Automated focus test caveat:** a script explicitly invokes `observeFocus(false)` then `observeFocus(true)` while holding a source-pinned native click. Those calls model the real callback lifecycle and prove cancel/stale-release behavior inside the Minecraft GUI and JNI bridge. They do **not** observe Windows losing focus, press a physical mouse button, or claim real OS focus restoration.
- The test-only Java wrapper and separate C++ native bridge remain opt-in and outside production packaging. No PR #2 source files are modified.

## Live acceptance evidence

Launched a real original Forge 11.15.1.2318 Minecraft 1.8.9 Dev client on Windows 11 with Temurin Java 8, offline identity `R212GuiOffline`, game directory `%TEMP%\slinkywinky-r212-gui-r218-focus-cancel`. The test uses separately staged PR2 `SprintObservationProbe.java` and its previously verified R210 native JNI DLL; no player or world was present.

New exact live log markers:

- `R218_FOCUS_LOSS_STALE_RELEASE_REFUSED_PASS`: admitted source Down, rejected duplicate Down, simulated foreground false canceled the JNI held gesture and Java held flag, foreground true then old mouse Up produced **no** request.
- `R218_HELD_CLICK_ESCAPE_CLOSE_CANCELED_PASS`: a later accepted source Down was held while the real Minecraft-owned GUI handled the **scripted Escape callback**. `onGuiClosed` canceled that native held gesture and Java state. No fourth request was delivered; the old vanilla title-screen instance was restored.

All earlier outcomes still held: three native-acknowledged Sprint requests ON / OFF / ON through the actual PR2 native manager, accurate GL11 source switch `#404040` / `#F35D12` rendering, R217 single-pass count, no-world `active=false`, zero ArrayList active rows, source mouse negative controls, independent menu/feed and JNI owner closure, supported `FMLCommonHandler.exitJava(0,false)`. Full real Forge run `BUILD SUCCESSFUL` and exit **0**.

`python tests/r218_forge_focus_cancel_receipt.py --log <real Forge latest.log>` independently checks that the two cancellation receipts occur exactly once and causally before any new legitimate source request / after the last source pixel / before native teardown. `--selftest` executes **20 mutation-based negative controls**. It inherits the R217 → R216 → R215 receipt validators and their strict JNI/GL11/closed-previous-screen checks.

## What is NOT proven

**This is not a physical mouse or real OS focus-loss test.** The script calls vanilla Minecraft `GuiScreen` methods and explicitly simulated focus samples. The manual path is connected to actual `Display.isActive()` samples but has not been exercised by a human physical click and real foreground/background transition. Windows can theoretically lose and regain focus between samples; only sampled transitions are claimed. There are no Minecraft LWJGL queue hooks or original-injected-client changes, nor in-world Sprint active state. Production physical focus and full injector behavior remain future acceptance gates.

Both PRs must remain draft and unmerged.

## Final regression and default distribution

Full Windows x64 CMake Release build exit **0**. The full suite reports **107/107 CTest PASS**, 40.59 seconds, including the new `r218_forge189_focus_and_close_cancel_negative_controls` and all inherited WGL, original-input safety, native attach, GUI/source-hit and JNI tests.

The independently repeated live **R214 legacy Java-button→C++ source request→real PR2 JNI** acceptance exited 0 with its full native state and cleanup. The final **R218 scripted in-process live** Forge acceptance also exited 0 after the manual-only diagnostics were added, and its real `latest.log` passed both the R218 validator and **20/20 negative controls**.

Without any R212–R218 opt-in, `gradle --offline --no-daemon clean jar` returned exit **0**. Inspection confirms the JAR retains the dormant standalone GUI bridge but **does not bundle** `SprintObservationProbe.class` from Chat 2 or any `.dll` file.

## Manual physical-input acceptance — prepared, NOT executed

The following is a reproduction guide for a future **human-controlled** Windows test, not a claim of completed hardware validation. The two separately built test DLLs and PR2 Java source must be present; start in the `experimental/forge_gui189` project directory using the preinstalled Java 8 / Gradle 2.14.1 environment. Never run this against a production Minecraft instance or install an input hook.

```powershell
Remove-Item Env:SLINKYWINKY_R215_SOURCE_MOUSE_TEST -ErrorAction SilentlyContinue
$env:SLINKYWINKY_R215_MANUAL_MOUSE='1'
$env:SLINKYWINKY_R213_PR2_JAVA_SOURCE=Join-Path $env:TEMP 'slinkywinky-r213-pr2-java-only'
$env:SLINKYWINKY_R213_PR2_JNI_DIR="$env:LOCALAPPDATA\SlinkyWinky\R210-JNI-Integration-Build\Release"
$env:SLINKYWINKY_R214_GL11_JNI_DIR='C:\Users\Felix\Desktop\Slinky-R1-Workspace\SlinkyWinky-R206-Build\Release'
$env:SLINKYWINKY_R212_GUI_GAME_DIR=Join-Path $env:TEMP 'slinkywinky-r212-gui-r218-manual'
& 'C:\Users\Felix\.openabyss-tools\gradle-2.14.1\bin\gradle.bat' --offline --no-daemon runClient --console plain
```

The accepted disposable title-screen GUI offers the reconstructed GL11 Sprint toggle. Its manual log distinguishes `R218_MANUAL_GUIScreen_SOURCE_DOWN_ACCEPTED`, `R218_MANUAL_GUIScreen_SOURCE_RELEASE_JNI_ACK id=N`, `R218_MANUAL_DISPLAY_INACTIVE_HELD_SOURCE_CANCELED` and `R218_MANUAL_DISPLAY_FOREGROUND_REOBSERVED` based on the actual Minecraft GUI callbacks and observed `Display.isActive()` values. A human should test a normal click, then hold Sprint while switching Windows focus away, release while inactive, return and check that no extra JNI request appears, then hit Escape and verify vanilla screen restoration and owner cleanup. **No one has executed or verified those hardware actions in this R218 run.**
