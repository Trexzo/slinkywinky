# R100 — External-operator menu navigation in source-pinned original Minecraft 1.8.9

**Status:** implemented on `reconstruction/r1-core-fidelity` in unmerged draft PR #1. A **supervised live vanilla 1.8.9 JVM** passed all **seven** strict admission stages, including four externally acknowledged first-party menu actions, explicit stop, exact original-source renderer restoration and admission release. The existing user keyboard/mouse paths and gameplay/combat features are **not** activated.

## Advance from R99

R99 proved the native menu could persist across 64 original-game render frames, but its four UI navigation actions were internally scripted inside the injected Java callback. R100 replaces that action source with a distinct **operator-driven control path**:

- `include/slinkywinky/windows_menu_operator_events.hpp` assigns **fixed** action names to eight local-only `MenuNavAction` controls and one stop command. Each event name includes the original target **PID and kernel process creation FILETIME**, guarding against PID reuse. Separate acknowledgement signals and a single-producer mutex scope the reply to one serialized sender. The names are `Local\\` Windows objects; these events are not general network endpoints and do not synthesize keystrokes.
- `src/windows_loader.cpp` accepts `--menu-next-tab`, `--menu-previous-tab`, `--menu-next-row`, `--menu-previous-row`, `--menu-expand`, `--menu-toggle-preview`, `--menu-increase`, `--menu-decrease` and `--menu-stop`. These **do not invoke the Java Attach API again**, avoid JVM safepoint stalls and never inject input into Minecraft. Every requested command requires an acknowledgement from the original WGL owner (within 2.5 seconds) before reporting success; refusal and timeout are explicit failures.
- `src/jni_bridge.cpp` creates the command events only during a separately armed R100 session. Its `nativePollOperatorMenu` runs only when the current WGL context/DC and render owner match the admitted, active `WglRetainedMenuPreview`. It handles **at most one event per frame**, and the entire session is bounded to **16 events**, preventing unbounded UI work. Unsupported/invalid local menu actions are acknowledged as **refused**, never converted to game module execution. `nativeCloseOperatorMenu` removes the PID/birth event endpoints and retained preview after the source-hook callback fence has drained.
- The existing R99 direct JNI scripting entry point is expressly denied while R100's external operator mode is active. R94 manual visibility controls and R99 finite scripted navigation retain their previous independent admission gates; no default behavior changes.
- The first-party menu state remains restricted to local `MenuState`/`MenuProfileBridge` settings and paint. **58 catalog entries are inert**; any display of combat/movement names is not proof of a working module. R97 hit testing and R98 standalone mouse gestures are **not installed inside Minecraft**.

## Admission, commands and operator stop

The new, independent Java 8 Attach option is `--render-menu=operator` (not `--render-retained=72`). This requires **all four** explicit environment gates:

```text
SLINKYWINKY_RENDER_OPERATOR=1
SLINKYWINKY_RENDER_OVERLAY=1
SLINKYWINKY_RENDER_CONTEXT=1
SLINKYWINKY_RENDER_CALLBACK=1
```

Only the exact original Mojang vanilla 1.8.9 JAR/source-verified loaded renderer is admitted, with the existing strict `R90` original runtime execution-shape provenance before weaving. Test-mode bypass, Forge and competing runtime hooks remain refused.

The original `bfk.a(FJ)V` render-RETURN callback first collects eight stable native current-WGL samples and then paints the compact retained GL11 menu with state restoration. **No** global keyboard or mouse hooking, gameplay event dispatch, game keybind change, network packet, or OS `SendInput` is permitted in R100.

The session ends on explicit `--menu-stop` (and its render-thread acknowledgement), an error, or its independent time/frame bound (up to 2048 accepted callbacks, a 20-second cleanup deadline). A normal successful R100 receipt requires an operator **stop**, not just a timeout. The class hook is uninstalled and the source-pinned original renderer verified/restored **before** releasing the common admission lock.

An operator's example command format, after exact original-game admission, is:

```powershell
slinkywinky_loader.exe attach <PID> <JDK8_HOME> --menu-next-row
slinkywinky_loader.exe attach <PID> <JDK8_HOME> --menu-expand
slinkywinky_loader.exe attach <PID> <JDK8_HOME> --menu-toggle-preview
slinkywinky_loader.exe attach <PID> <JDK8_HOME> --menu-next-tab
slinkywinky_loader.exe attach <PID> <JDK8_HOME> --menu-stop
```

These commands **cannot** operate without a currently armed original-client R100 native event channel and an alive target process with the same kernel birth identity. Each command's success is an acknowledgement of a **local UI action only**.

### Important security limit

PID + process birth prevents accidental PID recycling, **not** unauthorized use by another local process with the same Windows security principal. Windows named events are local-session IPC, **not cryptographically authenticated commands**. This experimental control path must remain limited to verified local original-client sessions, with combat/packet actions excluded, until stronger caller/session authentication and interactive focus/capture ownership are established.

## R100 actual original-JVM evidence

The original vanilla 1.8.9 sandbox game PID **36080**, checked against exact Mojang JAR SHA and kernel birth token, completed **seven independent admission stages**:

1. Original renderer preflight and preflight gate release.
2. Eight source-pinned original render callbacks, original bytecode restore.
3. Eight stable passive WGL samples, original bytecode restore.
4. R91 one-frame original native GL11 draw and restore.
5. R92 64-frame original native GL11 draw and restore.
6. R99 64-frame retained local menu script, restore.
7. R100 **five separate externally sent, owner-acknowledged control events**: NextRow, Expand, TogglePreview, NextTab, STOP.

The R100 original source-pinned JVM itself reported:

```text
R100_RENDER_CALLBACK_ARMED_LIMIT=2048
R90_RESTORED_SOURCE_PINNED_EXECUTION_PASS
R100_RENDER_CALLBACK_BOUNDED_PASS accepted=13 owner=Client thread#1
R100_WGL_CONTEXT_STABLE_PASS attempts=13 current=13 absent=0 invalid=0 changed=0 stable=true shown=5 actions=4 refused=0 tab=1 focus=17 preview=1 stop=operator
R100_OPERATOR_MENU_CONTROLLED_PASS actions=4 tab=1 focus=17 preview=1 stop=operator
R100_EXTERNAL_EVENT_PREVIEW_NO_GAME_INPUT_ACTIONS
R81_ORIGINAL_RENDER_ADMISSION_RELEASED_PASS
```

The separate supervising process verified exact ACKs for all five commands and reported **`R100_ORIGINAL_189_OPERATOR_MENU_RECEIPTS_PASS`**. The sanitized original-JVM receipt report is saved locally under `vanilla189-isolated/r100-operator-receipts-36080.json`, with seven checked stages and `visual_game_acceptance=false`, `in_game_menu_verified=false` deliberately retained. The disposable game JVM exited; no Minecraft JVM remained afterward.

The first real R100 run proved the external commands but initially emitted an `R100_WGL_CONTEXT_UNVERIFIED` marker due to a missed operator-stop clause in the final Java stable predicate. That gate was fixed in commit `a7b62a15`; the second, fresh original-client run with PID 36080 passed all original callback/WGL/restoration checks. This is a verified **local native GL/menu-state** result, **not an independently photographed in-game UX**.

### Regression evidence

- Production JNI/WGL CTest: real named-event registration, duplicate-channel refusal, original WGL owner checks, four admitted local actions, rejected unsupported action and refused ACK, STOP acknowledgement, wrong-thread refusal, channel teardown and GL state restoration — PASS.
- Strict Python original-JVM receipt tests: **35 run**, 1 platform-dependent skip, zero failures; forged/duplicate/wrong-stage R100 receipt counters are rejected.
- Full Windows MSVC Release build: PASS. Complete Java 8/JNI/Attach/WGL CTest suite should be recorded against the final code head; do not infer it from earlier revisions. GitHub Actions CI requires separate final-head verification.

## Next milestone

R100 is **operator-controlled in-game menu navigation through an external command-line control path**, not a conventional Slinky keyboard/mouse ClickGUI. The R95 foreground Insert test has not passed and the R97/R98 standalone input capture has not been verified for the real Minecraft window. The next controlled integration needs a source-admitted game-window identity, focus transitions, no-click-through policy and actual user mouse/keyboard actions, plus an independent on-screen screenshot/visible selected-row test and complete safe shutdown. The original Slinky gameplay modules are separate unfinished work; no combat module is certified or activated. PR #1 remains **draft/unmerged**.
