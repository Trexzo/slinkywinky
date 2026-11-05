# SlinkyWinky — native injectable reconstruction

**Development status: native Java 8/JVMTI injection foundation. Not yet a playable Slinky client.**

The primary Windows architecture is a standalone loader executable and C++ JVMTI agent DLL attaching to an existing Minecraft 1.8.9 JVM **without Forge**. The previously developed Forge preview remains an **optional testing harness**, not a shipping dependency.

See [native injection architecture, build and explicit limitations](docs/R24_NATIVE_ATTACH.md).

The tested C++ combat decision engine, native C API, Java JNI smoke bridge, session and input ownership regressions remain available. They are not yet connected to live vanilla Minecraft client hooks or an in-game GUI. The supplied candidate decrypted DLL is a local research reference, not redistributed or verified as an official artifact; OpenSlinky+ 2.2 is excluded.

## R86 — native menu-to-profile settings bridge (still not playable)

The first-party native C++ GUI previews now connect seven **existing engine configuration fields** to menu switches: Aim Assist, Auto Block, Auto Clicker, Backtrack, Hit Select, Reach and Velocity. The remaining **58 catalog names** are explicitly unavailable, rather than falsely showing working modules. The two native preview programs share a local versioned profile at `%LOCALAPPDATA%\\SlinkyWinky\\module-profile-preview.conf`; press **P** to save and **L** to load. Profile appearance remains independent, and corrupt profile loads are rejected.

The new `native_menu_profile_binding` CTest validates mapping, visible-only writes, tab/scroll sync, persistence and rollback. **These settings are not yet connected to Minecraft gameplay or a live injected in-game ClickGUI.** The shipped injector architecture remains standalone Java 8/JVMTI/JNI, not Forge. See [R86 details](docs/R86_NATIVE_MENU_PROFILE_BINDING.md).

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

## R68 — readable native GL11 menu and desktop preview

The first-party Windows OpenGL 1.1 menu now renders its own **5×7 textureless ASCII labels** for all publicly documented module names, category tabs, heading and preview-only controls. It uses no copied Slinky fonts or graphics assets. A separate `slinkywinky_gl11_menu_preview.exe` desktop app lets authorized users examine the **actual OpenGL drawing path** without Minecraft or Java injection. This executable is included in the Windows SHA-256-verified preview artifact. All demo switches remain inert.

Regression checks include `native_menu_bitmap_font` (all-platform alphabet/catalog integrity) and the isolated Windows `isolated_wgl_menu_state_restore` test with real GL pixel readbacks proving white labels render above the dark panels without leaving GL11 state mutated. Details and controls: [R68 original GL text and visual preview](docs/R68_GL11_BITMAP_TEXT_PREVIEW.md).

**R68 is still NOT a Minecraft in-game GUI.** The original vanilla 1.8.9 renderer callback remains unverified because the authoritative desktop is offline and the user-owned Mojang client JAR could not be accessed. This is a first-party interim font and render preview, not commercial Slinky UI parity.

## R69 — bounded translucent OpenGL overlay without scene destruction

The original R67/R68 menu covered the entire viewport. R69 adds an explicit opt-in `MenuGL11Mode::BoundedOverlay`: the first-party menu is drawn into a centered, clipped, translucent panel, with no framebuffer clearing, context switching or GL viewport changes. The R67 scoped state guard restores blending, scissor, matrices and GL11 attributes. A second `StandalonePreview` mode retains the original full-canvas visual testing path. The native `slinkywinky_gl11_menu_preview.exe` now has an **`O` key** for a standalone blue-scene overlay demo; it does not attach to or launch Minecraft.

The isolated WGL CTest compares the unmodified outer-scene pixels on all four sides before/after overlay drawing and verifies state restoration, while explicitly rejecting malformed frame/mode data. See [R69 bounded OpenGL overlay and limitations](docs/R69_BOUNDED_OPENGL_OVERLAY.md).

**This is not yet an injected or validated Minecraft 1.8.9 GUI.** The real renderer callback remains unverified because the authorized desktop and original vanilla JAR are inaccessible. GL11 fixed-function preservation does not certify modern GL/LWJGL state caching. UI toggles remain inert; draft PR #1 remains unmerged.

## R70 — native WGL owner-session integration

R70 adds `WglMenuOverlayHost` as the explicit **native graphics admission and teardown bridge** between the R63 menu snapshots and R69 scene-preserving GL11 renderer. It binds one owner thread, one exact WGL HGLRC/HDC and a monotonic context epoch; refuses duplicate opens, changed contexts, lost contexts and unsynchronized viewport changes; and closes safely on its owner thread even when no graphics context remains current. Each admitted frame is freshly created from the preview-only `MenuState` and rendered solely in bounded overlay mode. The standalone GL11 preview's **O-key** mode now uses the same bridge.

The Windows WGL regression exercises a second real disposable graphics context, wrong-thread close, forced context loss, viewport mismatch and explicit reattachment, while retaining the previous pixel-compositing and GL state checks. See [R70 owner-session handoff and remaining original-game gates](docs/R70_WGL_OVERLAY_OWNER_HANDOFF.md).

**No Minecraft render hook has been installed.** The real vanilla 1.8.9 renderer class and in-world OpenGL/LWJGL compatibility still require an original JAR scan and an authorized desktop runtime. Production Java agent and gameplay paths are unchanged.

## R71 — confirmed original vanilla 1.8.9 renderer identity

An **independent, read-only GitHub Actions audit** successfully downloaded the original 1.8.9 client from Mojang, verified its published SHA-1 and length, and found the **actual shipped obfuscated renderer method**: `ave.av` calls `bfk.a(FJ)V`. It also recorded and now pins exact SHA-256 fingerprints of the original `ave.class` and `bfk.class`, so the next development stage can refuse altered or incompatible class bytes. The Mojang JAR is downloaded to a temporary runner directory, immediately discarded and **never uploaded or included in release artifacts**. See [R71 exact original-game render mapping and provenance](docs/R71_ORIGINAL_RENDERER_189_VERIFIED.md) and [successful original audit](https://github.com/Trexzo/slinkywinky/actions/runs/41424516271).

This closes the **original bytecode mapping** gap, **not** the runtime rendering gap. Live Minecraft 1.8.9 graphical callback, LWJGL context ownership and shutdown are still unverified. The production injector remains unchanged; PR #1 remains draft.
