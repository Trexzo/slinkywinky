# R215 — Source-exact reconstructed GL11 toggle receives Minecraft-owned paired mouse events

**Date:** 2026-11-14
**Owner:** Chat 1 / `reconstruction/r1-core-fidelity`, draft PR #1.
**Cross-branch:** Chat 2's native module controller remains in PR #2 and was used only through an independently supplied Java/JNI Release artifact. No tracked PR #2 file edited.

## Scope and precise claim

R214 rendered the real native C++ reconstructed compact ClickGUI on the actual WGL owner of a disposable ForgeDev Minecraft 1.8.9 client, but the Sprint requests still originated from an ordinary Java `GuiButton`. R215 adds actual GL11 source-toggled mouse routing: **Minecraft 1.8.9's own `GuiScreen.mouseClicked` / `mouseReleased` callbacks** deliver a paired event to the **native painted Sprint toggle**, which validates the identical source hit at both edges and then sends one R210 `GuiModuleControlRequest` through Chat 2's real JNI `SprintObservationProbe.setEnabled/state` to the R209 active-only ArrayList feed.

This is a **live in-game integration of actual GUI mouse-dispatch methods and native source-painted hit geometry**, without a direct LWJGL queue rewrite, HWND hook, window focus theft or original-client injection. In automated acceptance, mouse handler calls are **scripted in-process** using a native-provided coordinate from the painted toggle. It is **not physical hardware mouse input evidence**, even though a separately opt-in manual mode supports actual `GuiScreen` mouse events. No manual/hardware clicks were tested in R215.

## Implementation

- `src/r214_forge_gui_overlay_jni.cpp`: Adds `nativeTogglePoint` (test-only painted-coordinate inspection), `nativeMouseDown`, `nativeMouseUp`, `nativeMouseCancel`. The source target is checked by Chat 1's existing `hit_test_menu` over the **actual last successful GL11 `MenuRenderFrame`**, exact canonical catalogue index 25 (`Sprint`, Move tab) and non-wide compact viewport.
- Native `MenuMouseGesture` admits left-button Down on the exact toggle, then refuses an orphan, unsupported state, foreign owner, different WGL context/DC, changed source sequence, generation regression, lost painted geometry or released click outside the original source target. It allows successful repaints while the underlying frame remains equivalent and preserves R98's original double-click guard. It never calls `profile_.toggle` or writes gameplay state.
- The source-backed R210 `queue_focused_runtime_toggle` only executes **after** the proven paired Up on the same catalog 25 source target. `GuiModuleControlHandoff.take_on_module_owner` returns a copied request to Minecraft's exact client owner thread.
- `experimental/forge_gui189/R214NativeMenuOverlay.java`: Handles the native paired mouse ABI, source point fixture and source-copy request semantics. The separate PR2 bridge remains the real JNI module manager, never a local mock.
- `R212ForgeGuiInputProof.ProbeScreen`: On the admitted `GuiScreen`, it maps vanilla scaled mouse coordinates into raw framebuffer pixels, routes left-button Down/Up to native and only calls the existing real JNI bridge after native releases a valid source request. The old Java Sprint `GuiButton` is hidden in R215 mode, preventing duplicate toggles; the native painted Sprint row is the only supported source for new module requests. A visible text line displays the actual **JNI-acknowledged** requested/active state separately; it does **not** falsely mutate `MenuState.preview_enabled` or imply a native visual switch parity that has not been implemented.
- `onGuiClosed` cancels pending native source gestures. The existing auto mode closes the native WGL menu, handoff, feed and PR2 JNI owner after restoring the previous vanilla title screen; the separately enabled manual mode closes the same resources when the screen closes. In manual mode, foreground loss observed by LWJGL `Display.isActive` cancels an in-progress native mouse gesture. This is not proof of restoration of any original Minecraft injector hook, because none is installed.
- Strict opt-in: `SLINKYWINKY_R215_SOURCE_MOUSE_TEST=1` for scripted automatic acceptance, or `SLINKYWINKY_R215_MANUAL_MOUSE=1` for a separately admitted interactive manual test (never both). Both require the same explicit temporary Forge 1.8.9 game dir, dummy offline identity, separately staged verified PR2 `SprintObservationProbe.java`, PR2's `slinkywinky_jni.dll`, and Chat 1's `slinkywinky_r214_gui_overlay.dll`.

## Live Windows proof

The R215 compiled Windows native DLL and Java 8 ForgeGradle project used the same installed Forge 11.15.1.2318 Minecraft 1.8.9 on Windows 11. The disposable game directory was `%TEMP%\slinkywinky-r212-gui-r215-source-mouse`; offline identity was `R212GuiOffline`. The game had no world or player.

The actual isolated game run emitted:
- `R215_SOURCE_MOUSE_NEGATIVE_EDGES_REFUSED_PASS` from orphan-release, outside-target-down and mismatched-up in the Minecraft-owned mouse callback path, **without dispatching a request**.
- `R215_EXACT_SOURCE_PAIRED_MOUSE_ACK_PASS id=1 requested=true active=false activeRows=0 frames=7`
- `R215_EXACT_SOURCE_PAIRED_MOUSE_ACK_PASS id=2 requested=false active=false activeRows=0 frames=38`
- `R215_EXACT_SOURCE_PAIRED_MOUSE_ACK_PASS id=3 requested=true active=false activeRows=0 frames=73`
- For each ID the old R214 `REAL_GL11_MENU_TO_JNI_TO_ARRAYLIST_FEED_PASS` and **actual PR2** R213 `REAL_JNI_NATIVE_SPRINT_ACK` also appeared in the correct order. The no-world native ArrayList stayed at **0 active rows**, as required.
- Native GL11 owner/feed close, PR2 native JNI owner close, exact previous vanilla title-screen restoration, `FMLCommonHandler.exitJava(0,false)`, and full ForgeGradle `BUILD SUCCESSFUL`, exit **0**.

### Failure discovered and corrected

The first full R215 live attempt correctly admitted the first click and rejected the second closely spaced click due to R98's native double-click suppression window. That early run ended with an explicit test assertion (not a native crash). The automated test was then amended to separate scripted clicks by sufficient client ticks to honor the existing input-debounce policy. No native click safety gate was weakened. The replacement complete live run passed all three toggles and exited 0.

`tests/r215_forge_source_mouse_receipt.py --log <real Forge log>` verified the exact accepted live session. Its independent **21 deliberate negative controls** validate version and identity, existence and order of the source mouse receipts, absence of bogus active status, monotonic painted frames, exact native request IDs, JNI acknowledgement order, and cleanup.

## What is and is not solved

R215 **does** connect an actual painted GL11 Sprint source hit, paired Minecraft-owned GUI mouse callbacks, source-focused R210 C++ requests and genuine PR2 JNI owner acknowledgements. This is a substantive step past R214's Java-button-led source-focused request.

R215 **does not** yet prove:
- physical human/OS mouse delivery to the C++ control (the automated test calls `GuiScreen.mouseClicked` and `mouseReleased` internally);
- the actual original injected Mojang JVM's exclusive mouse/keyboard ownership or focus recovery;
- correct ON/OFF painting in the **GL11 switch artwork** (the requested state is shown in a nearby acknowledged status readout; the ClickGUI's preview toggle is intentionally not used as a native module signal);
- a native `active_in_world=true` module or any nonempty active ArrayList in a real world;
- production packaging, full module functionality, or 1:1 Slinky parity.

The new code is exclusive to the explicitly opt-in local test build, with existing automatic R214/R213/R212 tests preserved. Both PRs remain draft and unmerged.

## Final rebuild, backward-compatibility and regression results

After the acknowledged-state readout was changed to display exact PR2 `requested=true/false` and `active=false` in a visible bottom-of-screen text line, R215 was independently re-run from the source with explicit opt-in. The final live ForgeDev run again logged all three paired source mouse approvals and the same negative-edge refusal, with real PR2 JNI acknowledgements, native feed lifecycle cleanup, supported Forge exit, **Gradle BUILD SUCCESSFUL, process exit 0**. The independent R215 live-log receipt validator and **21/21 negative controls** passed.

An independent **R214 legacy path** regression was run with R215 disabled. Its original Java-button → native C++ source-focused request → PR2 JNI → C++ feed path passed all three states, cleanup and `BUILD SUCCESSFUL` / exit 0.

Normal `gradle --offline --no-daemon clean jar` without any R215/R214/R213 opt-in also passed, exit 0. Inspection confirmed the default JAR includes dormant Java bridge class code but **does not bundle** `SprintObservationProbe.class` from PR #2 and **does not contain any DLL**.

The complete Windows x64 CMake Release build returned exit 0. The full CTest regression completed **104/104 PASS**, 22.72 seconds, including the new `r215_forge189_source_mouse_pair_receipt_negative_controls` and all inherited native WGL, GL11, JNI, Java attach, renderer, menu and input-safety tests.

This acceptance does not involve a human moving or clicking a physical mouse. The manual mode is available only in the named disposable TEMP ForgeDev game and is **not yet physically acceptance-tested**.
