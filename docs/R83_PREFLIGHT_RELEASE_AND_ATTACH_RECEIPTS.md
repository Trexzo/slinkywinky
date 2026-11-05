# R83 — original preflight admission-release fencing and exact Attach identity

**Scope:** Original, SHA-pinned vanilla Minecraft 1.8.9 diagnostics only. This does not enable the native in-game menu.

## Observed sequencing issue

R81 requires a post-release marker after the R75 and R78 finite renderer callback, but the R73-only `minecraft-render-preflight` branch previously printed `R73_LOADED_ORIGINAL_VANILLA_189_RENDERER_PREFLIGHT_PASS` inside `RendererLivePreflight.run`, then cleared the shared `hookInstalled` flag in `InstrumentationProbe.agentmain`'s `finally` block.

An external acceptance harness might observe the R73 marker and immediately request `--render-callback=8` **before** the finally block released the admission lock. R83 closes that gap:

- On a successful R73 preflight, after the passive inspector is confirmed removed and `hookInstalled.set(false)` has completed, the target JVM emits exactly one **`R83_ORIGINAL_PREFLIGHT_ADMISSION_RELEASED_PASS`**.
- On an SHA mismatch or a normal preflight exception, the unused admission lock may be released, but **no success or release-PASS receipt** is printed.
- On an unconfirmed inspector registration/removal, R80's fail-closed lockout remains in effect.
- On an unexpected JVM `Error` (e.g., `LinkageError` during class discovery), R83 now explicitly keeps the gate locked rather than treating the inspector lifecycle as proven.

The R81/R82 manual acceptance harness now requires **both** R73 preflight and R83 release receipts in exact order before moving to R75, just as it already requires the R81 release marker after R75/R78 restoration.

## Exact external loader identity

The harness previously accepted a substring `ATTACH_RESULT=SUCCESS pid=<PID>`; consequently a response for `pid=1234` could appear to satisfy `pid=123`, or a different Attach mode could be mistaken for the selected original-renderer stage.

R83's `validate_loader_receipt` requires **one complete external loader stdout line**, matching the known Java8 Attach agent format exactly:

| Requested diagnostic | Required external helper mode |
|---|---|
| `--render-preflight` | `ATTACH_RESULT=SUCCESS pid=<PID> mode=render-preflight` |
| `--render-callback=8` | `ATTACH_RESULT=SUCCESS pid=<PID> mode=render-callback-r75` |
| `--render-context=8` | `ATTACH_RESULT=SUCCESS pid=<PID> mode=render-context-r78` |

An alternate PID, mode, failure, duplicate response, suffix, prefix or extra stdout line is rejected. This **external** receipt is only proof that the Attach command was accepted; the independent **target-JVM** post-release markers and exact restored-class SHA are still required for the next step.

## Regression tests

The existing original-client audit workflow now checks:
- Missing/late/reversed R83 release receipts cannot satisfy the R73 stage.
- Valid R73 then R75 then R78 target receipts pass with the added R83 gate boundary.
- External Attach receipt has one exact PID, mode and result, rejecting `pid=1234` for `123`.
- The Windows fully mocked three-stage `--execute` path supplies realistic per-mode receipt text.
- `RenderPreweaveAdmissionSmoke` tests that a synthetic `LinkageError` during R73 passive class discovery retains admission and prevents a second diagnostic. It never loads actual game classes or JNI.

This is a **static, synthetic and offline CI** change. It does not establish that the target JVM process stayed visually healthy after the diagnostics, nor certify real WGL ownership. The original actual game acceptance remains outstanding while the Windows desktop is offline. Keep PR #1 draft and unmerged.
