# R216 — Native GL11 Sprint switch tracks actual PR #2 JNI acknowledgement, not preview flags

**Date:** 2026-11-14
**Owner:** Chat 1 / `reconstruction/r1-core-fidelity`, draft PR #1.
**Cross-branch:** Chat 2's draft PR #2 sources remain untouched.

## What R216 closes

R215 proved source-pinned paired mouse events could initiate an authoritative Sprint toggle request in an isolated ForgeDev Minecraft 1.8.9 client. Its C++ retained GL11 switch was still painted from the **inert MenuProfileBridge preview**, however; real native requested ON/OFF was displayed only in a separate Forge status line.

R216 changes only the **test-only** `slinkywinky_r214_gui_overlay.dll` painting path. After successfully painting the first-party C++ retained GL11 menu, its WGL owner verifies an exactly sequenced `ModuleArrayListFeed` snapshot with one canonical `Sprint` row. It makes a **deep presentation-frame copy** of the last source-pinned, successfully painted `MenuRenderFrame`. In that copy alone, the displayed Sprint `preview_enabled` field is replaced by the actual native manager's **JNI-acknowledged** `requested_enabled` value from the current R209 feed. The real first-party `draw_menu_gl11` redraws the reconstructed compact ClickGUI from that copy on Minecraft's same WGL context/DC and viewport. No `MenuState`, native profile, tooltip, gameplay module, keybind or underlying input geometry is changed. The R210 request handoff still checks the original source frame.

The C++ bridge reads one OpenGL framebuffer pixel at the **center track** of the precise source Sprint switch (between either thumb position). A new JNI `nativeVisual` receipt returns the acknowledged source sequence, its ON/OFF state, exact sampled RGB, GL11 frame count and the underlying `WglRetainedMenuPreview.preview_enabled(25)` flag. Java refuses a missing/incoherent receipt, changed preview flag, stale frame or a color outside a narrow tolerance of the original reference palette. The app's separate status readout still reports requested and active independently. Requested ON is never misrepresented as active in-world.

## Actual live acceptance (2026-11-14)

Exact dependencies: Windows 11 x64, Temurin Java 8, Forge 11.15.1.2318 / Minecraft 1.8.9, named TEMP-only ForgeDev game directory `%TEMP%\slinkywinky-r212-gui-r216-visual`, no player/world, dummy offline identity `R212GuiOffline`. PR #2's `SprintObservationProbe.java` was staged separately and backed by the previously compiled verified PR #2 R210 `slinkywinky_jni.dll`. This still verifies the stable native Sprint control ABI, not all of PR #2's newest native behaviors.

Actual source-client log reports **four consecutive pixel-validated native render states**:

| Native source sequence | Confirmed Sprint requested | Exact framebuffer RGB | Native preview flag | GL11 painted frame |
|---|---|---|---|---|
| 1 | OFF | `404040` | false | 7 |
| 2 | ON | `F35D12` | false | 37 |
| 3 | OFF | `404040` | false | 72 |
| 4 | ON | `F35D12` | false | 106 |

The three accepted source-exact paired mouse clicks proceeded through R215, the real PR #2 JNI manager and Chat 1's R210/209 owner/feed, with `active_in_world=false` and zero active ArrayList rows. Invalid orphan/outside-target/mismatched-release events were rejected. The original title-screen object was restored, both native owner sessions closed and Forge `FMLCommonHandler.exitJava(0,false)` completed. **Gradle runClient BUILD SUCCESSFUL, exit 0.**

`tests/r216_forge_authoritative_switch_pixels.py` independently validated the real Forge client's live `latest.log`, plus **24/24 negative controls** covering wrong source/ack order, forged RGB, incorrect ON/OFF sequence, changed local preview flag, stale painted frames, duplicate pixels, absent cleanup and invalid identity/version.

## Reproduction

Use the same explicit R215 scripted input setup (the exact two independently staged DLLs and PR #2 Java source must be present), changing the game directory:

```powershell
$env:SLINKYWINKY_R215_SOURCE_MOUSE_TEST = '1'
$env:SLINKYWINKY_R213_PR2_JAVA_SOURCE = Join-Path $env:TEMP 'slinkywinky-r213-pr2-java-only'
$env:SLINKYWINKY_R213_PR2_JNI_DIR = "$env:LOCALAPPDATA\SlinkyWinky\R210-JNI-Integration-Build\Release"
$env:SLINKYWINKY_R214_GL11_JNI_DIR = 'C:\Users\Felix\Desktop\Slinky-R1-Workspace\SlinkyWinky-R206-Build\Release'
$env:SLINKYWINKY_R212_GUI_GAME_DIR = Join-Path $env:TEMP 'slinkywinky-r212-gui-r216-visual'
# Inside experimental/forge_gui189 using Java 8 and locally cached Gradle 2.14.1:
gradle --offline --no-daemon runClient
```

`python tests/r216_forge_authoritative_switch_pixels.py --selftest` runs the no-game negative controls; `--log ...\logs\latest.log` validates a real isolated run.

## Important limitations

- This visual render bridge is **test-only**, not part of the original injected Minecraft client's active UI. The visual frame-copy technique currently **draws the compact ClickGUI twice per frame**, retaining a separate safety boundary instead of altering the production preview model. Production performance optimization and exact 1:1 visual parity remain future work.
- Automated click events are **scripted calls through vanilla `GuiScreen.mouseClicked`/`mouseReleased`** using first-party source-painted coordinates. **No physical mouse/foreground-loss/manual test has been passed.**
- The title screen contains **no player or world**; active module state remains false. Actual in-world Sprint activation, nonempty ArrayList, other module actions and full original standalone injectable integration are still unverified.
- Never replace original LWJGL mouse/keyboard queues, introduce global keybinds or merge PRs based on these isolated results.

Keep PR #1 and PR #2 draft and unmerged.

## Final regression and packaging status

The full Windows x64 CMake Release build returned exit **0**, including the modified dedicated R214/R216 GL11 test-only JNI bridge. The full CTest regression completed **105/105 PASS** in 22.51 seconds, including the new `r216_forge189_authoritative_switch_pixel_negative_controls`, JNI/attach, GL11 and input-safety tests.

A separately launched live **R214 backward-compatible Java-button → GL11 → PR2 JNI → C++ feed** acceptance also passed, including three Sprint requested ON/OFF/ON acknowledgements, native cleanup, vanilla title-screen restoration and Forge `BUILD SUCCESSFUL`, process exit **0**.

Finally, `gradle --offline --no-daemon clean jar` with all R212–R216 opt-in variables absent passed with exit **0**. The default JAR contains dormant Java bridge classes, but **does not contain** `dev/slinkywinky/SprintObservationProbe.class` or any native DLL. Only the separately admitted local dev test can load either native library.

The authoritative-gl11 frame copy purposely incurs a **second menu draw per frame**, which is acceptable for this bounded integration proof but needs one-pass optimization before any production client claims.
