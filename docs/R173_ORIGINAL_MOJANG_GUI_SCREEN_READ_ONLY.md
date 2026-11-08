# R173 — exact original Minecraft GUI-screen presence on the render owner

**Branch:** `reconstruction/r1-core-fidelity`, draft PR #1. The separate OpenSlinky C++ reference PR #2 is not modified.

## Objective

R171 found **96** mouse/keyboard/keybind source callsites outside the vanilla `ave.s()V` input method, so redirecting that method alone cannot safely own GUI clicks. R172 added physically read-only cursor/button observation on the exact original Minecraft WGL foreground window. Before arbitration, we also need a strictly source-checked answer to a different question: **is the original game currently displaying a GuiScreen, or is it in its normal gameplay view?**

## Original Mojang 1.8.9 evidence

The exactly pinned `1.8.9.jar` (8,461,484 bytes; SHA-1 `3870888a6c3d349d3771a3e9d16c9bf5e076b908`) contains `ave.class` (SHA-256 `430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82`). Its public field `ave.m:Laxu;` contains the currently active GUI screen, or null when absent. Its public static `ave.A()Lave;` has exactly two executable instructions: `GETSTATIC ave.S:Lave;` and `ARETURN`. This is a read-only singleton getter. An independent Java 8 / ASM5 audit checks those exact instructions and the public screen field, and detects a same-instruction-count mutation changing the singleton field read. It leaves the original JAR unchanged.

## Runtime integration (read-only)

`OriginalGuiScreenProbe` admits only the already-loaded, classloader-matched obfuscated `ave` game class, verifies the source-known getter `A`, field `m` and return type `axu`, then pins the **first admitted original render callback thread** on observation. Each subsequent `observe()` returns:
- `NO_SCREEN` when `ave.A().m == null`,
- `SCREEN_PRESENT` for an `axu` instance,
- `REFUSED` on wrong thread, invalid singleton, reflection error or untrusted type.

The probe neither creates a screen, calls any GuiScreen method, alters game state, nor captures or replays any input.

`RendererFrameDiagnostic` binds the probe **only after** the existing R73 original loaded-class preflight has SHA-verified `ave` and `bfk`. It locates the unique already-loaded `ave` through `Instrumentation.getAllLoadedClasses()` and refuses other loaders or duplicates. The explicit, finite, read-only `minecraft-render-window-audit-r101` session then observes the screen state for each of its 8 admitted render callback frames, in addition to R172's foreground-only physical pointer samples. A refused sample fails the audit closed, rather than being mistaken for gameplay/no screen. Successful original-run evidence additionally prints an aggregate `R173_ORIGINAL_GUI_SCREEN_STATE_READ_ONLY_PASS open=N closed=M refused=0` with `N+M=8`. **No screen names, coordinates, mouse buttons or account data are logged.** The existing R101 format is preserved.

The synthetic Java 8 fixture constructs disposable `ave` and `axu` classes in a separate JVM and verifies open → closed, classloader refusal, wrong-thread refusal and no screen mutation. Those fake classes and fixtures are **not** included in the injectable JAR. The source audit is only in the separate render-audit JAR.

## Limits and next integration gate

This proves that the read-only GUI presence lookup has an exact original-bytecode basis and can be admitted alongside the passive WGL pointer observer. It is **not** an installed Minecraft GUI click interceptor, a guarantee of input ownership, or behavioral fidelity to the original commercial Slinky.

**Next:** R174 should use the already-source-verified `ave` GUI state and original WGL pointer data to construct a **reversible**, single-owner input arbitration plan covering renderer, GUI and gameplay consumers. Only after source coverage, actual game-thread dispatch, safe release, original-class restoration and real Minecraft 1.8.9 acceptance may pointer events be delivered to the retained native menu. Leave all 67 gameplay modules marked unfinished until their actual in-game behavior is separately verified.
