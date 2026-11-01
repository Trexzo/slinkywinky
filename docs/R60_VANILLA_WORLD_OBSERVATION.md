# R60 — independently verifiable native world/player and ray-entity observations

**Date:** 2026-11-01. **PR:** #1 (draft), `reconstruction/r1-core-fidelity`. **Status:** implementation and deterministic tests committed; actual vanilla world test NOT yet executed at this revision. This is not a playable client or verified original-Slinky equivalent.

## What changed
`InstrumentationProbe.onGameTick` now accumulates *native-accepted* read-only observations on the exact Minecraft client thread, not just the state of the first tick. It emits at most one `NATIVE_WORLD_PLAYER_OBSERVED` and `NATIVE_RAY_ENTITY_OBSERVED` notification per attached session, plus exactly one terminal line:

```
NATIVE_OBSERVATION_SUMMARY accepted=<N> worldPlayer=<W> rayEntity=<R> generations=<G>
```

The numbers are only updated when Java's reflected sample passes `nativeObserveFrame` and the C++ engine returns a zero-action, ledger-accepted frame. They are reset between attachments. `worldPlayer` means both actual Minecraft world and local player objects existed at collection time; `rayEntity` means the camera ray had an `entityHit` field. **It does not establish health, visibility, reach, legitimate attacks, combat fidelity, or authenticity.** No entity names/addresses/coordinates, packets, input or secrets are printed. Counts never make game actions possible.

The R59 default 20-tick, opted-in 120-tick and opted-in 600-tick exact bounds, thread-owned close and bytecode restoration policy are unchanged.

## CI-only negative controls (no Minecraft)
`tests/attach_smoke.py` verifies both a 20- and 120-tick live Java 8 Attach/Instrumentation cycle against the disposable mapped fixture, including exact native-accepted frame counts and first world/ray markers for each session.

`tests/test_vanilla_observation_evidence.py` separately exercises the manual-validator function using synthetic log fragments; it explicitly rejects:
- Menu-only observations when a real world is required
- World observations with no ray-selected entity when ray acceptance is required
- Wrong tick counts; missing, duplicate, inconsistent, or impossible frame counts
- Missing or inconsistent class bytecode restoration and cleanup
- Native sample rejection or any hook abort / native close failure

Windows CI runs these parsing tests, compiles the manual launcher, and executes `--help`. **CI does not start Minecraft or authenticate the candidate commercial binary.**

## Actual vanilla 1.8.9 manual acceptance
R60 extends `tests/windows_vanilla189_isolated_manual.py`. It requires an explicit `--run`, independently checked original vanilla 1.8.9 client JAR SHA-256, a complete Windows x64 Java 8 JDK and a disposable workspace outside the normal `.minecraft` folder. It rejects existing attachable Minecraft JVMs, attaches only to the exact newly created PID, and never downloads, signs in or runs Forge. It uses preinstalled original game libraries and local single-player only.

Example, after building this GitHub branch on Windows:

```powershell
python tests/windows_vanilla189_isolated_manual.py --workspace "C:\slinkywinky" --jdk8 "C:\Java\jdk8" --minecraft-dir "$env:APPDATA\.minecraft" --run --world-observation
```

If you have multiple build directories, explicitly pass e.g. `--loader "C:\slinkywinky\build-inject\Release\slinkywinky_loader.exe"`. The loader path must be inside `--workspace` and have the expected executable name.

The console pauses for the operator to create or enter a local single-player world. After the operator confirms it is loaded, R60 starts one **opted-in 600-callback** native read-only session. Acceptance requires all terminal markers, 600 C++-accepted callbacks, at least one `worldPlayer` frame, successful native close, one exact restored class fingerprint, a live Minecraft process after hook cleanup and no error signatures. A menu-only client therefore fails even if it attached cleanly.

Add `--require-ray-entity` only when an entity can be placed under the crosshair during the diagnostic window; that requires at least one `rayEntity` frame. No aimbot, input synthesis, world control or automated target selection is enabled.

```powershell
python tests/windows_vanilla189_isolated_manual.py --workspace "C:\slinkywinky" --jdk8 "C:\Java\jdk8" --run --world-observation --require-ray-entity
```

Without `--world-observation`, the original R49/R59 menu-level **two successive 20-tick** check remains available. The command never silently invokes world tests; only the user-started, interactive `--run --world-observation` path launches and checks an in-world scene.

## Remaining gates
1. Execute the R60 interactive manual acceptance against an installed original vanilla 1.8.9 client on a connected authorized Windows desktop. Preserve the full log and SHA-256; don't mark runtime PASS on CI alone.
2. Validate actual camera/eye/world/target samples over world transitions, while ensuring no game actions.
3. Extend to opt-in, safely stoppable persistent lifetime only after explicit shutdown and repeated bytecode restoration tests.
4. Build the first-party native ClickGUI/HUD with separately reviewed visuals and settings; then test one first-party combat module at a time.
5. 1.7.10, other launchers, native DLL full unload, and equivalence to official Slinky remain separate unverified work.

No proprietary Slinky DLL, decrypted image, game JAR or extracted commercial resource was committed.