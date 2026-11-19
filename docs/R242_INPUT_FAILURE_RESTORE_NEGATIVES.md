# R242 — Fail-closed input observer cleanup after foreign-thread observation

2026-11-19 | Chat 1 | `reconstruction/r1-core-fidelity` | draft PR #1.

## Identified safety bug

R241's test-only `restoreOnOwner()` checked exact observer counters
**before** restoring the transformed method. If an unexpected
off-owner input entry was observed, the first rejection could leave the
disposable JVM running with the observer still woven into the method.
That is not a valid restoration contract.

## Correction

The existing R241 test-only agent now stops observations, records whether
the source owner counters are valid, and **restores and SHA-256-verifies
the original class bytes first**. It reports the counter failure only
after restoration, leaving a one-shot attach admission permanently
closed. The earlier all-valid eight-call R241 path remains unchanged.

A new R242 exact-PID disposable Java 8 target, launcher and Python CTest
exercise all the following against real cross-process Java Attach and
`Instrumentation.retransformClasses`:

1. Invalid agent options are refused without consuming the one valid
   source admission.
2. One legitimate attach admits eight source-owner input entries.
3. A ninth call from a different thread increments only the foreign
   counter, while the original method body still executes.
4. Another Attach is refused while active.
5. The unexpected foreign counter is detected and denied only **after**
   real source-byte restoration is verified.
6. Reattach remains denied after restoration; eight additional fixture
   calls preserve original behavior, yielding 17 total body calls and
   no resumed observation.

Both R241 and R242 are test-only agents and targets; no game JARs,
source assets, input hooks, keyboard/mouse consumer replacements,
packet operations or Minecraft client processes are involved.

## Remaining limits

This is a foreign-thread observation negative control, not a general
proof that all JVM faults are recoverable. Failures while registering
or removing transformers, JVM shutdown during restore, and live
original Minecraft classloader restoration still require independent
tests. Original 1.8.9 physical input ownership, duplicate dispatch
suppression and source-perfect ClickGUI parity remain unverified.
PR #1 stays draft/unmerged; PR #2 untouched.

## Exact-head CI

Append verified GitHub workflow results only after completion.
