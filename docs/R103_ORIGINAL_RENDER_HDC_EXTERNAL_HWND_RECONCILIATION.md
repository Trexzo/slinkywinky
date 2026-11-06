# R103 — source-pinned original game render HDC reconciled with its external HWND

**State:** verified in a supervised disposable genuine vanilla Minecraft 1.8.9 Java 8 process. Full local Windows x64 Release + JNI/Attach/WGL CTest **28/28 PASS**; strict original render receipt Python suite **36 run, one platform-specific skip, no failures**. PR #1 remains **draft/unmerged**. This is a **read-only identity/foreground audit**, not a user-operated in-game ClickGUI or gameplay module.

## The R102–R103 gap

R102 independently located a single visible 854×480 original Minecraft 1.8.9 window from outside the Java process, using a PID/kernel process-creation identity and eight passive HWND observations. R101's first-party JNI/native `audit_game_render_window` was tested on a disposable WGL fixture, but the original game agent did not register that read-only Java audit mode or expose it through the exact native loader. These were **separate** results: R102 alone could not certify that its HWND was the active source-pinned original Minecraft GL renderer's window.

R103 now connects and independently verifies both sides **for the same isolated original process and process-birth identity**:

1. `slinkywinky_loader.exe attach PID JDK8_HOME --render-window-audit=8` is a **distinct original-only** Java 8 Attach mode requiring `SLINKYWINKY_RENDER_WINDOW=1`, `SLINKYWINKY_RENDER_CONTEXT=1` and `SLINKYWINKY_RENDER_CALLBACK=1`. It does **not** enable the R91 overlay, R100 operator controls, mouse interception, SendInput, WndProc subclassing, focus adjustment, packets or combat.
2. The real source-pinned original `bfk.a(FJ)V` render owner makes eight read-only JNI calls. Each checks the exact current **WGL HGLRC and HDC**, resolves `WindowFromDC` and its root HWND, validates current OS thread/PID ownership, exact `Minecraft 1.8.9` title, unique visible/unminimized window, physical client rect, and unchanged OpenGL viewport. A real background window is classified **background**, not accidentally treated as foreground. Invalid, ambiguous, wrong-thread/viewport or changed-context samples fail closed.
3. The source-pinned original render callback is detached, the **original bytecode execution shape independently restored**, and the shared R81 admission gate released before a successful full receipt can be claimed. Each stage is fresh target-JVM stdout, not solely a helper's Attach success message.
4. A separate Python Win32 process (R102 inspector) samples the **same PID and kernel process creation FILETIME** eight times immediately before and eight times after the original native audit. The verifier demands one matching uniquely titled original root HWND, identical **actual physical width/height** across both external observations and the native WGL viewport, plus zero refused samples. It independently checks the process birth and original Mojang JAR before/after each renderer step. Only source-pinned restored and released stages are accepted.
5. The JSON report records `window_crosscheck` with external-pre, native-original-WGL and external-post evidence. It explicitly keeps `visual_game_acceptance=false`, `in_game_menu_verified=false`, `foreground_or_input_capture_certified=false` and `os_input_sent=false`. The same-PID/window identity agreement does **not** amount to certified physical keyboard/mouse capture, click-through prevention or a user-visible GUI.

## Accurate original-client testing timeline

- **First run**, original vanilla PID **26528**: R90 preflight, original callbacks and R78 stable context all passed, but the final window stage correctly **refused** the Attach because the JNI agent dispatcher did not yet register the read-only window-audit mode. This was a *missing mode-registration defect*, not native HWND rejection. The missing Java agent case was added separately and independently confirmed present in the rebuilt JAR via Java 8 `javap`.
- **Second run**, fresh original vanilla PID **34512**: the four-stage source-pinned original-game R103 window audit passed. This run established the native-HDC/WindowFromDC results, but was before independent bracketed external dimension reconciliation was added.
- **Final supervised run**, fresh original vanilla PID **9192**: completed **all four** source-pinned original admission stages with separate preflight/8-original-callback/8-stable-WGL/8-real-WGL-window checks and verified original renderer restore/gate release. All three window evidence tracks agreed on **854×480** client pixels, with **8/8 background** observations on original render thread and **8/8 background** in each external bracket. Foreground samples were **zero**; no system focus changes or game input were attempted. The supervising process emitted:

```text
R103_ORIGINAL_RENDER_HDC_EXTERNAL_WINDOW_RECONCILED_PASS
R103_ORIGINAL_189_RENDER_WINDOW_RECEIPTS_PASS pid=9192
R103_READ_ONLY_NO_FOREGROUND_INPUT_OR_GAMEPLAY
```

- A separate Python process **reopened the resulting JSON using `json.load`**, verified all four source-checked stages, exact original process identity, 854×480 native/external dimension agreement, eight native samples, pre/post eight-sample external window observations, and the false input-certification flag. It emitted `R103_INDEPENDENT_CROSSCHECK_JSON_PASS`. The owned test child was terminated by the isolated harness; independent process enumeration returned `R103_NO_MINECRAFT_JVM_REMAINING`.

Final machine-readable report on the authorized Windows test host:

```text
C:\Users\Felix\AppData\Local\Temp\slinkywinky-r89-inject-build\vanilla189-isolated\r103-window-receipts-9192.json
```

## Regression evidence

- Windows MSVC Release + Java 8 agent/JNI/Attach/WGL native full build **PASS**, **28/28 CTest PASS**.
- Strict original-JVM receipt tests: **36 run**, one platform-specific skip, zero failures. The new forged/duplicate/wrong-stage R103 tests reject incorrect foreground/background totals, unbound context owner, invalid dimensions, inconsistent callback counts, missing restoration and false window identity markers.
- All previous R78–R100 original/standalone paths remain behind their preexisting separate admission checks. R101 JNI window audit logic is otherwise unchanged.

## Remaining steps before real user keyboard/mouse ClickGUI

**Foreground ownership is unverified** because the genuine original window was background for every R102/R103 passive observation. This work intentionally does not circumvent Windows foreground restrictions or bypass the earlier R95 focus refusal. The next admission should require actual user-owned foreground focus with no forced focus, correct release/rearm across lost focus, explicit modal input capture that blocks normal Minecraft gameplay input, and independently observed show/hide and click-through behavior before permitting any physical input into the menu.

Window-to-HDC reconciliation is not proof of a faithful Slinky reconstruction, nor of gameplay/combat module functionality. PR #1 remains draft/unmerged until separately certified.
