# R174 — original Minecraft input overlap-risk classifier (no click dispatch)

**Branch:** `reconstruction/r1-core-fidelity`, draft PR #1. The OpenSlinky C++ source integration remains independently owned by draft PR #2.

## Motivation

R171 scanned all **2,507 classes** in the exact official Mojang Minecraft 1.8.9 JAR and recorded **146** direct mouse, keyboard or key-binding callsites, including **96 outside `ave.s()V`**. This proves a single-method early-return gate is not sufficient to certify exclusive input capture. R172 then added a narrowly admitted read-only cursor/button-state sample on the exact original render WGL/HWND owner; R173 verified the original `ave.A().m` GUI screen status against pinned original bytecode.

**R174 combines those two separately admitted read-only observations into a conservative *risk* classifier.** It is intentionally NOT a Minecraft input transformer, hook, event listener, ownership token or permission to deliver a ClickGUI click.

## Runtime behavior

`OriginalPointerArbitrationAudit` is a first-party Java 8 class shipped inside the agent with only a pure, synchronized `observe(screen,foreground,pointer,width,height)` entry point. It is invoked only from the existing finite, independently selected `minecraft-render-window-audit-r101` original-render callback mode after both the original game GUI screen state and the exact-window identity are admitted.

The original WGL/HWND reader uses `GetCursorPos`, `ScreenToClient` and **`GetKeyState`** (the render thread's last dequeued Windows-message state). This can miss physical transitions and is **not** a substitute for the original LWJGL2 input queue or its arbitration. The classifier receives only the 4-integer R172 pointer receipt and the source-pinned R173 GuiScreen presence, not event objects or real Minecraft gameplay state.

On the render thread, the classifier:

- Pins an exact viewport and rejects any changed viewport rather than inferring new coordinates.
- Refuses malformed receipts, foreign threads, unknown original GUI states and background-state observations that improperly include physical pointer data.
- Requires seeing every mouse button released before interpreting a left-button transition. A press already held at attachment time is quarantined.
- Labels any fresh left press observed while an original GuiScreen is present and the cursor is inside the admitted window as **`UNSAFE_PRESS`**; paired release is `UNSAFE_RELEASE`. The names explicitly mean *possible simultaneous vanilla processing*, not permission to route or suppress the press.
- Cancels pending risk candidates if focus is lost, the original screen closes, the pointer leaves the client area or another button is chorded. Reacquired focus with a held button remains quarantined until a clean release.
- Rejects all future samples in a session after a changed viewport or malformed observation.
- Provides only aggregate risk counts: candidate presses, paired releases, quarantined samples, cancelled candidates and refusals. No raw cursor coordinates, button masks, original screen identities, handles or user data are logged.

`mayDeliverMenuClick()` and `exclusiveOriginalInputOwner()` are **permanently false**. The runtime does not invoke `WglRetainedMenuPreview::pointer()`, `MouseQueueOwner.arm`, `KeyboardQueueOwner.arm`, `SendInput`, OS window hooks or any original Minecraft keybind/attack method.

On verified completion of the bounded eight-frame R101 audit, the original-runtime log additionally emits:

`R174_ORIGINAL_INPUT_OVERLAP_RISK_AUDIT_PASS candidates=N cancelled=M native_click_delivery=disabled`

This marker reports only successful *observation*, not successful input isolation or tested native menu interactivity.

## Test evidence and negatives

`R174OriginalInputOverlapSmoke` uses a disposable, Java 8-only input sample fixture with no game process, classloader instrumentation, LWJGL event queue or native input. It covers:

- Press/hold/release edge tracking without double counting and all-buttons-up admission.
- A button initially held at entry, clean rearm, gameplay/no-GuiScreen, and switching from gameplay to GUI while still holding a click.
- Foreground loss, reacquisition while held, release fencing, right/left mouse chords, outside-window presses and original GUI closure during a held click.
- Eight valid observations and fixed candidate counts; wrong-thread producer refusal, changed viewport permanent lockout and malformed receipt refusal.
- The invariant that **even a fully verified eight-frame observation cannot authorize click routing or original-input ownership**.

The summary reader is synchronized and can run on the separate post-drain cleanup thread; only `observe()` is render-thread bound. A Java 8 regression explicitly checks this separation.

The new CTest `r174_java8_original_input_overlap_risk` executes the test independently with Java 8. Original R158 receipt compilation now includes the extra production class dependency. The agent package includes only `OriginalPointerArbitrationAudit`; the test fixture stays outside both runtime and original-source-audit JARs. GitHub Actions build and execute the new CTest and preserve the independent R171/R173 exact Mojang source checks.

## Remaining steps toward genuine input ownership

The audit still **cannot know** whether a button edge was actually consumed by the original game, whether the vanilla GUI already handled it, or whether the retained menu should receive that click. It also currently observes an original GuiScreen presence, not an authenticated SlinkyWinky menu's own visibility/ownership mode. Those gaps are not hidden by the test's synthetic state.

Before any real ClickGUI click delivery, the project must establish independently:

1. Source-pinned original Minecraft GUI and gameplay input dispatch boundaries covering the renderer, GUI consumer, keybindings, polling and other paths from R171.
2. A live, reversible input handoff that prevents simultaneous vanilla/gameplay and retained-menu processing.
3. Real original game-thread mouse/key edge tests, including held input across focus and viewport changes, and actual original-bytecode and physical-key-state restoration afterward.
4. Independent in-game acceptance inside the exact original Minecraft 1.8.9 client.

**Status:** Read-only risk classification integrated in the admitted renderer, not live click ownership, gameplay execution or 1:1 Slinky. PR #1 remains draft.
