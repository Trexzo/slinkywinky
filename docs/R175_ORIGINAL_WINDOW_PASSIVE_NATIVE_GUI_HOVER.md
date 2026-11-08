# R175 — passive cursor-to-tooltip rendering in the original Minecraft retained ClickGUI

**Repository:** `Trexzo/slinkywinky` · **branch:** `reconstruction/r1-core-fidelity` · draft PR #1. PR #2 independently handles the OpenSlinky C++ source reference and is not modified.

## Goal and real runtime change

Previous R171–R174 work established that original Minecraft 1.8.9 has **96 static mouse/keyboard/keybinding sites outside `ave.s()V`**, and built original-source-pinned GUI-state, exact-window Windows pointer observations and read-only overlap-risk checks. Those observations do **not** establish exclusive input ownership. Therefore, R175 does **not** enable live clicks.

The first-party **production C++** retained WGL menu now has a separately guarded **`WglRetainedMenuPreview::observe_hover()`** method, used by the existing **`RendererWglEvidence.nativeDrawRetainedPreview`** JNI entry point. After the first admitted original game frame establishes the retained menu's exact WGL owner, each later drawing frame:

1. Calls the existing `audit_original_render_pointer` check for the **exact current original Minecraft 1.8.9 render HGLRC/HDC, matching viewport and foreground HWND**. This is read-only and reuses the R172 `GetCursorPos`, `ScreenToClient` and `GetKeyState` evidence.
2. If admitted, passes **only client-pixel coordinates** to `observe_hover()`. If not admitted/backgrounded, passes `foreground_verified=false`, clearing the ephemeral hover.
3. The retained menu validates owner thread, actual WGL context/DC, matching pinned viewport, active/drawn state, and that **no synthetic click/drag gesture is pending**. It creates a current source-indexed menu-render snapshot and obtains the same `hit_test_menu` result as the original first-party GL11 ClickGUI.
4. Only `RecoveredSliderPreview` and `RecoveredRangePreview` can set a native `MenuNumericHover`; `BindChip` and `BindHold` may set the inert bind-hover row. Any other surface, focus loss or outside-window position clears the hints. `MenuState` independently validates current tab, source widget, expanded row and settings page. Repeated identical hover samples are **accepted idempotently**, rather than failing the real render loop.
5. The existing GL11 menu draw uses those inert hints for source-validated numeric tooltips and bind-hover presentation. They do **not** write `ClientSettings`, execute modules, send packets, emulate key presses, suppress original input or route any mouse-down/up events.

This is the first native retained GUI *visual mouse-position interaction* wired into the existing original-game rendering entry point. It is passive: Minecraft receives all of its normal input.

## Java 8 → test-only JNI → real C++ WGL verification

The existing isolated R166 Windows/JNI DLL gains **test-only** `R175RetainedHoverJniSmoke.observeHover` and `hoverReceipt` endpoints. The Java 8 test reuses the **exact source-validated Auto Block / Main / Range slider pixels** from R167, opens and expands the real retained native WGL menu, samples hover, and confirms the actual C++ `MenuNumericHover` is set.

The new CTest **`r175_java8_original_retained_passive_hover`** verifies:

- No hover before start or after disposal, and no wrong-thread/viewport ownership.
- A real recovered numeric slider source hit gives a retained hover, survives repeated same-location observation and an actual GL11 redraw.
- Leaving the viewport, losing foreground and restoring it correctly clears/restores hover.
- A currently held synthetic WGL pointer gesture prevents hover mutation until explicitly cancelled.
- The exact serialized native profile remains unchanged across hover, redraw and cancellation.
- The disposable test class and DLL never ship inside production Java agent, audit JAR or production JNI distribution.

## What remains unproven

The original Minecraft `minecraft-retained-preview-r99` mode must be executed against a real 1.8.9 game world to prove user-visible hover behavior at runtime. The JNI and native code are compiled and exercised in the isolated real WGL test. **That test does not itself prove the original game process was entered in this run.**

The overlay still cannot consume or exclusively own mouse clicks. The 67 gameplay modules are not automatically functional from mouse hover. Next steps require source-indexed original input arbitration and a reversible live suppression/restore protocol before the retained menu's existing click handler may be called in a game.

**Acceptance:** Windows Debug + Release CTest suites (including the full original-client Java 8 attach regressions), runtime-agent package exclusion guards, and four GitHub CI workflows. Keep PR #1 draft until independent real-game and rollback acceptance.
