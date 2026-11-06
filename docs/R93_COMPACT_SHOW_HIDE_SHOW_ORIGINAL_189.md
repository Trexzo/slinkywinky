# R93 — compact GL11 show / hide / show verified inside original vanilla Minecraft 1.8.9

**Status: local native/JNI/Java8 tests PASS; original-game supervised six-stage receipts PASS; independent process-scoped real-game window capture confirms compact menu shown, hidden and shown again.** This is a finite automatic visibility acceptance, **not** a user-controlled hotkey/interactive ClickGUI, Forge mod or production release. PR #1 remains draft/unmerged.

## First-party implementation

- `MenuGL11Mode::CompactOverlay` is a distinct native mode that reuses the R91/R92 WGL owner/context/DC and fixed-function state-restoration protections. At the captured 870×519 Windows Minecraft window, it uses a centered **480×420 logical viewport-space panel**, about 70% internal typography/geometry scale. Original `StandalonePreview` and `BoundedOverlay` remain unchanged; the preexisting C++ `WglMenuOverlayHost::draw(menu,profile)` ABI is preserved alongside a new explicitly typed overload.
- `slinkywinky_jni.dll` provides `nativeDrawCompactOverlay` by calling the same validated WGL current-context/viewport/state-snapshot/restore routine as R91's original native menu. Neither one creates a context, changes Minecraft state or enters an input hook.
- The isolated `--render-visibility=72` original-only Java8 Attach option requires `SLINKYWINKY_RENDER_VISIBILITY=1`, `SLINKYWINKY_RENDER_OVERLAY=1`, `SLINKYWINKY_RENDER_CONTEXT=1` and `SLINKYWINKY_RENDER_CALLBACK=1`. The R93 supervised six-stage checker also explicitly authorizes the prior R92 multiframe stage independently with `SLINKYWINKY_RENDER_MULTIFRAME=1`. An absent flag rejects the specific Attach, without falling back to a different mode.
- R93 uses the already original-source-pinned **render-RETURN** callback from R92. It has exactly **72 admissible calls**: first eight WGL/owner/thread preflight frames; then **20 compact visible**, **16 hidden with zero native draw calls**, and **28 compact visible** frames. All 72 must retain the same current WGL context/DC and viewport. End-of-session release is contingent on successful source-pinned original class restoration and observer/drain verification. The sole successful R93 target receipt is `R93_MENU_SHOW_HIDE_SHOW_PASS shown=48 hidden=16`.
- This phase is intentionally scripted with no keyboard hook, game mouse control, gameplay activation or user-input handling. The next separate acceptance boundary is an explicit stop/unload and user-driven show/hide control with input isolation.

## Verification, local Windows desktop

- Full MSVC Windows x64 native / Java8 Attach / JNI / isolated WGL CTest: **25/25 PASS**.
- R81/R89/R90/R91/R92/R93 Python target-receipt tests: **28 tests run, 1 platform-specific skip, zero failures**, including an entire six-stage mocked acceptance, source-JAR pinning, and rejection of forged/duplicated/out-of-order hidden/visible receipts.
- Actual production JNI compact drawing on the independent disposable WGL fixture: PASS with GL pixel readback, cross-thread refusal, wrong context/viewport refusal, full GL state restoration. The fixture also redraws the background on every frame and reproduces **48 shown / 16 hidden** frames. No Java game is started in those tests.
- Exact pinned original Mojang 1.8.9 bytecode audit: `R75_PINNED_ORIGINAL_RENDER_CALLBACK_PREFIX_PASS` and `R92_PINNED_ORIGINAL_RENDER_CALLBACK_EXIT_PASS` PASS. The R93 branch only runs the same audited at-RETURN hook with an alternate show/hide-only sampler.

### Supervised original vanilla 1.8.9 acceptance

- Early attempt #1 refused at R92 because the newly introduced R93 six-stage operator script omitted R92's **own** explicit multiframe flag. This was corrected in the supervised checker and negative controls; **the R92 gate was not weakened**.
- A later attempt refused before Attach because another unrelated Forge/dev Minecraft-like process was running. The harness exited without touching it.
- **Final successful isolated original vanilla JVM PID `14620`:** all **six** original-source/PID-birth/WGL/renderer restoration/receipt stages PASS. Target-JVM stdout confirms `R93_RENDER_CALLBACK_BOUNDED_PASS accepted=72 owner=Client thread#1`, `R93_WGL_CONTEXT_STABLE_PASS attempts=72 current=72 absent=0 invalid=0 changed=0 stable=true shown=48 hidden=16`, `R93_MENU_SHOW_HIDE_SHOW_PASS shown=48 hidden=16` and `R81_ORIGINAL_RENDER_ADMISSION_RELEASED_PASS`. The local sanitized receipt file is `vanilla189-isolated/r93-visibility-receipts-14620.json`.
- **Independent original game window capture (PID-scoped, offline Minecraft `SWOfflineTest` only)** stored locally under `vanilla189-isolated/r93-window-evidence/`: `r93-pid14620-frame030.jpg` shows the **compact** native menu over Minecraft 1.8.9's genuine main menu, `frame033.jpg` shows original Minecraft **without** the Slinky menu, `frame036.jpg` shows it **reappearing**, and `frame040.jpg` shows normal Minecraft after the bounded drawing session. Additional fixed-pixel sampling of the locally captured 41 frames independently detected the compact orange top accent on frames 30–32 and 36–39 but not on frames 33–35 and 40–41. Captures are intentionally **not committed** to Git, avoiding any Mojang copyrighted game imagery in the repository.
- The isolated R93 JVM terminated through its own harness after the verified receipts, and no Minecraft-like JVM remained. Other user-owned Java programs were not terminated or injected.

## Limits and next acceptance

The screenshots establish that a compact, noninteractive first-party menu **appeared, disappeared, reappeared and disappeared** within the genuine original game's main-menu window. They do **not** prove in-world UI stability, mouse/keyboard control, FPS impact, configurable module functionality or production-ready gameplay hooks. The sanitized R81 receipt JSON deliberately retains `visual_game_acceptance=false` and `in_game_menu_verified=false` because the automated receipt checker does not itself authenticate these separate local screenshots. Do not silently turn those fields true.

Keep PR #1 draft/unmerged. For R94, define user-controlled show/hide and stop/unload only after separately verifying keyboard focus/input routing, no game-input collisions, fail-closed WGL shutdown, and a supervised original-game visual regression.
