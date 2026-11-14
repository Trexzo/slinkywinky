# R212 — Real Minecraft 1.8.9 GuiScreen input ownership experiment

**Owner:** Chat 1 (GUI, input and shell), draft PR #1 `reconstruction/r1-core-fidelity`. **Separate from** Chat 2 / PR #2 module actions. This is an independent ForgeDev acceptance harness, NOT a production injector input hook.

## Why this route

The verified original Mojang 1.8.9 `ave.s()V` is not its sole input consumer. R171 and R194 found 146 direct Mouse/Keyboard/keybinding callsites, including 96 outside `ave.s()V`. Replacing only `Mouse.next()` or `Keyboard.next()` could leave vanilla pre-queue, held-keybinding and other consumers active. R211 therefore remained a synthetic first-party WGL gesture. R212 tests a different approach: use Minecraft's **own `GuiScreen` as the input owner** instead of overriding the game's keyboard/mouse queues.

## Implementation (isolated)

`experimental/forge_gui189/` is a separate ForgeGradle 2.1 Java 8, Minecraft 1.8.9 / Forge 11.15.1.2318 project with no dependency on Chat 2's source tree. Its only mod, `R212ForgeGuiInputProof`, has no native gameplay/JNI hooks.

- Both modes are disabled by default. An explicit environment variable plus the exact temporary game directory prefix `slinkywinky-r212-gui-`, offline test username `R212GuiOffline`, no world and no player are required.
- In automatic mode `SLINKYWINKY_R212_GUI_AUTO_TEST=1`, the client installs its own `GuiScreen` from the legitimate Forge client-tick owner thread on the vanilla main menu using `Minecraft.displayGuiScreen`.
- The fixture invokes `GuiScreen.mouseClicked` with the **center of the real `GuiButton`** from that owner thread. Vanilla `GuiScreen` then calls `actionPerformed` once. This is an **in-process scripted click**, **not** a hardware mouse click. It creates a canonical `Sprint` **request-only** counter `id=1 requested=true active=false`; no module is armed, no native manager is called and no ArrayList item is painted.
- The fixture invokes the same screen's `keyTyped(KEY_ESCAPE)` and verifies the prior `GuiMainMenu` object is restored. This Escape is also scripted through the real `GuiScreen` handler, not a claimed physical keyboard event.
- After the proof, only the uniquely named disposable test client's JVM calls Forge's supported `FMLCommonHandler.exitJava(0,false)`; neither other Minecraft processes nor system input settings are changed.
- An independent manual mode `SLINKYWINKY_R212_GUI_MANUAL_TEST=1` opens the same owned test GUI and permits real clicks/Escape **for future hands-on testing**, without scripted actions or auto-shutdown. **Physical manual input was not acceptance-tested in R212.** The old globally registered F8 keybinding was deliberately removed.

The normal mod launch is inert. No live original Mojang JAR is rewritten, no `Mouse.next` / `Keyboard.next` callsite is replaced, and no injected original-client window or LWJGL event queue is intercepted.

## Exact verified Windows acceptance (November 14, 2026)

- Windows 11, Java 8 Temurin 1.8.0_504, ForgeGradle 2.1, Forge 11.15.1.2318, Minecraft 1.8.9; isolated game directory under the Windows TEMP tree.
- `gradle --offline --no-daemon compileJava`: PASS.
- `gradle --offline --no-daemon runClient` in **auto-test mode**: Forge detected four mods, including `slinkyguiinputproof`. It logged all ordered receipts:
  - `R212_REAL_FORGE_GUI_SCREEN_INSTALLED`
  - `[R212GuiProof] request-only canonical=Sprint id=1 requested=true active=false`
  - `R212_REAL_FORGE_GUI_BUTTON_ACTION_REQUESTED`
  - `R212_REAL_FORGE_GUI_ESC_RESTORES_PREVIOUS_SCREEN`
  - `R212_FORGE189_OWN_GUIScreen_MOUSE_AND_ESCAPE_PASS`
  - `R212_ISOLATED_CLIENT_CLEAN_SHUTDOWN`
  - `FMLCommonHandler.exitJava` accepted code 0
- Final complete Gradle process exit **0**, `BUILD SUCCESSFUL`. The splash-screen's transient "Crash Report" debug-info print is normal Forge output. An earlier attempted `Minecraft.shutdownMinecraftApplet()` triggered Forge's `ExitTrappedException` and was **replaced and retested** with the correct Forge exit mechanism.
- `tests/r212_forge_gui_input_receipt.py --selftest`: **12/12 negative controls** PASS (missing/duplicated/out-of-order click, wrong active flag, version and unsafe exit).
- `tests/r212_forge_gui_input_receipt.py --log <TEMP>/slinkywinky-r212-gui-admission/logs/latest.log`: confirmed ordered real-client receipts.

### Reproduce only in a dedicated temporary test directory

From `experimental/forge_gui189` on Windows with Java 8 / Gradle 2.14.1 and offline ForgeGradle caches available:

```powershell
$env:SLINKYWINKY_R212_GUI_AUTO_TEST = '1'
$env:SLINKYWINKY_R212_GUI_GAME_DIR = Join-Path $env:TEMP 'slinkywinky-r212-gui-admission'
gradle --offline --no-daemon runClient
```

For interactive manual testing, use `SLINKYWINKY_R212_GUI_MANUAL_TEST=1` **instead of** `AUTO_TEST=1`. The Gradle build supplies the required child JVM properties and offline identity; do not launch into a real user's Minecraft game directory.

## Next actual integration gap

A native reconstructed GL11 ClickGUI does **not** become a Minecraft `GuiScreen` merely because the isolated Forge proof passed. Likewise a `GuiButton` action here does not call R210's C++ `GuiModuleControlHandoff` or Chat 2's admitted Sprint manager. The remaining tasks are to establish the actual injected Java client screen/lifecycle owner, relay canonical commands onto the real module owner, publish genuine acknowledged module state to R209's ArrayList, and verify physical input, focus loss and close/unload restoration in the original Minecraft client. Keep both PRs draft until then.

## Final Chat 1 regression

After adding the optional receipt-parser CTest and excluding nested ForgeGradle caches from Git, the complete existing Windows x64 Release build passed (exit 0), followed by **101/101 CTest PASS** (22.26 seconds, exit 0). The new `r212_forge189_gui_receipt_negative_controls` is CTest #15 and passed. Existing native retained WGL pointer, GL-state, JNI attach and original read-only acceptance tests remain green.

The final live Forge auto-test also ran **without externally setting JAVA_TOOL_OPTIONS**. Its Gradle `runClient` task supplied typed child JVM properties and verified temp offline username, produced all receipts, used `FMLCommonHandler.exitJava(0,false)`, exited **0**, and reported `BUILD SUCCESSFUL`. The only gameplay-adjacent action remains a GUI request value with `active=false`.

Manual physical clicking/keyboard acceptance and integration with the actual JNI WGL reconstructed menu / Chat 2 native manager remain explicitly **unverified**.
