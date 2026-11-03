# R75 — bounded original vanilla 1.8.9 render-callback diagnostic

**Status: experimental, disabled by default, draft PR #1.** This is a finite Java-only callback diagnostic. It does **not** draw an in-game menu, access LWJGL/OpenGL/WGL state, modify game logic, or certify the live render thread. The owner desktop was offline when this was developed.

## Implementation
- R73's `RendererLivePreflight.admit` must first establish the **exact, original SHA-256** for both loaded `ave` and `bfk`, the unique `ave.av → bfk.a(FJ)V` callsite, the renderer field, and matching classloader.
- A separate `--render-callback=8` option is rejected by default. The operator must set `SLINKYWINKY_RENDER_CALLBACK=1` in the Windows loader's environment. The CLI refuses a non-Minecraft PID and any `SLINKYWINKY_ATTACH_TEST=1` process. Only the exact original game classes are eligible.
- The existing Java agent's instrumentation lock is shared with other tick and preflight modes. A second Attach call cannot overwrite an active owner session.
- `RendererFrameDiagnostic.weavePinnedOriginal` inserts the exact `FLOAD 1; LLOAD 2; INVOKESTATIC RendererFrameDiagnostic.onRender(FJ)V` prefix to the SHA-pinned original `bfk.a(FJ)V`. No other class or method is intended to change.
- In the callback, `RenderFrameGate` samples at most **eight** stack-validated frames, requiring the adjacent Java stack `bfk.a` under `ave.av`, a consistent owner `Thread`, finite [0,1] partial tick values and non-negative frame time. It never calls JNI or graphics functions.
- A daemon watcher stops sampling after eight frames or **12 seconds**, deactivates callbacks and requests a **second** class retransformation with the weaving transformer already removed. The inspector must observe exactly one pristine SHA-pinned original renderer class. Only a proven restoration releases the session admission lock. Failure to restore prevents future instrumentation attempts within that JVM.
- Diagnostic evidence appears in the **target JVM output**, not just the loader process. It reports `R75_ORIGINAL_RENDER_CLASS_RESTORE_SHA256_PASS` and `R75_RENDER_CALLBACK_BOUNDED_PASS accepted=8` only after successful restoration plus bounded, stack-valid samples. A timeout, malformed frame, wrong thread or restoration failure must never be reported as a callback pass.

## Offline acceptance
Windows Java 8 tests check: exact CLI opt-in, refusal of a non-Minecraft PID and Attach-test bypass, the eight-frame bounded state machine, invalid render fractions/time, wrong stack and thread, and pinned hash rejection.

The independent original Mojang JAR audit downloads and verifies the official vanilla 1.8.9 SHA-1 and size in a temporary directory and tests the **actual production callback prefix** in memory. It checks that the original method opcode/operand event stream and method list are preserved with exactly those three added entry instructions. No modified proprietary class files are written, uploaded or shipped.

## Controlled live acceptance — NOT YET PERFORMED
On an authorized Windows system with an independently backed-up, original vanilla 1.8.9 Java 8 instance:

1. First run the already implemented `--render-preflight` for the specific PID and inspect the original target JVM's R73 marker. Stop if it cannot admit exact unmodified classes.
2. Only for a controlled disposable game session, launch the game normally. In the *separate loader terminal*, set `SLINKYWINKY_RENDER_CALLBACK=1` and invoke `slinkywinky_loader.exe attach PID JDK8_HOME --render-callback=8`.
3. Verify from the game's own console/log that it records **both** the bounded callback result and the exact SHA-256 restoration marker. An `ATTACH_RESULT=SUCCESS` receipt alone is **not sufficient**.
4. Inspect whether the game continues to render after the diagnostic is gone and restart the game if any state is uncertain. Do not attempt to override the locked session if the restoration marker is absent.

This code does **not** validate WGL context, graphics state isolation, native menu drawing, actual HUD input handling or a fully working commercial-Slinky-like injectable GUI. Those require separate, supervised in-game tests. Do not merge or enable in normal startup until validated.
