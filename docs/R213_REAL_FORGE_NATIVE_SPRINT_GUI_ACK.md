# R213 — Real Forge-owned GuiScreen toggle → actual PR #2 JNI Sprint manager

**Owner:** Chat 1, `reconstruction/r1-core-fidelity` / draft PR #1.
**Cross-branch dependency for the test only:** Chat 2, `integration/openslinky-fd9c592-reference` / draft PR #2. **No PR #2 tracked source edited or copied into PR #1.**

## What is real now

R212 proved Minecraft 1.8.9's own `GuiScreen` button and Escape handlers in a temporary offline Forge client, but its Sprint request was only a local preview counter. R213 adds the optional Java 8 `R213NativeSprintBridge` to the same isolated acceptance. It calls the **actual** PR #2 `SprintObservationProbe.setEnabled(boolean)` and `state()` via reflection from the exact admitted game-client owner thread, backed by PR #2's real native JNI handle. The reflection is not an arbitrary plugin search or input interception: the one exact class and method signatures are pinned and the response is checked for the exact `Sprint` name, requested value, no-world active status and nonregressing generation.

The module-control bridge is enabled **only** with `SLINKYWINKY_R213_NATIVE_TEST=1` plus explicit PR #2 Java source and JNI DLL paths. It refuses a missing library or missing class; it never manufactures an acknowledgement. The default Forge experiment remains independent and contains no PR #2 `SprintObservationProbe` class.

The title screen has **no world/player**, so native `requested_enabled` changes are accepted while native `active_in_world` legitimately remains false. A user request is not a gameplay action. After each JNI acknowledgement, the actual Forge `GuiButton.displayString` is refreshed using the copied acknowledged state (requested ON/OFF and active OFF); not `MenuState.preview_enabled` and not a fabricated active HUD row. The fixture uses `GuiScreen.mouseClicked` **directly** to supply three *scripted* clicks through the vanilla method, not physical mouse hardware events.

## Verified actual live Forge 1.8.9 acceptance — November 14, 2026

Windows / Temurin Java 8 / ForgeGradle 2.1 / Forge 11.15.1.2318. The game directory was exclusively `%TEMP%\slinkywinky-r212-gui-r213-native`, username `R212GuiOffline`; the game had no world/player.

The accepted event sequence, logged from the actual Minecraft client thread:
1. Native Sprint owner initialized with explicit requested OFF and active OFF.
2. Minecraft installed the test-owned `GuiScreen`.
3. Vanilla GUI button click #1 → **real native JNI** acknowledgement `id=1 requested=true active=false generation=0`.
4. Vanilla GUI button click #2 → **real native JNI** acknowledgement `id=2 requested=false active=false generation=0`, and a verified GUI label derived from that acknowledgement.
5. Vanilla GUI button click #3 → **real native JNI** acknowledgement `id=3 requested=true active=false generation=0`, again verified in the GUI label.
6. JNI `SprintObservationProbe.close()` completed; Escape restored the **same** pre-existing vanilla title screen.
7. `FMLCommonHandler.exitJava(0,false)` closed the isolated client. **Gradle BUILD SUCCESSFUL, process exit 0.**

`tests/r213_forge_gui_native_receipt.py --log` accepts the real Forge client log exactly once in order. Its **16 negative controls** reject duplicate/missing/wrong-state native acknowledgements, changed ordering, changed identity/version, a missing close or a trapped JVM exit. Offline receipt checks do not independently prove the source provenance of a library; that is supplied by the explicit local Java/JNI source and artifact path and checked hash.

The exact Java wrapper staged **outside PR #1** was extracted from PR #2 head `8ce56a2081a442e38753a871689b5d1b8ff899b4`, `java/dev/slinkywinky/SprintObservationProbe.java`, with staged source SHA-256 `41204859B9FE94020509D978CC4856942307F2A836762BD777F7C2160ADD436C`. The real native JNI DLL was a previously compiled/verified **R210 PR #2** Release artifact `%LOCALAPPDATA%\SlinkyWinky\R210-JNI-Integration-Build\Release\slinkywinky_jni.dll`, SHA-256 `B8607387DC8D337DAD980607B2FE7D1E80A9878B3DE06DDAB6F931626E8AE281`. **The DLL was not freshly rebuilt from the newer PR #2 head**, so this acceptance proves the stable `setEnabled/state/close` ABI across these two artifacts, not every newer native PR #2 behavior.

## Packaging and backward compatibility

`gradle --offline --no-daemon clean jar` without R213 env variables passed. Inspecting its first-party JAR proved:
- `R213NativeSprintBridge.class` is included as a dormant test-only adapter.
- `dev/slinkywinky/SprintObservationProbe.class` is **not** included.
- No native DLL is packaged into the default JAR.

A separately launched isolated R212 baseline `runClient` without the R213 native opt-in also passed with the **original single request-only Sprint click**, vanilla Escape restoration and clean Gradle exit 0. The Java/JNI bridge requires full opt-in and source availability; no default game module activation or key mapping was added.

## Reproduction: separate explicit test dependencies

Only inside the dedicated development test workspace with Java 8 / locally cached ForgeGradle:

```powershell
$env:SLINKYWINKY_R213_NATIVE_TEST = '1'
$env:SLINKYWINKY_R213_PR2_JAVA_SOURCE = Join-Path $env:TEMP 'slinkywinky-r213-pr2-java-only'
$env:SLINKYWINKY_R213_PR2_JNI_DIR = "$env:LOCALAPPDATA\SlinkyWinky\R210-JNI-Integration-Build\Release"
$env:SLINKYWINKY_R212_GUI_GAME_DIR = Join-Path $env:TEMP 'slinkywinky-r212-gui-r213-native'
gradle --offline --no-daemon runClient
```

The first source directory must already contain the verified exact PR #2 `dev/slinkywinky/SprintObservationProbe.java`. The second directory must contain the verified PR #2 native `slinkywinky_jni.dll`. Both must be supplied **outside the PR #1 repository**. Normal release and CI never fetch them or load a DLL.

## Still not complete

This establishes a **real Forge-owned GUI → PR #2 native owner-control → acknowledgement → in-game Java button label** chain. It does **not** exercise the reconstructed R211 GL11 WGL ClickGUI geometry, the C++ R210 `GuiModuleControlHandoff`, R209 native ArrayList renderer, original Mojang injected-client physical mouse input, world entry, native active=true, Sprint key/gameplay actions, or the other modules. Those remain separate integration gates. R213 is an isolated noninteractive (scripted), **no-world** title-screen acceptance and cannot justify enabling original `Mouse.next`/`Keyboard.next` hooks or claiming 1:1 reconstruction.

Keep both PRs draft and unmerged.

## Final R213 regression status

The full Chat 1 Windows x64 Release CMake build returned exit **0**. The complete CTest suite returned **102/102 PASS** in 22.21 seconds, including `r213_forge189_real_native_control_receipt_negative_controls`, native retained-pointer/WGL/GL-state tests, JNI/Attach tests and the previous GUI module-control lifecycle.

An independently launched TEMP-only Forge R212 regression without `SLINKYWINKY_R213_NATIVE_TEST` and with a freshly built default JAR also returned `BUILD SUCCESSFUL`, exit **0**, with its original one request-only click and successful vanilla Escape restoration. This confirms the opt-in bridge did not replace the previous read-only title-screen test lane.

R213's live native test returned `BUILD SUCCESSFUL`, exit **0**, across all three true JNI acknowledgement phases and a supported Forge shutdown. A separate script accepted its `latest.log` and rejected 16/16 malformed fixture receipts.
