# R172 — original-window physical pointer observation (no input ownership)

**Repository:** `Trexzo/slinkywinky` · `reconstruction/r1-core-fidelity` · draft PR #1. PR #2 independently owns the OpenSlinky reference integration and is not modified.

## Motivation

R171's exact Mojang 1.8.9 scan found **146** Mouse, Keyboard, or key-binding invocation sites across **2,507** classes, including **96** outside `ave.s()V` and **86** direct LWJGL mouse/keyboard calls outside that method. A whole-method early return or `Keyboard.next()` queue replacement therefore cannot be certified as exclusive GUI input ownership. Before any click is delivered to the retained menu, we need to establish where the real Windows pointer is relative to the exact original game window and how its physical buttons are represented at the admitted rendering point.

## Implemented read-only path

`include/slinkywinky/windows_original_pointer_audit.hpp` adds a narrow **foreground-only** `audit_original_render_pointer`. It calls the existing `audit_game_render_window` gate first, which requires the current WGL context and HDC, matching client size and GL viewport, the exact original `Minecraft 1.8.9` window title, single same-process visible root and the window's **original render thread**. If that gate returns anything other than `ValidForeground`, observation is denied.

Only then, on the rendering thread, it reads `GetCursorPos`, converts to the authenticated HWND's client pixels with `ScreenToClient`, and reads the **high state bit only** from `GetKeyState` for five mouse-button states as of the render thread's last retrieved Windows messages. It reports `{x, y, inside_client, buttons_down_mask}` to its caller; the mask range is 0–31. Outside-window coordinates are retained as evidence of being outside, never clamped to a plausible clickable control. The source never calls `GetMessage`, `PeekMessage`, `SetCursorPos`, `SetCapture`, `SendInput`, OS hooks, LWJGL input manipulation or first-party click dispatch.

`src/jni_bridge.cpp` exposes this owner-checked observation as `RendererWglEvidence.nativeObservePhysicalPointer` returning a 4-int array; **no pointer value is logged**. The existing explicit, read-only `minecraft-render-window-audit-r101` renderer option now obtains the observation only for admitted foreground samples. Java validates the coordinates and inside/outside classification, physical button mask and receipt shape, accumulates counters, and fails closed on malformed/refused evidence. Background samples still receive only identity evidence and never read physical pointer data. `windowIdentityVerified()` additionally requires one accepted observation for each foreground render sample; the existing acceptance receipt format remains stable.

This does not authorize clicking or hide actual mouse state from Minecraft. No call to `WglRetainedMenuPreview::pointer()` or an OS/LWJGL hook has been added.

## Validation

The native original-WGL fixture tests strict right/bottom-excluding client coordinates, denial before an exact original-window title is admitted, correct owner-bound foreground readout when Windows grants focus, refusal while background, viewport/context mismatch, ambiguous same-title windows and foreign OS threads. It independently invokes the JNI entry point with a missing JVM environment and requires refusal. No test moves the user's cursor or sends a click. The original `isolated_wgl_menu_state_restore` test also checks that GL state is unchanged.

Build and CTest include the Java 8 JNI agent suite; the new native method is compiled into the existing JNI DLL. The Java 8 input-owner guards from R119 remain in place and no live input interceptors are enabled.

## Limitations and acceptance gate

This is a **real OS read of the exact admitted game HWND within its render callback path**, once that runtime mode is invoked; on its own it is not an authenticated **exclusive** input owner. A disposable WGL fixture with the same title can validate the native geometry/foreground checks, but is not proof of the real game process. The original Mojang class SHA and live render admission remain separate prerequisites. Importantly, vanilla still receives the pointer, including within original GUI and renderer code paths discovered in R171.

Before connecting live clicks to native retained controls, R173+ must prove an original-client GUI-active state, source-indexed input arbitration across all relevant consumers, no gameplay double-dispatch, physical-release/foreground-loss handling, and verified **live class and input-state restoration**. Until then, keep the mouse-event routing disabled and PR #1 draft. This milestone does not implement gameplay modules or prove original Slinky equivalence.
