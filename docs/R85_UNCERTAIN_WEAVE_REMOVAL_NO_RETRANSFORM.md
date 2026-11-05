# R85 — never retransform with an unremoved renderer-weave transformer

**Scope:** R75/R78 finite SHA-pinned original vanilla Minecraft 1.8.9 render diagnostics. The original-game in-process UI is still disabled; this is a bytecode-restoration safety fix, not an in-game rendering feature.

## Recovered concrete bug

In `RendererFrameDiagnostic.start`, R75 installs a bytecode-changing transformer to insert the bounded callback at `bfk.a(FJ)V`. After the first retransformation it attempts `instrumentation.removeTransformer(transformer)`. The **installation-error path** previously called the passive original-bytecode `restore()` routine **even when removal returned `false` or threw**.

An unremoved transformer may run again when `restore()` invokes `retransformClasses(target)`, potentially **reapplying the callback weave to the renderer class**. That is exactly the scenario where attempting another retransformation is unsafe. Likewise, `addTransformer` might throw after partially registering a transformer; `added=false` does not establish that nothing was registered.

## Fix

- Introduced one guarded `restoreIfWeaverRemoved(...)` gate used in the installation-failure and cleanup-worker-start-failure recovery paths.
- When `weaveRemoved == true` (a confirmed successful `removeTransformer`), the existing passive inspector may attempt exact SHA-256 restoration as before.
- When `weaveRemoved == false`, it **does not invoke `retransformClasses` or register a new restoration inspector**. Instead the JVM logs `R85_UNCERTAIN_WEAVE_REMOVAL_SKIP_RETRANSFORM` and retains the shared `hookInstalled` admission lock. No further R73/R75/R78 Attach is allowed within that target JVM.
- A failed `removeTransformer` on the weaving transformer therefore cannot cause the failed-installation recovery path to execute the same transformer again. The existing successful eight-frame cleanup and verified restoration path is not modified.

**Limitation:** This cannot guarantee the game remains healthy after a partially registered or unremoved transformer. It intentionally avoids an extra unsafe mutation, refuses to claim restoration and requires the operator to restart the disposable game session instead of overriding the lock. Any log without the exact R75 restoration + R81 release markers is still an unverified result.

## Synthetic Java 8 regression

Extended the existing `RenderPreweaveAdmissionSmoke` fixture with a reflection-only invocation of the exact production guard using its synthetic proxy `Instrumentation`:

- **Unconfirmed weave removal:** `restoreIfWeaverRemoved(false, ...)` must return `false` and register **zero** passive inspectors, remove **zero** transformers and invoke **zero** retransforms.
- **Confirmed weave removal:** `restoreIfWeaverRemoved(true, ...)` must attempt one bounded passive SHA check with exactly one inspector registration, one class retransformation and one verified removal. The generated fixture deliberately cannot pass the SHA-pinned original-class check.

The test does not start Minecraft, load proprietary classes, load native libraries, draw, or modify a real JVM. Existing Windows Java 8 CTest and the official original 1.8.9 memory-only audit remain independent acceptance gates.

See draft PR #1. Keep the original renderer diagnostic **opt-in only** and the PR unmerged until supervised real original-game testing.
