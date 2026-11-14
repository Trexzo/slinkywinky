# R206 — Stable Windows build and fail-closed original-input evidence classification

**Repository/branch:** `Trexzo/slinkywinky` • `reconstruction/r1-core-fidelity` • draft PR #1. Independent C++ OpenSlinky PR #2 is untouched.

## Problem found after R205

The previous 96-test Windows x64 Release build was located under `AppData\Local\Temp\SlinkyWinky-R172-Chat\build-r184-clean`. Microsoft Build Tools repeatedly warned `MSB8029` that placing the intermediate/output directory beneath Temp can cause incremental-build trouble. Multiple R19x–R20x runs observed stale object behavior until a clean rebuild.

R205's isolated original Minecraft 1.8.9 read-only run also reported **51 foreground samples out of 63**, compared with the earlier runs' zero. But its exact source hover hits, numeric controls and bind hits were all zero, and its original-JVM JSON report explicitly said:
- `scope="R81 original vanilla render receipts only"`
- `visual_game_acceptance=false`
- `in_game_menu_verified=false`

Focus alone does **not** prove that physical mouse/keyboard events were received, exclusively owned or restored. An exact geometry hover, if observed later, would still be read-only cursor evidence, not a delivered click.

## Stable source-isolated full Windows build

Configured a fresh out-of-Temp build tree:

`C:\Users\Felix\Desktop\Slinky-R1-Workspace\SlinkyWinky-R206-Build`

The existing Git checkout remains in its prior path. No source was duplicated or deleted. The build directory is intentionally distinct from the source checkout.

A first configuration unintentionally retained the project defaults `SLINKYWINKY_BUILD_JNI=OFF` and `SLINKYWINKY_BUILD_ATTACH=OFF`; the 54/54-pass test subset therefore **was not** equivalent to the earlier 96-test run. R206 explicitly corrected that:
- `SLINKYWINKY_BUILD_JNI=ON`
- `SLINKYWINKY_BUILD_ATTACH=ON`
- `BUILD_TESTING=ON`
- Java 8 JDK `C:\Program Files\Eclipse Adoptium\jdk-8.0.504.1-hotspot`
- The pinned ASM 5.2 JAR was copied only from the previously verified build after SHA-256 `3e5ea0d7da2c5155ef4f470d9092d42de34e3f53db6589c7c07d6721adf4ba3e` was checked for both source and destination. The CMake integrity check still enforces the same hash.

The complete corrected build registers **97 CTests**: all former 96 plus one R206 original-input scope negative-control test. Do not report 54/54 as the complete suite.

## New first-party R206 acceptance classifier

Added `tests/r206_original_input_readiness.py` and `tests/test_r206_original_input_readiness.py`, and connected them to `tests/windows_vanilla189_isolated_manual.py` plus CMake.

The classifier accepts **only** a restored R99 read-only original-JVM report with the exact expected six renderer stages, one unambiguous retained-menu receipt, `visual_game_acceptance=false`, and `in_game_menu_verified=false`. It cross-checks original foreground/hover/source-draw counters against separately verified exact OS focus counters, preserving sample conservation and refusing forged, duplicated, reordered or contradictory reports. Boolean-forged counters are rejected as invalid integer observations.

Its result always distinguishes:
- `original_render_receipts=True` (restored renderer observation)
- `foreground_observed` and integer sample count (OS/window focus)
- `source_hover_observed` and `exact_source_hover_samples` (read-only cursor/last-painted target geometry only; two or more samples required)
- `original_click_delivery="disabled"`
- `input_exclusivity_certified=False`
- `interactive_gui_verified=False`
- `gameplay_modules_verified=False`

Even an intentionally valid synthetic source hover and focus cycle cannot promote to exclusive input. No GUI event interception, JVMTI gameplay binding, OS pointer hooks or original-input suppression is introduced.

The original vanilla acceptance prints `R206_ORIGINAL_PASSIVE_INPUT_SCOPE_PASS foreground=... exact_hover=... source_hover=... click=disabled gui_interactive=false` only after the existing authentic/verified original render receipts and these additional admission checks succeed.

`tests/test_r206_original_input_readiness.py` provides synthetic read-only negative controls for original 0/63 focus, observed 51/63 focus with no hover, hypothetical repeated source hover, and approximately 30 contradictory or forged variants, including duplicate R99 receipts, wrong source draw counts, stage order, invalid scope, report promotion and invalid OS focus count. It never launches Minecraft or sends any input.

## Verification

- The synthetic R206 Python negative-control test passed locally on Windows, including tampered original-readiness scopes, contradictory foreground counters, invalid source draw counts, duplicated R99 restored receipts and attempts to promote read-only evidence to interactive/gameplay claims.
- The stable out-of-Temp x64 MSVC Release build was configured with both optional integration modes explicitly **ON** and JDK 8. The full build completed successfully, and **all 97/97 Windows CTests passed** (including Java/JNI, native agent attach and R206's new evidence test). The previous 54/54 subset was deliberately discarded as an incomplete configuration.
- **A separate original vanilla Minecraft 1.8.9 read-only acceptance passed with process exit 0**, using the new stable loader located **inside** `Desktop\Slinky-R1-Workspace`. It did not weaken the loader/workspace containment rule. The restored exact original-JVM receipt printed `R206_ORIGINAL_PASSIVE_INPUT_SCOPE_PASS foreground=0 exact_hover=0 source_hover=false click=disabled gui_interactive=false`, and the standard R99 retained renderer acceptance passed. There were 60 source-visible slider draws and 63 passive hover samples, all with no foreground hover. The earlier R205 foreground=51 sample count was from a different run and is not evidence of sustained foreground input.
- The original acceptance JSON still reports `visual_game_acceptance=false` and `in_game_menu_verified=false`. Even a later successful supervised R181 source-hover test would remain read-only cursor evidence, not physical click/hardware ownership.
- Real hardware input remains unavailable/unverified. R206 must **not** be treated as authorization to enable live mouse/keyboard hooks or as a module gameplay reconstruction.

## Reproduce

```powershell
$p='C:\Users\Felix\AppData\Local\Temp\SlinkyWinky-R172-Chat'
$build='C:\Users\Felix\Desktop\Slinky-R1-Workspace\SlinkyWinky-R206-Build'
$env:JAVA_HOME='C:\Program Files\Eclipse Adoptium\jdk-8.0.504.1-hotspot'
cmake -S $p -B $build -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON -DSLINKYWINKY_BUILD_JNI=ON -DSLINKYWINKY_BUILD_ATTACH=ON
cmake --build $build --config Release --parallel 8
ctest --test-dir $build -C Release -j 6 --output-on-failure
```

The source checkout still resides under Temp; eventually migrate the checkout deliberately, after reconciling any other active sessions, without deleting unrelated local files or duplicating the full build.
