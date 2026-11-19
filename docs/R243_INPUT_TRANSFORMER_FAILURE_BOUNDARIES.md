# R243 — Transformer failure boundaries and verified restoration

Date: 2026-11-19 | Chat 1 | draft PR #1 `reconstruction/r1-core-fidelity`.

## Scope

R242 fixed a foreign-thread counter error that prevented restoring the
test-only observer before reporting failure. R243 extends that **same**
disposable Java 8 agent with deterministic, opt-in fault injection at
three boundaries: BEFORE first transformer registration, BEFORE first
weaver removal, and BEFORE the first restoration retransformation.

The test-only agent requires both
`SLINKYWINKY_R241_DISPOSABLE_FIXTURE=1` and
`SLINKYWINKY_R243_DISPOSABLE_NEGATIVE=1`, plus an exact allowlisted
`SLINKYWINKY_R243_FAULT_MODE`. This fault mode does not exist in the
production injected instrumentation JAR.

## Fail-closed behavior

- **install:** no transformer is registered. Admission remains permanently
  closed; original fixture ticks continue unchanged and no observer starts.
- **remove:** the first removal call is deliberately interrupted
  *before it executes*. The cleanup path requires a subsequently confirmed
  `removeTransformer` success before it may restore source bytes. A passive
  retransformation and exact SHA-256 verification then restore the fixture.
  Failure remains reported; the observer is never activated.
- **restore:** the first restoration retransformation is deliberately
  interrupted *before it executes*. Because the weaver was independently
  removed, the cleanup path performs one real retransformation, observes
  exactly one original-source SHA-256 match and confirms verifier removal.
  The original method body continues after restoration and reattach stays
  refused. A failure in the actual API would remain uncertified unless the
  exact post-retry restoration proof succeeded.

The existing R241 successful Attach flow and R242 unexpected foreign
thread/counter recovery are preserved. A missing confirmation is NOT
treated as restored. No automatic retry can re-enable an already failed
one-shot agent admission.

## Evidence

A new CTest runs three independent disposable Java 8 JVMs. Each
receives an external Java 8 Attach API call to its exact PID, exercises
one injected failure, checks the original source method body behavior,
then tests that a second Attach is refused. It rejects missing or
duplicated markers and any R243 test classes in the production agent JAR.

These are **simulated failures at known boundaries**, not real HotSpot
throwing at arbitrary points. A genuine exception midway through
JVM retransformation, undefined transformer removal state, process
termination during cleanup, and original 1.8.9 live game input
ownership still require separate evidence. No Minecraft process is
launched, attached to or modified. No keyboard/mouse interception,
packet routing or gameplay module is enabled.

Keep PR #1 draft/unmerged; do not edit Chat 2's PR #2.

## Exact commit verification

To be filled from actual GitHub CI results, not predicted.
