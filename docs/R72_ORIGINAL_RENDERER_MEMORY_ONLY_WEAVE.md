# R72 — SHA-pinned original 1.8.9 render method: memory-only ASM dry-run

**Status:** Implemented on draft PR #1; CI is the acceptance gate. This is **not** an enabled render callback or an in-game menu.

## Problem and provenance

R71 independently confirmed the original vanilla Minecraft 1.8.9 client: SHA-1 `3870888a6c3d349d3771a3e9d16c9bf5e076b908`, size 8,461,484 bytes, original callsite `ave.av` calling `bfk.a(FJ)V`, and original `bfk.class` SHA-256 `8a4ce93bbf272398ea298d2ed363f75211dd469fcd3b66682a387e4738a419a2`.

R72 tests a *potential instrumentation layout* against those exact original bytes, without touching a running game. It does not prove that arbitrary launchers or modded clients use the same renderer, that the GL context is safe, or that a real injected callback can run.

## Scope

- `OriginalRendererWeaveAudit` reuses the existing whole-client SHA-1/size check and independent structural renderer callsite audit. It then requires the **exact original** `bfk.class` SHA-256 before doing any trial transformation.
- In memory, ASM inserts two argument loads and a call to a deliberately nonexistent **offline-only marker** at the entry of the exact public instance `bfk.a(FJ)V`. No file contains the modified proprietary bytecode and this marker is never executed.
- The audit re-parses the transformed bytes and requires the same class identity, same method signatures and access flags, one marker call, and the same original opcode/operand event stream after removing the three planned prefix instructions. It also rechecks that the source byte array's SHA-256 is unchanged. This is a structural regression check, **not** proof of perfect byte-for-byte JVM verification or runtime compatibility.
- The original JAR is downloaded only inside the existing temporary R71 GitHub Actions audit from an allowlisted Mojang endpoint, verified before use and deleted on all exits. No original game bytes or transformed class bytes are uploaded as build artifacts.
- `OriginalRendererWeaveSmoke` uses generated, synthetic classes and tests incorrect SHA, wrong target class, wrong method signature, static method and corrupt class rejection.
- The standard Windows injector CTest runs synthetic negatives; the independent original-client workflow exercises the real R71-pinned `bfk` class. Nothing from R72 is compiled into `slinkywinky_instrument.jar` or `slinkywinky_agent.dll`.

## Evidence required

- Windows Java 8 agent suite: synthetic `R72_OFFLINE_WEAVE_SYNTHETIC_NEGATIVE_CONTROLS_PASS` marker and all existing lifecycle tests.
- Separate official 1.8.9 audit: `R72_ORIGINAL_RENDERER_OFFLINE_WEAVE_PASS` after whole-JAR and class SHA checks.
- Git workspace remains clean; verified client bytes remain ephemeral.

## Still outstanding

There is **no** production `bfk.a(FJ)V` retransformation, Minecraft Java render-thread callback, WGL/LWJGL state admission, native menu host connection or in-game UI rendering in R72. Do not interpret R72 as a playable client, match to commercial Slinky, or completed ClickGUI. Before allowing any in-game integration, require explicit owner consent, read-only render-context evidence on an authorized unmodified vanilla 1.8.9 process, graceful teardown and live graphics-state tests.

**Keep PR #1 draft and unmerged.**
