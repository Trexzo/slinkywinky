# R63 — guarded native menu render lifecycle

**Status:** implementation and deterministic tests committed to draft PR #1; CI and actual in-game acceptance must be reported separately. This is a renderer-neutral *admission layer*, not an in-game Minecraft render hook.

## Why this stage is necessary

R62 introduced a first-party, standalone Windows GDI menu preview. The production Java 8 injector currently hooks only the verified game-tick callback; the repo does not yet have an independently verified Minecraft 1.8.9 **render** callback or a safely admitted GL graphics context. Guessing another obfuscated method name could destabilize the game. R63 therefore makes the rendering boundary explicit before attempting injected drawing.

External context: Minecraft 1.8.9 methods may have different Notch-obfuscated, SRG and MCP names. Those names require version-exact and classloader-exact verification; a 1.8.8 mapping alone is not sufficient for 1.8.9. See https://forums.minecraftforge.net/topic/36050-how-to-get-method-and-variable-names-for-obfuscationreflectionhelper/ .

## Implemented

- `include/slinkywinky/menu_render.hpp` and `src/menu_render.cpp`: a portable C++17 **MenuRenderSession**. Only a single owner thread can attach, resize, generate a frame or detach. Viewports smaller than 320×240, larger than 8192×8192, zero epoch, repeated attach, and previously used context epochs are refused. A synchronization mutex protects concurrent calls.
- `next(MenuState)` copies an immutable **MenuRenderFrame** carrying the current epoch, incrementing frame sequence, validated viewport, selected tab, display preferences and at most eight public catalog entries. The adapter never retains pointers to game objects, JVM classes, graphics objects or input state.
- `tests/test_menu_render.cpp`: negative controls for missing session, bad viewport, stale epochs, off-thread frame/resize/teardown, snapshot independence from later UI modifications, 7-row Block and placeholder Profiles layouts, and valid new-context reattachment.
- `examples/windows_menu_preview.cpp`: GDI `WM_SIZE` events admit/update the owned session, `WM_PAINT` obtains a validated snapshot and draws only if viewport and context match, and `WM_DESTROY` calls owner-thread detach. No synthetic Minecraft render callbacks are claimed. User-facing controls remain preview-only/inert.
- `CMakeLists.txt`: `native_menu_render_lifecycle` CTest wired into portable core, Linux/Windows, Debug/Release and sanitizer matrix.

## Verified / unverified boundaries

This stage proves an **architectural graphics-lifecycle gate in native code** and its operation in the Windows standalone preview build when CI passes. It does **not** establish an actual OpenGL state restoration path, a Minecraft render callback, Java/GL resource ownership, a working in-game menu, or a downloadable authenticated original Slinky client. Graphics contexts and menus rendered by Minecraft are not touched in R63. No proprietary artwork, font files or decrypted binaries are redistributed.

## R64 implementation gates

1. Obtain a **version-exact** vanilla Minecraft 1.8.9 rendering function signature from a trusted mapping/verified local game binary. Do not assume a 1.8.8 obfuscated method is the same.
2. Before any UI draw operation, implement an isolated **read-only** callback count test and guarantee no duplicated hook, native action, target pointer, or classloader leak.
3. Validate render-context owner and GL state backup/restore; teardown must be idempotent and fail closed on GL context loss, minimizing, disconnection and process exit.
4. Only then connect the existing first-party menu model to actual renderer-owned draw commands and separately guarded input routing.
5. Original vanilla in-world R60/R61 acceptance, actual visual comparison and 1.7.10 / Lunar / Badlion environments remain separate tests, not inferred from CI.

No Forge dependency was added to the production injector. The reference archive remains uncommitted.
