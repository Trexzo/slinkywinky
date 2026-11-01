# SlinkyWinky — native injectable reconstruction

**Development status: native Java 8/JVMTI injection foundation. Not yet a playable Slinky client.**

The primary Windows architecture is a standalone loader executable and C++ JVMTI agent DLL attaching to an existing Minecraft 1.8.9 JVM **without Forge**. The previously developed Forge preview remains an **optional testing harness**, not a shipping dependency.

See [native injection architecture, build and explicit limitations](docs/R24_NATIVE_ATTACH.md).

The tested C++ combat decision engine, native C API, Java JNI smoke bridge, session and input ownership regressions remain available. They are not yet connected to live vanilla Minecraft client hooks or an in-game GUI. The supplied candidate decrypted DLL is a local research reference, not redistributed or verified as an official artifact; OpenSlinky+ 2.2 is excluded.

## Build

On Windows x64 with a complete Java 8 JDK and MSVC:

```powershell
cmake -S . -B build -A x64 -DSLINKYWINKY_BUILD_ATTACH=ON -DSLINKYWINKY_BUILD_JNI=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The injector targets a chosen process explicitly and checks it is Minecraft. It never silently scans for and modifies arbitrary JVMs. A successful agent attach **does not yet mean a working client**; live combat, game-thread hooks, GUI, clean unload and true original parity still require implementation and verification.

## Vanilla 1.8.9 standalone runtime acceptance

R48 verified **two** consecutive read-only native attachments into an isolated **original obfuscated Minecraft 1.8.9** JVM, not a Forge development client. The standalone loader admitted class `ave`, executed zero-action C++ observations on the `Client thread`, closed the session, and restored matching bytecode fingerprints. See [R48 evidence](docs/R48_VANILLA_RUNTIME_ACCEPTANCE.md).

The existing late JVMTI breakpoint approach remains unsupported on Java 8 (`JVMTI_ERROR_NOT_AVAILABLE 98`). The working hook uses Java Instrumentation and JNI and is explicitly developer-gated. It defaults to 20 ticks; R59 adds optional, separately authorized **120- or 600-callback** read-only windows (`--hook-probe=120` / `--hook-probe=600` with `SLINKYWINKY_EXTENDED_DIAGNOSTIC=1` in the attaching process). See [R59 diagnostic bounds and acceptance](docs/R59_EXTENDED_NATIVE_OBSERVATION.md). The actual client is **not yet playable**: live world/target observation, real combat decisions, ClickGUI/HUD, full native-agent unload and verified reference parity are incomplete.

## R60 — independently verifiable world/player observation

R60 adds native-accepted frame counters (world+player, crosshair ray entity, session-generation changes) and a **manual-only** 600-tick original-vanilla-world acceptance harness. The harness refuses menu-only evidence when a world is required. Its optional `--require-ray-entity` also requires a ray-selected entity; neither observation certifies combat eligibility or visible targets. The test never synthesizes input or starts Minecraft in CI. See [R60 evidence requirements and usage](docs/R60_VANILLA_WORLD_OBSERVATION.md).

**The local Windows desktop was offline during development, so R60's in-game acceptance has not been run.** Successful CI only establishes the implementation/build/disposable JVM tests, not a playable or fully reconstructed client.

## R61 — opt-in continuous observation with explicit stop

The native Java 8 injector now also accepts `--hook-probe=continuous` behind `SLINKYWINKY_EXPERIMENTAL_HOOK=1` and `SLINKYWINKY_CONTINUOUS_OBSERVATION=1`. The separate `--stop-observation` command asks the existing Minecraft client-thread owner to close its read-only C++ observation session and restore the original class method. A fallback upper bound of **36,000 client ticks** applies; the stop command only acknowledges a request and is **not** an instant/full native DLL unload. The previous 20/120/600-tick modes remain unchanged. See [R61 lifecycle and limitations](docs/R61_CONTINUOUS_STOP.md).

The original vanilla single-player in-world R60 acceptance and full graphical/behavioral client fidelity are not yet verified. No Forge deployment dependency or proprietary reference binary is included.

## R62 — first-party native module-menu scaffold

The public [Slinky module reference](https://docs.slinky.gg/modules/visual/gui/) and [getting-started guide](https://docs.slinky.gg/basics/get-started/) inform a new C++17 menu model with the 65 publicly documented module names, category navigation, compact/wide UI, an orange first-party theme, scroll and expandable **preview-only** rows. The Windows desktop build packages `slinkywinky_menu_preview.exe` alongside the injector. GUI appearance is serialized independently of combat profiles under `%LOCALAPPDATA%\\SlinkyWinky\\menu-preview.conf` for the preview. All catalog switches are **inert demo state**, and neither profile management nor unload is exposed as a working game action. Documentation and offline verification: [R62 menu model and limits](docs/R62_NATIVE_MENU_SCAFFOLD.md).

This GDI window is a standalone design/testing reference—not a Minecraft render hook, injectable in-game ClickGUI, authenticated original artwork, or evidence that any catalog module is implemented. R60/R61 original vanilla in-world acceptance remains a separate required step.

## R63 — guarded native menu frame lifecycle

The first-party C++ renderer-neutral `MenuRenderSession` provides an explicit graphics-owner admission boundary: valid viewport, strictly increasing context epoch, immutable numbered menu frames and owner-thread-only resize/shutdown, with a synchronized cross-thread refusal path. The standalone Windows GDI menu preview now uses this adapter for `WM_SIZE`, `WM_PAINT`, and `WM_DESTROY`. See [R63 render lifecycle and integration gates](docs/R63_MENU_RENDER_LIFECYCLE.md).

**R63 still does not draw inside Minecraft**. The injector has no authenticated vanilla 1.8.9 render callback and no independently verified OpenGL state/resource restoration. R60/R61 real single-player runtime acceptance also remains outstanding. No game action, native input injection or Forge production dependency has been introduced.

## R64 — exact original vanilla 1.8.9 renderer-callsite audit

R64 adds a **non-injected**, SHA-1-gated Java 8 verifier for the original Mojang 1.8.9 client JAR. It inspects the `ave` Minecraft class via ASM, requires a renderer field actually read in the same Minecraft method, finds uniquely matching virtual `(FJ)V` calls, then checks that the target renderer class contains exactly one public, concrete method with that signature. Zero or ambiguous matches fail closed. A separate `slinkywinky_render_audit.jar` is included in the Windows preview package. The source and fixture tests are in `tests/java/dev/slinkywinky/attach/RenderCallsiteAudit*.java`; see [R64 offline procedure](docs/R64_VANILLA_RENDER_CALLSITE_AUDIT.md).

**This is an offline candidate-finding tool, not an injected render hook, in-game ClickGUI or original-Slinky fidelity claim.** It does not download or distribute Minecraft binaries. Running it against a user-owned exact original client JAR and confirming the candidate against the live game are still separate gates.

## R65 — bounded read-only render callback fixture

R65 adds a generated Java 8 render-method test fixture with an exact `(FJ)V` callback, owner-thread-only frame counting, an explicit 1–64-frame cap, per-session epoch gates, wrong-thread/invalid-frame rejection and SHA-256 preservation of original fixture class bytes. This **test-only** ASM implementation is compiled into the offline Java 8 acceptance classpath, **not** the production injected agent. See [R65 test and remaining gates](docs/R65_RENDER_CALLBACK_FIXTURE.md).

The Minecraft original-JAR verification from R64 and live game-world/render acceptance are still outstanding while the Windows desktop is offline. **No in-game GUI, GL hook, combat actions or original Slinky binary parity is claimed by these synthetic tests.**

## R66 — disposable Java 8 render lifecycle regression

R66 adds a second, independent Java 8 process to the Windows CI suite. A test-only agent exercises the synthetic renderer's eight-frame bound, preserves the original method's behavior, verifies the restored class SHA-256, and exits cleanly. The new `native_render_fixture_attach` CTest passed, bringing the Windows suite to **16/16** on R66 commit `2668a16`.

This fixture is **not Minecraft**: no original 1.8.9 renderer mapping, in-game GUI or graphics-context teardown has been verified. The test-only agent stays outside the release artifact; PR #1 remains draft.

## R67 — isolated WGL OpenGL 1.1 menu drawing and state guard

`slinkywinky_menu_gl11` now renders first-party menu panels, category accents and inert preview switches as OpenGL 1.1 quadrilaterals. It requires the exact current Win32 WGL graphics/device context, a valid menu frame and viewport, and uses a scoped fixed-function state guard to restore OpenGL server/client attributes and the modelview/projection/texture matrix stacks. The `isolated_wgl_menu_state_restore` Windows CTest creates its own throwaway WGL window, checks reject paths, compares pre/post state and requires zero GL errors after accepted draws. See [R67 GL1.1 rendering and limitations](docs/R67_GL11_STATE_GUARD.md).

**Not an in-game GUI yet.** This code is not connected to a Minecraft render hook, JNI or input, and does not fully certify modern OpenGL/LWJGL shader, framebuffer or state-cache restoration. The real original 1.8.9 render method is still unverified because the authorized Windows desktop is offline. Forge is not introduced into the production injector.
