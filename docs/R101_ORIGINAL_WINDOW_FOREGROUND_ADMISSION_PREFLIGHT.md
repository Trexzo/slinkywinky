# R101 — WGL-bound game-window identity and foreground preflight

**Status:** native production Windows code and isolated fixture tests implemented on draft PR #1. The first R101 batch is a **read-only input-admission prerequisite**, not an in-game keyboard/mouse hook or a certified interactive ClickGUI. R100's previously verified external menu navigation and original renderer restoration remain unchanged.

## Why R100's PID scope is insufficient for keyboard/mouse input

R100 can receive a command from an external process and let the source-pinned render thread change *only* inert menu-preview state. That is not the same as safely capturing a physical key or mouse click. A PID or title alone does not prove that a foreground Minecraft window owns the current OpenGL framebuffer. A title-matched secondary window, minimized window, stale graphics HDC, lost focus, or mismatched client dimensions could cause a key/click to be interpreted by another UI. Minecraft/LWJGL also has its own input plumbing, and a Win32 title/foreground check alone does not demonstrate that events will not reach underlying gameplay.

## Implemented

- New `include/slinkywinky/windows_game_window_gate.hpp` defines `audit_game_render_window(HGLRC,HDC,unsigned,unsigned)` and a three-state `GameWindowFocus`: **Refused** (`-1`), **ValidBackground** (`0`), **ValidForeground** (`1`).
- Requires **exact current WGL HGLRC and HDC**, a real `WindowFromDC` HWND that is its own `GA_ROOT`, the root HWND owned by the **current process and current render thread**, visible, enabled, nonminimized, and the exact vanilla title **Minecraft 1.8.9**. The **client rectangle and live GL_VIEWPORT** must exactly match the admitted physical dimensions (320–8192 wide, 240–8192 high). An extra same-title visible top-level window inside that PID is ambiguous and refused.
- `GetForegroundWindow()==root` is required for the **ValidForeground** status. A valid exact original render HWND that is not currently foreground is reported only as **ValidBackground**, never silently treated as focused.
- The code does not call `SetForegroundWindow`, `SendInput`, `GetAsyncKeyState`, `SetWindowLongPtr`, `SetWindowsHookEx`, or any event-consumption API. It does not modify gameplay, Java state, keybinds, or overlay navigation.
- The production `slinkywinky_jni.dll` now exports `RendererWglEvidence.nativeAuditRenderWindow` and the Java 8 diagnostic model supports an **exclusive** 8-frame native window-identity audit configuration. This configuration is not yet promoted into the user-facing original-Minecraft launcher or a live gameplay admission. The preflight only collects foreground/background/refusal counts, never actual keys or mouse data.

## Verification

- The existing real-WGL `isolated_wgl_menu_state_restore` CTest gained **native and actual exported JNI agreement checks** for exact context/DC and owner-bound identity.
- Negative controls refuse an arbitrary titled WGL fixture, a wrong viewport or absent context, a duplicate visible same-title HWND in the same PID, a call from another thread, a hidden HWND, and a changed window title.
- A constructed, visible, unique test window titled `Minecraft 1.8.9` is classified as either `ValidForeground` or `ValidBackground` according to **actual OS focus**, without ever forcing foreground.
- Earlier R100 producer ACK/stop and the rest of R99/WGL GL-state restore tests still run. This fixture **does not claim that an arbitrary title-matched synthetic window is genuine Minecraft**: source provenance must separately be admitted by the R90 original Mojang 1.8.9 SHA gate.
- Local MSVC x64 / Java 8 build and `isolated_wgl_menu_state_restore`: PASS. Full CTest and GitHub CI on the final document commit should be checked separately.

## Limitations and required next gate

This result establishes a **conservative foreground/graphics owner classification**, not a modal input consumer. It has not yet been admitted against a fresh original vanilla JVM window with independent foreground-state evidence. The user-facing R101 original-game mode is intentionally **not enabled** in the launcher. A future optional, original-source-pinned game audit can establish real HWND/window-title/class/client-rect compatibility without sending input. Before admitting physical Insert, arrow keys or mouse, a genuinely modal **no-click-through** policy must be proven under LWJGL, with capture transitions, released keys, focus loss, resize and unload, and no accidental gameplay actions. Fail closed whenever that cannot be proven. No combat or movement module is activated. PR #1 remains **draft/unmerged**.
