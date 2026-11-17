# R241 — Real Java 8 cross-process input observer attach/restore fixture

Date: 2026-11-17. Chat 1; `reconstruction/r1-core-fidelity`, draft PR #1.

## Why
R240 proved that the **exact Mojang 1.8.9 source** receives only one
passive method-entry observation call and that all 58 input consumers
keep their original instructions. However, the R240 synthetic regression
weaved bytes in an isolated classloader only; it did not verify actual
Java Instrumentation retransformation and removal in another JVM.

## Added isolated acceptance
- A strict opt-in `SLINKYWINKY_R241_DISPOSABLE_FIXTURE=1` disposable
  Java 8 JVM and a separate Java 8 Attach API caller.
- A **test-only** agent calls R240's **unchanged real production ASM
  `weaveFixture`** against the already-loaded R240 fixture class,
  with exact class identity/system-loader and one-transform checks.
- The transformed input method runs eight times on the fixture owner.
  The test calls `fixtureEnd`, removes its weaving transformer, then
  performs a second real class retransformation with a passive source
  verifier that checks original class SHA-256 bytes before removing
  the verifier. The same fixture subsequently runs eight more times.
- The independent Python test requires distinct exact receipts for
  external Attach, eight observed input entries, verified real bytecode
  restoration, 16 unchanged input-method body invocations and clean
  shutdown. It checks that all R241 test classes are absent from the
  production instrument agent JAR.
- Test files and the separate test-only agent manifest are not part of
  the actual Minecraft injector. The real original input observer's
  admission, game-specific classloader and complete in-world restoration
  are not being exercised by this fixture.

## Scope and remaining evidence
No Minecraft process is launched, attached to, or modified by R241.
No LWJGL input queue is captured; no vanilla handler is suppressed;
no gameplay or GUI click is forwarded. This is stronger synthetic JVM
restoration evidence, **not original-client hardware input acceptance**.

Final source commit and exact-commit CI outcomes must be recorded from
GitHub Actions before calling R241 fully verified. PR #1 stays draft and
unmerged. PR #2 is untouched.
