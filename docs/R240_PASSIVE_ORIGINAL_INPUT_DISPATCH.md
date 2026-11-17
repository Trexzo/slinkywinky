# R240 — Source-pinned, opt-in original input dispatcher entry diagnostic

2026-11-17 | Chat 1 | `reconstruction/r1-core-fidelity` | draft PR #1.

An explicit new Java 8 attached-agent mode
`minecraft-input-dispatch-readonly-r240` adds a bounded passive
entry observer to the actual vanilla Minecraft 1.8.9 `ave.s()V` input
dispatcher. It is not enabled by default or by R99's native GUI mode.

## Live admission and safe restoration

R90 first verifies the two unique already-loaded original Mojang classes,
official JAR/source fingerprints, shared loader and transformed runtime
bytecode execution shape. A new input-specific preflight performs the same
source check on `ave` before the weaver runs.

ASM adds precisely one `INVOKESTATIC onInputEntry()V` at method entry.
The original input body executes unchanged. There is no code to invoke
LWJGL Mouse/Keyboard, intercept `Mouse.next`, consume/replay events,
suppress the vanilla method, or act on gameplay, window focus or GUI
state. The synchronized observer admits at most 64 owner-thread-only
callbacks, and the bounded supervisor closes observation before verifying
restoration. The weaver must be **removed** before original `ave`
retransformation/inspection. A missing removal or failed restored-source
equivalence keeps the shared attach admission gate locked.

A generated-class smoke compiles the **same ASM weaver** into a disposable
Java 8 classloader and exercises unchanged method semantics, repeat-hook
refusal, wrong-thread counts and post-close callback suppression.

This is original input **dispatcher-entry instrumentation**, not a
proof of exclusive GUI ownership. R194–R195 showed original input
consumers outside `ave.s()`; physical double dispatch and full JVM
input restoration remain independently unproven. The Windows desktop
was unavailable for live original-client input acceptance. PR #2 is
untouched; no original game JAR or private original source is committed.

## CI

Pending exact-commit verification.
