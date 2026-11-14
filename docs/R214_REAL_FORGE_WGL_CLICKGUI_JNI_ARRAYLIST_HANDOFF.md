# R214 — Actual Chat 1 GL11 ClickGUI renders in Forge 1.8.9 and routes canonical Sprint requests through real PR #2 JNI

**Date:** 2026-11-14
**Owner:** Chat 1 / draft PR #1 (`reconstruction/r1-core-fidelity`). **Chat 2 / draft PR #2 source left unchanged.**
**Scope:** isolated opt-in original Minecraft 1.8.9 **ForgeDev test client**, not the standalone injector. This is a no-world, non-gameplay acceptance.

## Integration architecture

R213 already provided a live Minecraft-owned `GuiScreen` and a first-party Java-to-real-PR2 JNI native module-controller round trip. However Chat 1's real C++ retained GL11 renderer, R210 source-focused C++ control handoff, and R209 active-only ArrayList remained independent and were not being used in the same game process.

R214 supplies a **distinct test-only native DLL** called `slinkywinky_r214_gui_overlay.dll`, compiled from `src/r214_forge_gui_overlay_jni.cpp` and linked to **Chat 1's unchanged first-party `slinkywinky_menu_gl11` and `slinkywinky_core`**. It deliberately does **not** replace the separate PR #2 `slinkywinky_jni.dll`, which provides `SprintObservationProbe.setEnabled/state`. The DLL is excluded unless CMake `BUILD_TESTING=ON` and `SLINKYWINKY_BUILD_JNI=ON`; no original injector inputs/hooks are enabled.

- The admitted ForgeDev screen, on the actual Minecraft client/render thread, creates a copy-only initial native `ModuleArrayListSnapshot` with source sequence 1, world generation 0, `Sprint requested=false active=false`, **derived from a real initial PR2 state acknowledgement**. Native state is not guessed from `MenuState.preview_enabled`.
- On each `GuiScreen.drawScreen`, the JNI bridge obtains the actual current WGL context/HDC and validates that the OpenGL viewport matches `Minecraft.displayWidth/displayHeight`. `WglRetainedMenuPreview::draw` paints the **actual reconstructed C++ OpenGL 1.1 compact menu**, navigates the source-owned focus to Sprint in the Move tab, scrolls that row into view, and subsequently renders it across multiple real client frames. The native `WglMenuOverlayHost::draw_active_modules` draws the R209 active-only ArrayList feed after the retained menu. The source is not a synthetic HUD enabled flag.
- A scripted click on an ordinary Minecraft-owned **Java GuiButton** invokes R214's native `queue_focused_runtime_toggle` on the last-painted GL11 menu, then `GuiModuleControlHandoff.take_on_module_owner()`. The actual source snapshot sequence and generation are returned to the same admitted Java owner thread. **This is NOT proof of a physical click on the reconstructed GL11 switch or source hit pixel.** The underlying Java `GuiButton` is the input target.
- Java then calls the **real PR2 JNI** `SprintObservationProbe.setEnabled(enable)` and reads back the native `ControlState`. Only after those checks does `R214NativeMenuOverlay.acknowledge` publish the deep-copy state using `GuiModuleControlHandoff.acknowledge_on_module_owner`, which advances the C++ ArrayList feed. The GUI owner and module owner are the **same verified Forge client thread in this test**, not separate threads; original injector ownership is not claimed.
- On Escape the test tears down the R214 retained WGL model, R210 handoff and R209 feed, closes PR2's JNI owner, restores the identical original vanilla title-screen object, and requests a supported Forge `FMLCommonHandler.exitJava(0,false)` on the isolated client.
- R214's Java wrapper class is `experimental/forge_gui189/src/main/java/dev/slinkywinky/forgegui/R214NativeMenuOverlay.java`. Native and Java methods refuse invalid sequence, missing WGL/viewport, wrong thread, unsupported name, incomplete session and malformed active/requested state.

## Actual live Windows test

- Windows 11 / Java 8 Temurin / Forge 11.15.1.2318 / Minecraft 1.8.9 / local TEMP-only dev game, account `R212GuiOffline`.
- `slinkywinky_r214_gui_overlay.dll`: Windows Release MSVC build exit 0; links the Chat 1 actual GL11 menu, not an independent mock renderer.
- ForgeGradle Java 8 `compileJava`: PASS.
- Minecraft `runClient`: **BUILD SUCCESSFUL**, Gradle exit **0**. Exact observed sequence:
  - `R214_NATIVE_GL11_SOURCE_INITIAL_OFF_PASS`
  - `R212_REAL_FORGE_GUI_SCREEN_INSTALLED`
  - `R214_REAL_GL11_MENU_TO_JNI_TO_ARRAYLIST_FEED_PASS id=1 requested=true active=false activeRows=0 frames=7`
  - `R214_REAL_GL11_MENU_TO_JNI_TO_ARRAYLIST_FEED_PASS id=2 requested=false active=false activeRows=0 frames=10`
  - `R214_REAL_GL11_MENU_TO_JNI_TO_ARRAYLIST_FEED_PASS id=3 requested=true active=false activeRows=0 frames=13`
  - Matching real `R213_REAL_JNI_NATIVE_SPRINT_ACK` for each ID, no-world generation 0.
  - `R214_NATIVE_GL11_GUI_AND_FEED_CLOSED_PASS`, `R213_REAL_JNI_SPRINT_OWNER_CLOSE_PASS`, vanilla Escape restoration, supported clean Forge exit.
- Offline `tests/r214_forge_gl11_native_bridge_receipt.py`: accepted the real exact game log, **20 negative controls PASS**, guarding incorrect order, wrong active state, absent/duplicate ack, missing source paint progress and missing unload restoration.
- The PR #2 Java wrapper used was separately staged from PR #2 head `8ce56a2` (the source was never copied into tracked PR #1). The native JNI artifact used was PR #2's previously built **R210 Release DLL**, SHA-256 `B8607387DC8D337DAD980607B2FE7D1E80A9878B3DE06DDAB6F931626E8AE281`. This proves the shared stable Sprint JNI control API, **not** all PR #2 latest code.

## Reproduction (strictly isolated; no live client modifications)

```powershell
$env:SLINKYWINKY_R214_GL11_TEST = '1'
$env:SLINKYWINKY_R213_PR2_JAVA_SOURCE = Join-Path $env:TEMP 'slinkywinky-r213-pr2-java-only'
$env:SLINKYWINKY_R213_PR2_JNI_DIR = "$env:LOCALAPPDATA\SlinkyWinky\R210-JNI-Integration-Build\Release"
$env:SLINKYWINKY_R214_GL11_JNI_DIR = 'C:\Users\Felix\Desktop\Slinky-R1-Workspace\SlinkyWinky-R206-Build\Release'
$env:SLINKYWINKY_R212_GUI_GAME_DIR = Join-Path $env:TEMP 'slinkywinky-r212-gui-r214-gl11'
# With Java 8/Gradle 2.14.1, inside experimental/forge_gui189:
gradle --offline --no-daemon runClient
```

Explicit verified PR #2 Java source and both native DLLs must exist at those paths; builds without R214 opt-in must remain independent. The test never patches the personal Minecraft directory, existing user accounts, original Mojang JAR, or external Minecraft instances.

## Remaining important gaps

1. **Java test button vs C++ pixel:** Requests presently originate from a Minecraft-owned Java GuiButton, **not** a physical mouse click on the C++-painted source-exact Sprint switch. The rendered native menu's preview-toggle artwork does **not** yet reflect actual native requested ON/OFF state; authoritative state currently updates the Java button and feeds the active-only native HUD.
2. **No world:** Sprint `requested=true` is never equated to `active_in_world=true`; ArrayList must remain empty in this test. An in-world producer with real module subscriptions and an authoritative generation transition is not yet exercised.
3. **Original injector:** No proof of real mouse/keyboard owner, native screen lifecycle, or source-exact rendered mouse hit in an injected unmodified Minecraft 1.8.9 JVM. No original LWJGL event queue is replaced.
4. **Incomplete reconstruction:** Other modules, appearance parity, actual gameplay effects and 1:1 original Slinky behavior remain unverified.

Keep both PRs draft/unmerged.

## Final regression and packaging audit

The full Windows x64 Release CMake build completed with exit **0**, including the new opt-in `slinkywinky_r214_gui_overlay.dll` target. The subsequent full CTest run returned **103/103 PASS**, total 27.63 seconds, including `r214_forge189_wgl_native_gui_receipt_negative_controls`, native retained GL11/WGL state tests, R209 ArrayList, R210 control handoff, Java JNI and the original attach acceptance.

`gradle --offline --no-daemon clean jar` without R213/R214 environment toggles returned `BUILD SUCCESSFUL` and exit **0**. The default Forge JAR contains the inert R214 wrapper class but **does not bundle** `dev/slinkywinky/SprintObservationProbe.class` or any native DLL, confirming this remains a separately admitted dev test mode.

This R214 milestone is a **live actual-gl-render/copy-only-control-plane acceptance**, not a demonstration that the on-screen C++ toggle can receive physical clicks or that a real active module produces an ArrayList row in a world.
