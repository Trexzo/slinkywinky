# R74 — disposable obfuscated render-callback lifecycle

**Scope:** cross-process Java 8 Attach + test-only instrumentation of generated classes with the **same original vanilla 1.8.9 owner/method names** (`ave.av → bfk.a(FJ)V`). These are local synthetic fixtures, **not** original Mojang classfiles or an injected in-game ClickGUI.

## Why this exists
R73 proves the strict live-class preflight logic against Mojang's original bytes in offline CI. R66 already verifies generic `R65Fixture.a(FJ)V` test-only Attach, callback and restoration. R74 closes a different gap: a JVM actually loaded with the obfuscated caller and renderer owner names must continue its normal render-call path after Java retransformation, then return to exact pre-transformation bytecode.

## Test design
- `tests/java/bfk.java` declares a generated `bfk.a(float,long)` whose only original behavior increments a volatile counter.
- `tests/java/ave.java` adds an inert synthetic `ave.av(float,long)` call to its own `bfk` renderer field. No original Mojang files are included.
- `R74RenderTarget` starts a separate, disposable Java 8 JVM with `SLINKYWINKY_R74_DISPOSABLE_FIXTURE=1`, loads those two generated classes through the system classloader, and invokes the original-name method before Java Attach.
- A **separate, excluded-from-release** `slinkywinky_r74_test_agent.jar` is loaded by `R74AttachLauncher` only for that disposable PID and exact gate. Production `slinkywinky_instrument.jar` and native loader options are unchanged by R74.
- `R74RenderAttachAgent` reads `ave` and `bfk` once using a strictly passive transformer, checks the same **structural** renderer callsite shape as R73 (with synthetic fixture hashes), and re-transforms only generated `bfk.a(FJ)V` with a bounded test-only render callback.
- The fixture requires eight accepted callbacks from the owner thread while the original renderer executes sixteen times, a wrong-thread callback refusal, exact unmodified class SHA-256 restoration, and an additional twelve original renderer invocations with **zero** callbacks after restoration.
- Windows CTest `native_r74_obfuscated_render_attach` enforces these receipts and process-exit success. It is packaged into the build directory only, never into the downloadable injector preview artifact.

## Evidence and limits
The real R71 pinned original-class hashes are **not** replaced or weakened: synthetic fixture SHA checks are test-only. The code does not access Minecraft, LWJGL, OpenGL, native rendering contexts, input hooks, networking, or gameplay state.

Do **not** interpret a passing R74 fixture as proving live vanilla Minecraft renderer attach, full game-thread ownership, WGL/LWJGL compatibility, native overlay rendering, commercial Slinky fidelity, or clean DLL unload. A live original 1.8.9 process and explicit graphical acceptance are still required before enabling an actual in-game menu.

**Keep PR #1 draft and unmerged.**
