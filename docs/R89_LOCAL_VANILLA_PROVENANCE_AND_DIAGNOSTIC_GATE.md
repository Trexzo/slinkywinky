# R89 — original vanilla client provenance fence for supervised live renderer acceptance

**Status:** source-level gate implemented, standalone injector compiled locally, real installed Mojang 1.8.9 jar audited **offline**. **No Minecraft process was launched or attached during R89**. This is not in-game ClickGUI verification.

## What was verified on the authorized Windows desktop

- Original installed `%APPDATA%\\.minecraft\\versions\\1.8.9\\1.8.9.jar` exists, size **8,461,484 bytes**, SHA-1 `3870888a6c3d349d3771a3e9d16c9bf5e076b908` and SHA-256 `14f0d96d1a56fb4f5c3b2233d00699525893fe5ce3dcf181e7de59120595d298`.
- The installed **Temurin OpenJDK 8u504 64-bit full JDK** has `lib/tools.jar`. CMake configured the real Windows C++/Java8 injector + JNI/JVMTI project with this JDK, and MSVC Release built the standalone native loader, agent, JNI bridge, render-audit and instrumentation artifacts successfully.
- The repository's own **non-injecting** `RenderCallsiteAudit` ran directly against this installed original Mojang jar and emitted:
  - `R64_RENDER_CALLSITE_CANDIDATE caller=ave.av renderer=bfk method=a descriptor=(FJ)V`
  - `R71_ORIGINAL_MINECRAFT_CLASS_SHA256=430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82`
  - `R71_ORIGINAL_RENDERER_CLASS_SHA256=8a4ce93bbf272398ea298d2ed363f75211dd469fcd3b66682a387e4738a419a2`
  - `R64_ORIGINAL_JAR_SHA1_VALIDATED`
- The complete local Java 8 Attach / JNI / WGL synthetic test suite passed **25/25 CTest**, including the R77 cross-process JNI/WGL/GL11 renderer and renderer restoration fixture, without touching Minecraft.
- The local R81/R89 Python acceptance test suite passed **22 tests, one platform-specific skip**. The installed Mojang jar passed the new SHA-1/SHA-256 verifier as well.

No user-owned game bytes, Minecraft JAR, proprietary source, credentials, or third-party fonts were placed in Git.

## New first-party source guard

`tests/original_189_live_render_acceptance.py` now optionally accepts `--original-client-jar PATH`; without it, the supervised `--execute` path defaults to the original vanilla file in `%APPDATA%\\.minecraft\\versions\\1.8.9`.

Before *any* JVM listing or Attach on `--execute`, it verifies:
- exact original size, SHA-1 and SHA-256;
- stable file-object metadata across the complete read-only hash pass, not a replaced file path;
- unchanged original artifact identity and digest before each subsequent R73/R75/R78 step, after receipt validation, and after final listing.

A wrong, missing or replaced JAR stops the acceptance before any additional Attach. Plan-only mode remains **no-IO/no-process**, and real-game callback tests still require explicit `--execute`, a single identified original vanilla PID, its kernel creation identity, a separate fresh target stdout log and independently verified loaded class fingerprints/restore receipts.

Regression tests additionally cover wrong JAR size, correct-size counterfeit hash and a JAR replacement after R73 but before R75. Mocked Windows runs never open Minecraft or send game packets.

## Remaining gate

Even the successful original-JAR and disposable-JVM results **do not establish a current WGL context in a live Minecraft process**. R81/R89 supervised original-game receipt tests and an independent visual stability check must pass *before* considering native in-game menu enablement. PR #1 remains **draft/unmerged**. No production in-game menu drawing was enabled in R89.
