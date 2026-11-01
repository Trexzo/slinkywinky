# R68 — readable first-party OpenGL menu preview

**Date:** 2026-11-01. **Branch:** `reconstruction/r1-core-fidelity`. **PR:** #1, draft.

## What is implemented

The first-party OpenGL 1.1 adapter added in R67 previously drew only dark panels, an accent strip, category indicators and inert switches. R68 adds **original textureless ASCII letterforms** and a **separate, desktop-visible GL preview**. No original Slinky font file, Minecraft asset, decrypted DLL or proprietary vendor binary is included.

- `include/slinkywinky/menu_bitmap_font.hpp` and `src/menu_bitmap_font.cpp`: deterministic five-by-seven geometric glyphs covering ASCII uppercase letters, digits and the punctuation needed by the public 65-module catalog. Lowercase is intentionally rendered as uppercase. Unsupported glyphs become a visible question-mark placeholder. Each glyph is an array of seven five-bit rows; it uses no Windows font, filesystem or graphics API.
- `src/windows_menu_gl11.cpp`: capped-length text drawing issues GL1.1 quads for the glyphs. It renders menu title, category labels, selected-tab heading, actual public catalog module names, preview-only explanatory text, plus/minus row markers and an explicit non-gameplay footer. Glyphs are clipped to allocated horizontal display widths before geometry emission. R67's exact HGLRC/HDC/viewport gates, stack-capacity checks and fixed-function state restoration remain in force.
- `tests/test_menu_bitmap_font.cpp`: native cross-platform integrity test for the complete documented module-name catalog, category tabs, representative caption text, five-bit glyph bounds, case folding, blank spaces and unknown characters.
- `tests/test_windows_menu_gl11.cpp`: extends the isolated WGL context test. It refuses a menu frame whose title disagrees with the authenticated catalog index and uses `glReadPixels` from its own temporary WGL back buffer to distinguish actual white glyph ink from background and confirm the orange accent. The existing complete before/after GL state comparisons remain.
- `examples/windows_gl11_menu_preview.cpp`: standalone **visible native OpenGL menu window**, separately packaged as `slinkywinky_gl11_menu_preview.exe`, with the same inert category and switch controls as the earlier GDI preview. It creates and owns its own Win32 WGL context, obtains immutable menu frames from the R63 owner-thread adapter, updates the viewport on resize, presents with `SwapBuffers`, and releases its own context and resources on close. Preferences are stored under `%LOCALAPPDATA%\\SlinkyWinky\\menu-gl-preview.conf` separately from gameplay settings.
- `CMakeLists.txt`: adds `native_menu_bitmap_font` on all platforms and includes the Windows GL11 preview executable in native builds. `.github/workflows/native-attach.yml` verifies and uploads its SHA-256 with the Windows standalone preview artifact.

## Standalone preview controls

On an authorized Windows desktop, download the **R68** `slinkywinky-standalone-loader-x64-preview` workflow artifact from the successful GitHub Actions run. Run `slinkywinky_gl11_menu_preview.exe` directly (do **not** attach to any process). Keys `1`–`6` select categories; `C` changes compact mode, `W` changes wide mode, `G` changes the first-party accent; up/down arrows or mouse wheel scroll; `Esc` closes. Clicking plus/minus and demo switches changes only local UI preview bits. Profiles is a placeholder; Unload cannot be selected; combat modules, packet sending and Minecraft input are **not** connected.

## Verifiable boundaries

This change makes **native OpenGL UI labels inspectable**, but **does not hook Minecraft's original renderer**, does not render inside a Minecraft window, does not capture gameplay input and does not implement Slinky combat behavior. It does not claim exact commercial Slinky font/ClickGUI visual parity. Its 5×7 text is a deliberate first-party interim font, not copied or decrypted vendor assets.

The authorized desktop is currently offline and the official Mojang 1.8.9 client JAR cannot be downloaded in the tool environment. The R64 actual class-byte audit has consequently **not** been completed; it remains the next gate before installing any original-game render callback. R68's isolated WGL state restoration is also **not** proof of modern GL/LWJGL state-cache preservation in real Minecraft or another launcher.

Draft PR #1 stays unmerged until original 1.8.9 in-world rendering and clean shutdown are independently verified. Do not ship a commercial-client-equivalence claim based on these previews or CI-only evidence.
