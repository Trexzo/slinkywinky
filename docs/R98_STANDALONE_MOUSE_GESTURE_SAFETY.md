# R98 — standalone first-party mouse-down/up lifecycle admission

**Scope:** R98 adds a fail-closed C++17 mouse gesture state machine to the **separate native Windows GL11 SlinkyWinky preview**. It does **not** hook, inject mouse events into, intercept input from, or activate gameplay in Minecraft 1.8.9. R94 original-game JNI remains **visibility-only**, R95 physical/synthetic Insert admission is not yet certified, and draft PR #1 stays unmerged.

## Reason for the change

R97 aligned the actual painted compact/bounded/full menu with mouse hit regions, but the demo previously executed a local setting action on `WM_LBUTTONDOWN` alone. That could make a pressed-and-dragged or repeatedly clicked control accidentally toggle even though the user never released on it.

R98 moves action admission to the **same-target release** boundary, behind a new platform-neutral `MenuMouseGesture` in `include/slinkywinky/menu_mouse_gesture.hpp` and `src/menu_mouse_gesture.cpp`. It holds a short-lived immutable, first-party `MenuRenderFrame` snapshot (maximum eight rows) with no JVM or game pointers. It is bound to the constructing OS/UI thread. It never dispatches gameplay actions.

## Exact state transitions

- **Down:** accept only when the preview window is currently foreground, GL context/viewport are admitted, R97 returns a valid painted hit, and there is no active press. No model, tab, setting or module state changes on down. Repeated down without an intervening release **cancels** the current attempt.
- **Move while held:** continue only if focus, panel mode, viewport dimensions, source renderer epoch/sequence, tab, current visible catalog row identity, preview status, expanded state and parameter captions match the originally pressed snapshot; the pointer must still hit the **exact same row and control kind**. Leaving the target is terminal, even if it is entered again later.
- **Up:** consume the captured press and return a `MenuHit` only if the elapsed monotonic interval is 0–1500 ms, the preview is still foreground, the renderer/frame matches, and mouse-up hits the exact same control as mouse-down. Release in row gap, another button, another row, outside the panel, after timeout, after a timestamp rollback or on a changed layout is inert. A valid release yields only the R97 `Tab`, `RowBody`, `Expand` or `Toggle` classification.
- **Duplicate click:** reject the second release on the *same* target if it falls within Windows' own configurable `GetDoubleClickTime()` interval. The GUI explicitly rejects `WM_LBUTTONDBLCLK`, preventing a Windows-recognized second press even if the OS settings differ. Changing that OS timing policy while a press is held cancels the press. Different controls remain independently actionable.
- **Lost ownership:** `WM_KILLFOCUS`, `WM_ACTIVATEAPP(false)`, `WM_CAPTURECHANGED`, `WM_CANCELMODE`, size changes, mouse-wheel scrolling, keyboard navigation, graphics/window destruction or loss of WGL admission cancel pending input. The preview captures the pointer only for an admitted press; capture loss or failure cancels it. No global mouse hooks or game-context clicks are installed.
- **Dispatch:** only the separate preview Win32 `WM_LBUTTONUP` consumes a successful `MenuHit` via R96's thread-bound local `MenuNavigation` and `MenuProfileBridge`. Unsupported catalog entries are focus-only; the Unload tab and off-page/partly clipped controls remain non-clickable under R97. The 58 unimplemented catalog items cannot activate gameplay.

The state machine uses caller-provided monotonic millisecond timestamps. The actual preview calls `GetTickCount64()`; no wall-clock behavior or OS timer is required in the portable core. The C++ model itself does not import Win32 or call `SendInput`.

## Verification

`native_menu_mouse_gesture_safety` is a new portable CTest and runs through all three R97 layouts (StandalonePreview, BoundedOverlay, CompactOverlay) against first-party render sessions. It covers initial foreground refusal, no mutation on down, same-target down/up, repeated-down rejection, OS double-click threshold boundary, independent controls, dragging off and re-entering, focus loss, cancel/capture loss, long-press timeout, timestamp rollback, release on different button/panel/outside, wrong tab or viewport, re-ordered frame, changed appearance, old session sequence, subsequent valid frame sequence, local UI-only toggles, inert unsupported rows, foreign-thread refusal and timing-policy change while pressed.

The Windows GL11 preview executable builds with the new Win32 input lifecycle. R97's exact painted hit zones and independent native GL pixel/state-restoration tests remain unchanged. The final complete Windows x64 MSVC/Java8/JNI/Attach CTest count increased from **27 to 28**; check the actual final local test output before reporting overall PASS. GitHub Actions CI needs separate independent checking for the final documentation revision.

## Remaining acceptance

R98 **does not establish** that the installed Minecraft client safely accepts mouse or keyboard input. Before any in-game UI interaction, verify foreground ownership and rendering thread, explicit menu-only input capture, down/up behavior without allowing gameplay clicks through, game keybinding conflicts, loss of focus, release/stop/unload and in-world visuals on a controlled original JVM. No combat modules or Minecraft gameplay behavior were enabled by R98. PR #1 remains draft/unmerged.
