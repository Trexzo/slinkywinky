# R80 — pre-weave admission recovery and passive inspector lifecycle

## The issue

R75 and R78 reserve a shared `hookInstalled` admission flag before their exact-original `ave`/`bfk` renderer preflight. Before R80, a failed original-class SHA, missing game class or unsupported retransformation could throw before the bytecode-changing callback transformer was registered, but leave `hookInstalled=true` permanently. This could block later clean, authorized diagnostic attempts even though **our code never changed a game class**.

The inverse failure also matters: the passive, read-only class fingerprint inspector itself was registered with `Instrumentation.addTransformer` and removed with `removeTransformer`, but the boolean removal return value was not checked. If registration or removal becomes uncertain, it is not safe to treat that path like a clean, fully released preflight.

## Changes

- `RendererFrameDiagnostic.start(...)` now gives a clean **pre-weave** failure its own catch boundary. A missing or fingerprint-mismatched original renderer, unsupported instrumentation, or other ordinary pre-weave exception releases the unused admission flag via `finishUnmodifiedRenderAdmission()`.
- The hook gate is **not** released for a failure after a bytecode-changing transformer was registered or retransformation attempted. Such paths still require independent exact original `bfk` SHA restoration proof through `finishRenderDiagnostic(restored)`.
- `RendererLivePreflight.admit(...)` now checks that its passive inspector was removed, and distinguishes a false removal return, thrown removal or uncertain registration with `InspectorRemovalUncertain`. These cases leave the admission flag claimed, even though the inspector only returns `null` bytecode. An unexpected VM linkage failure is also left fail-closed rather than presumed recoverable.
- These rules apply equally to R75 Java-only callbacks and the R78 opt-in passive WGL diagnostic. They do not add a renderer menu hook, change game state or weaken the original SHA pin.

## Java 8 regression

`native_r80_preweave_clean_release_and_uncertain_lockout` uses a **fake Instrumentation implementation** and generated `ave`/`bfk` Java 8 class files. It does not start or attach Minecraft and does not load a native DLL.

The test checks that:
1. A missing original renderer leaves the claim unheld for both R75 and R78.
2. A JVM without retransformation support cleanly releases the claim.
3. Generated, structurally valid but **wrong-SHA** classes receive one read-only transform pass and a confirmed inspector removal; they are rejected without locking out a new preflight.
4. A passive inspector `removeTransformer` returning `false` or throwing retains the claim, and a concurrent second attach is denied before any new instrumentation.
5. An uncertain `addTransformer` call also retains the claim, and later attempts are refused until external recovery.

This is a **test of control flow and failure semantics** using synthetic class files; it does not establish original Minecraft compatibility or in-game WGL behavior. PR #1 remains draft until real original Java 8 game acceptance and exact restored renderer evidence are obtained.
