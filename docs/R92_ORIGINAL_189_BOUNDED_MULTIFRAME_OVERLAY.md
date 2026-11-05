# R92 — bounded 64-frame native GL11 menu on the original Minecraft render thread

**Scope:** opt-in source-pinned Java 8 Attach/JVMTI + JNI native Windows x64 client; exact unmodified Mojang Minecraft 1.8.9. The overlay is read-only; no combat/gameplay settings, hooks, inputs, packets, textures or third-party UI assets are enabled. PR #1 remains draft and unmerged.

## Why R92 has a separate render return hook

R91's one-frame GL11 probe executed at entry into `bfk.a(FJ)V`. A successful JNI draw receipt there cannot prove the image survives the rest of Minecraft's scene rendering. R92 instead adds an independent **at-return** injection only for the distinct `minecraft-render-overlay-r92` mode. The original-game class is still source-pinned and execution-shape admitted before transformation and source-pinned/restored after. R75/R78/R90/R91 retain their prior at-entry semantics.

The offline source-level test `OriginalRendererFrameDiagnosticAudit.checkExit` takes exact pinned Mojang `bfk.class`, independently compares every ASM method instruction against the output of the R92 weaver, and permits only three inserted callback instructions immediately before each original `RETURN`. It refuses changed methods and non-original classes; no Minecraft is launched.

## Explicit finite admission

An isolated, test-only vanilla launcher can be invoked with `--run --render-multiframe-acceptance`, in addition to the known `--workspace`, `--jdk8` and `--loader` paths. The harness checks for other Minecraft-like JVMs first; it never shuts down other processes. A fresh exclusive local stdout log and a new sanitized report are created.

The original-only supervised tool performs five *separate* Attach stages with new target-JVM stdout and release verification at each stage:

1. `--render-preflight` — exact Mojang source and runtime-class identity, then gate released.
2. `--render-callback=8` — eight source-pinned original-caller/owner thread samples, original code restored, gate released.
3. `--render-context=8` — eight stable read-only WGL samples, original code restored, gate released.
4. `--render-overlay=8` — R91 single-frame native drawing, code restored, gate released.
5. `--render-overlay=72` — **R92** eight preflight WGL frames followed by **64 one-shot native GL11 draws** on the same verified original render thread, after scene rendering; all 72 current WGL samples must be stable and unchanged; then source-pinned code restored and gate released.

The R92 mode requires explicit `SLINKYWINKY_RENDER_MULTIFRAME=1`, `SLINKYWINKY_RENDER_OVERLAY=1`, `SLINKYWINKY_RENDER_CONTEXT=1`, and `SLINKYWINKY_RENDER_CALLBACK=1`. It is never activated by the read-only R90/R78 modes. The 72-frame limit cannot be increased by user-provided arguments. The gate has a twelve-second bound and permanently stops callbacks on invalid caller, thread, viewport, context or draw result. A drawing failure cannot be reported as a PASS.

The per-frame native `slinkywinky_jni.dll` function uses the existing `WglMenuOverlayHost` and fixed-function state-restoring renderer; it creates no WGL contexts, does not call `wglMakeCurrent`, changes no gameplay settings, opens/closes the host on that same callback thread and rejects invalid GL state, context, viewport or output. The same production JNI function has independent disposable-WGL pixel/readback and state-restoration regression coverage.

## Connected Windows verification

At the exact R92 implementation on 5 November 2026:

- Native MSVC Release Windows x64 Java 8 Attach/JNI/WGL complete CTest: **25/25 PASS**.
- Original Mojang 1.8.9 bytecode audit: `R92_PINNED_ORIGINAL_RENDER_CALLBACK_EXIT_PASS`.
- Supervised Python receipt verifier: **26 tests run, 1 Windows-specific skip, 0 failures**, including forged 72-callback/64-draw receipts, wrong WGL evidence, reversed/missing/duplicate markers and five-step simulated Attach.
- **Actual isolated vanilla Minecraft 1.8.9 target JVM PID 35332:** `R92_ORIGINAL_189_MULTIFRAME_OVERLAY_RECEIPTS_PASS`. Target stdout verified `R92_RENDER_CALLBACK_BOUNDED_PASS accepted=72 owner=Client thread#1`, `R92_WGL_CONTEXT_STABLE_PASS attempts=72 current=72 absent=0 invalid=0 changed=0 stable=true frames=64`, `R92_MULTIFRAME_OVERLAY_DRAW_PASS frames=64` and `R81_ORIGINAL_RENDER_ADMISSION_RELEASED_PASS`. Report: `vanilla189-isolated/r92-multiframe-receipts-35332.json` in the local isolated build workspace. Five required stages all verified. The harness terminated its own test process and confirmed no Minecraft-like JVM remained.
- **Independent scoped real-game window capture (second isolated run, PID 8376):** The local Windows observer only selected the child JVM with the original `net.minecraft.client.main.Main`, `SWOfflineTest` and isolated game-directory process arguments. It captured 22 `PrintWindow` images of that Minecraft 1.8.9 window (870×519, including window chrome) without capturing unrelated desktop windows. `r92-pid8376-frame006.jpg` shows the normal vanilla main menu; `frame018.jpg` visibly shows the original first-party **SLINKYWINKY** native menu over that vanilla Minecraft main-menu background; `frame022.jpg` shows the vanilla main menu unobstructed again after the bounded render epoch. The second run independently passed `R92_ORIGINAL_189_MULTIFRAME_OVERLAY_RECEIPTS_PASS` with complete renderer restoration. All captures remain **local only** at `vanilla189-isolated/r92-window-evidence/`; no Minecraft copyrighted image assets are committed to the repository.
- Existing R90/R91 read-only and one-frame tests remain distinct and are not broadened by this diagnostic.

## Nonclaims

R92 now has **real original-game target-side render receipts and a separate actual Minecraft-window visual capture**, showing the menu appearing and disappearing on the vanilla **main menu**. This is not yet an in-world screenshot, quantified FPS/stutter test, interactive ClickGUI, persistent user-visible menu or selectable UI. The machine-readable R81 diagnostic report intentionally retains `visual_game_acceptance=false`, `in_game_menu_verified=false`, because that receipt pipeline does not incorporate or authenticate the independent window screenshots; they must not be silently reinterpreted as that report's booleans changing. A further opt-in bounded show/hide session with safe unload/stop control, keyboard/input isolation, in-world visibility and window-health regression is required before considering production UI activation. Keep PR #1 draft/unmerged.
