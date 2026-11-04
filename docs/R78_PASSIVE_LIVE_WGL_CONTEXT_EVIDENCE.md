# R78 — pinned original renderer, passive WGL context evidence

**Status: opt-in, experimental. No native menu drawing in live Minecraft.**

R77 proved a generated Java 8 renderer can be instrumented by an external Java 8 Attach, call JNI, draw the first-party GL11 menu in its own disposable WGL context and restore original generated class bytes. R78 adds a deliberately separate **read-only** native WGL observation option to the SHA-pinned original Minecraft 1.8.9 render callback.

## Production mode

From the external loader, `--render-context=8` requires **both** `SLINKYWINKY_RENDER_CONTEXT=1` and `SLINKYWINKY_RENDER_CALLBACK=1` in the loader's environment; `SLINKYWINKY_ATTACH_TEST=1` is refused. Target selection remains restricted to a same-user attachable JVM advertising Minecraft. The agent still requires R73's exact loaded original `ave` and `bfk` SHA-256 fingerprints, unique classloaders and original invocation shape before any JNI library is loaded or callback bytecode is changed.

The agent passes the canonical `slinkywinky_jni.dll` location to the R78 mode. `RendererFrameDiagnostic` keeps its existing **eight-frame** callback limit and **12-second maximum cleanup window**, exact renderer restoration verification, and permanent reattach lock if restoration is unproven.

For eligible callback entries, `RendererWglEvidence` invokes a new `nativeCurrentWglSnapshot` method. On that **same native caller thread**, it reads only `wglGetCurrentContext()`, `wglGetCurrentDC()`, and, when both are non-null, `glGetIntegerv(GL_VIEWPORT)`. It never activates or creates a context, renders geometry, reads a framebuffer, replaces a swap function, or touches input/game state. A missing context returns zeros without any GL call.

The Java evidence collector compares the **opaque** WGL context/DC handles and the viewport across at most eight observations. It records the number of present, absent, invalid and changed observations but does not log opaque pointer values. It admits a stable result only after eight same-thread, same-context, same-viewport observations and successful original renderer SHA restoration. A callback that has no WGL context is not a failure of Minecraft by itself; it means the game's real GL context is **not confirmed at this injection point**.

Only the target JVM prints the final R78 evidence after cleanup:
- `R78_READ_ONLY_WGL_CONTEXT_STABLE_PASS`: exactly eight valid, consistent observations after renderer class restoration; **not evidence of any in-game menu**.
- `R78_READ_ONLY_WGL_CONTEXT_UNVERIFIED`: missing/changed/invalid WGL sample, wrong owner, incomplete frames, or unproven restoration.

## CI test coverage

`native_r78_passive_wgl_evidence` is a disposable Java 8 test JVM using the R76 first-party temporary WGL fixture plus the production `slinkywinky_jni.dll` native method. The test checks: a missing context is not admitted, eight stable snapshots, HGLRC replacement, changed viewport, temporary context absence, cross-Java-thread refusal, and finite sampling. Separate Windows loader/Attach tests require both operator gates and reject test and non-Minecraft PIDs. The old R73 original-bytecode audit and R77 end-to-end JNI menu fixture remain independent.

## In-game acceptance still missing

The desktop computer was offline for this work, so this specific original-game callback and WGL evidence path has **not** been run in Minecraft. R78 does **not** connect the production render callback to the C++ GL11 menu host, and does **not** claim Slinky parity, functional injection UI, menu interaction, or reliable game rendering. Keep PR #1 draft/unmerged until a supervised original 1.8.9 Windows Java 8 run produces both the R75 exact class restoration marker and an R78 result, then verify GL ownership and state isolation separately.
