# R179 — exact source-backed Auto Block slider hit geometry in original vanilla Minecraft 1.8.9

**Source branch:** `reconstruction/r1-core-fidelity` · **draft PR #1** (unmerged). OpenSlinky C++ reference PR #2 remains separate and untouched.

## Purpose

R176 proved the original Minecraft 1.8.9 client called the passive hover observer for 63 native WGL samples, but all were background samples. R177 pixel-tested the tooltip in a disposable C++ OpenGL context. R178 kept the recovered Auto Block numeric slider on screen for 60 of the original 64 GUI frames. **R178's schedule alone did not independently prove that the actual native renderer's numeric track occupied a valid, clickable client-pixel rectangle for those 60 frames.**

R179 makes that distinction explicit, without enabling Minecraft input capture.

## First-party source target

New `menu_source_hover_target.hpp` provides one narrow, reusable **read-only source target**: the unique `Auto Block → Main → Range` recovered Slider from the candidate decrypted reference catalog.

For a given first-party `MenuRenderFrame`, it requires:

- Original MenuTab::Combat; unique source module and widget identities with the correct widget kind.
- Exact frame viewport dimensions and source-validated, expanded settings page.
- The same `menu_panel_geometry`, `menu_numeric_track_geometry` and `hit_test_menu` functions as the first-party C++ GL11 renderer.
- An entirely client-contained pixel rectangle, nonzero dimensions, and a center point that the **actual native hit tester** identifies as the same source widget.

It returns the read-only physical **client** rectangle and center for the own-process WGL owner; no global screen coordinates, OS mouse state, key code, gameplay packets or input actions are involved. Invalid tab, closed module, changed source page, missing widget or clipped viewport returns no target.

## Actual native retained renderer instrumentation

`WglRetainedMenuPreview` now uses its own render-owner source frame and independently counts:

1. `source_visible_draws` — a native GL11 draw completed while that exact source-backed slider had valid client-pixel/hit-test geometry.
2. `source_exact_hover_hits` — accepted foreground physical hover observations for **that** source-identified rectangle (a subset of R176 numeric hit samples).

The existing R175 generic hover behavior remains unchanged. The native C++ target and evidence accessors require the same owner thread, WGL HGLRC/HDC and unchanged viewport as the retained first-party GL11 renderer. The new production JNI method returns exactly three counter values, `[frames,source_visible_draws,source_exact_hover_hits]`. No cursor coordinates or button state cross JNI or appear in logs.

R179 extends the Java 8 R99 renderer acceptance: on the last original-game frame, the old R176 five-count hover receipt and the new R179 three-count native source receipt must independently agree:

- `frames = 64`
- `source_visible_draws = 60`
- `0 <= source_exact_hover_hits <= foreground_numeric_hits`
- The original 64-frame WGL, source class SHA restoration and JNI owner release must all pass.

An independent loader-side checker requires the exact original-JVM marker and reconciles its source hits against the earlier R176 foreground and numeric counts. Duplicated, malformed, forged, wrong-count, contradicting foreground and click-enabled markers are all denied.

## Java8/JNI/WGL tests

The disposable `R179ExactSourceTargetJniSmoke` uses the test-only, separately packaged R166 JNI DLL and the same first-party native renderer. It verifies:

- No target before opening the recovered numeric settings; then a **real native source rectangle** matching an independently computed R167 slider pixel.
- The actual C++ GL11 source-available count rises on successful retained draws; never on a wrong tab.
- Source-exact hover hit counters increase on admitted foreground slider hover, and **not** on lost focus or hover outside the target.
- Wrong Java thread or viewport cannot read or change source geometry; the serialized game-independent native settings do not change.
- No geometry/readback remains after disposable WGL disposal.

`R179OriginalSourceTargetReceiptSmoke` checks Java 8 counter shape and negative controls, including invalid source-visible frame count and source hits unsupported by R176 numeric/foreground evidence. `test_r179_original_native_target.py` validates exact original-JVM log lines independently. All three are registered with CTest and protected by packaging and production-input guards; the Java 8 JNI fixture remains isolated from the distributed agent.

## Original vanilla Minecraft acceptance actually run

A **fresh isolated original Mojang 1.8.9** Java 8 client was launched twice with the original client SHA, class-load identity, native WGL rendering and restoration harness. The second run included the strict R179 loader-side verifier and completed:

`R179_ORIGINAL_NATIVE_SOURCE_TARGET_PASS frames=64 source_draws=60 exact_hits=0 native_click_delivery=disabled`

`R179_ORIGINAL_VANILLA_NATIVE_SOURCE_TARGET_PASS source_draws=60 exact_hits=0 foreground=0`

The prior native R176/R178 counters and R99 restoration receipts matched exactly. The original client was closed afterward. This is **real first-party C++ geometry available in 60 actual original Minecraft 1.8.9 GL11 frames**, not a fabricated Java schedule.

**Limit:** The isolated original client was backgrounded for all observations (`foreground=0`), so **no real foreground physical cursor hit or visible original-game tooltip is proved**. The R177 GL11 pixel proof remains limited to its separate disposable WGL test. This milestone also does not suppress Minecraft input, send clicks or enable any of the 67 gameplay modules. Keep PR #1 draft until independently tested reversible game input ownership and restoration.
