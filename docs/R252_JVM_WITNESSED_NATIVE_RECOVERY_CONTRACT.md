# R252 — JVM-exclusive witness contract to R250 native recovery

Date: 2026-11-20. Chat 1, draft PR #1; PR #2 untouched.

## What changed

R251's disposable Java 8 Attach API test had an authenticated **fixture**
lease state, but R250's native recovery accepted only source generation
numbers. A copied receipt must not be equivalent to a live owner lease.

`include/slinkywinky/jvm_witnessed_native_recovery.hpp` adds an R251
identity parser and `JvmWitnessedNativeRecovery`. It requires:

- a strictly valid source identity: positive PID, JVM start timestamp,
  lease epoch and exactly 32 lowercase nonzero hex nonce characters;
- the old and new identities to be distinct, a same-JVM reattach to
  have a strictly newer lease epoch, and the new challenge to differ;
- an explicitly supplied **exclusive witness** that checks the old
  lease quiescence and the new lease activity and *holds* that
  decision while executing the owner-native R250 recovery callback;
- the witness to invoke the recovery callback **exactly once**, not
  merely return true; a callback failure cannot be overridden by the
  witness returning true;
- no replay of an already admitted native recovery, no foreign-thread
  admission, and fail-closed publication retirement if the witness
  returns false or throws after a native snapshot was published;
  a refusal before any publication remains retryable rather than
  needlessly poisoning the native view;
- R250's full old-owner native shutdown, discarded GUI/feed, new native
  world-generation/sequence, and fresh adapter/feed rules remain in
  force. The **actual PR2 source manager** is used in the pinned CI
  integration test for the latter.

A different JVM process incarnation may start its Java8 lease epoch
at 1 again; this does **not** relax R250's native world/sequence
monotonicity check.

## Two complementary verification paths

1. Pinned PR2 native CI
   `r252_real_pr2_jvm_witnessed_native_recovery` uses the actual
   `SprintObservationPreview` with a strictly controlled, **fake**
   exclusive-witness implementation. It verifies positive real native
   recovery and a real No Slow GUI request, process identity and epoch
   mismatches, replay, malformed receipts, owner-thread refusal,
   and witness refusal/exception after publication. This confirms
   interface and fail-closed behavior, **not** a production JVM identity.
2. Real Java 8 Attach CI extends the existing R251 disposable target
   with owner-only `ATTEST`, which returns a receipt **only while
   the real R251 test Attach agent reports an active, unpoisoned lease**.
   A standalone C++ parser probe verifies those exact actual Java8
   receipts, then rejects independently tampered PID, birth time,
   epoch and challenge values. Inactive/poisoned sessions produce no
   receipt. Both the C++ probe and the R251 agent are test-only and
   excluded from normal package artifacts.

## Deliberate security limit

There is **no production trusted OS/JVM witness implementation** in
this change. A fake or user-created witness may call the callback;
this API is a contract awaiting a trusted implementation, not a
cryptographic capability. The displayed R251 challenge is a
test-generated correlation token, **not a secret** and not a basis
for original-client authorization. The two verification paths are
complementary tests; they are **not a shared production JNI session**
or a verified process identity handoff.

No Minecraft is launched, no physical input owner is acquired, no
game action is written, PR2 is not modified and unimplemented
module callbacks are not claimed recovered.

## Exact-head verification

Only mark after the GitHub workflows complete on the exact commit.
