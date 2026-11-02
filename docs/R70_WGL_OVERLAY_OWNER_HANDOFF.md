# R70 — WGL overlay owner-session handoff

**Status:** first-party Windows rendering-host integration in draft PR #1. CI and actual game acceptance are separate. This is **not** a Minecraft render hook.

## Problem addressed

R69 proved a bounded, scene-preserving GL1.1 panel in an isolated context. It still left the actual renderer entry point as a low-level `draw_menu_gl11(frame, HGLRC, HDC)`: callers could supply a stale, already generated menu frame or invoke a renderer without being admitted as the owner of a particular graphics-context generation.

R70 adds `WglMenuOverlayHost` (`include/slinkywinky/windows_menu_overlay_host.hpp`, `src/windows_menu_overlay_host.cpp`), a single explicit boundary connecting the R63 `MenuRenderSession` and R69 GL renderer. It is ready to receive a future independently verified game render callback **but is not installed into any game**.

## Implemented behavior

- `open(context, device, width, height)` requires the exact current WGL context and DC, an existing viewport that matches the supplied dimensions, a valid R63 render session viewport and a fresh monotonically increasing internal context epoch. A second open while active is rejected without changing the session.
- `draw(menu)` checks the owner thread and exact live WGL HGLRC/DC before obtaining a **new immutable menu frame** and calling `draw_menu_gl11` in `MenuGL11Mode::BoundedOverlay`. It cannot request full-canvas painting or act on gameplay modules. A different graphics context, lost current context or off-thread call fails closed.
- `resize(width,height)` requires a preexisting active session, exact current HGLRC/DC, owner thread and an OpenGL viewport already resized to the requested dimensions; silent viewport changes or guessed surface sizes are denied.
- `close()` requires the owner thread but deliberately makes **no OpenGL calls**. It can revoke the session after WGL context loss; only an explicit subsequent `open` with a new internal epoch can admit rendering again. A second close fails.
- The host does not create or delete graphics resources, locate target processes, load DLLs, change LWJGL, call JNI, install Java retransformation, load fonts, or access networking.

The standalone `slinkywinky_gl11_menu_preview.exe` now uses this exact host for its **O-key** bounded overlay demonstration. The full-window first-party preview remains on the existing standalone path; switching O off closes the bounded host. On resize, it admits the new WGL viewport before any new menu frame. Window destruction closes the host before the window destroys its own graphics context. No production Minecraft injector source is changed.

## Actual isolated Windows graphics regression

The Windows `isolated_wgl_menu_state_restore` CTest now exercises the full WGL host:

1. Refuse drawing or closing an unopened host, and refuse null graphics context/DC or a mismatched initial viewport.
2. Admit a verified current WGL context and frame, render a bounded menu with GL state restoration.
3. Reject an unsynchronized GL viewport resize; accept a matching explicit resize; reject another silent size change.
4. Create **a second, distinct real WGL context** for the same disposable test window. The existing host rejects it; an unrelated thread cannot close the host.
5. Close the old session even though it no longer owns the current WGL context, then explicitly admit the second context, draw, and restore its own state.
6. Revoke the WGL current context, verify drawing is refused, close safely, and restore the original graphics context. A fresh admission succeeds, then explicit teardown clears the host.
7. Verify the normal GL1.1 state restoration and R69 scene-preserving pixel checks still pass.

These checks use their own Windows process/window and do not read, start, attach to, or change Minecraft.

## Explicit external acceptance gates

- The Windows Remote Desktop Commander device was **offline** during R70 development.
- Mojang's original 1.8.9 `1.8.9.jar` was not present in the authorized runtime, so the R64 SHA-1-gated renderer-callsite audit remains unrun on the original bytes. A method inferred from public MCP names is not enough to install a production hook.
- This host guards the **native WGL** lifecycle but is not an assurance of LWJGL Java caches, FBO/shader/texture multi-unit state preservation or compatibility with other launchers.
- The native OpenGL panel, though more integration-ready, is still a **standalone UI**. There is no verified in-world GUI, live render callback, gameplay module wiring or original Slinky binary fidelity.

The next implementation gate is a version-exact local vanilla 1.8.9 JAR audit, followed by a gated read-only render-thread test, then deliberate native bridge integration after GL and LWJGL state verification. PR #1 stays draft and unmerged.
