# R251 — Java 8 Attach identity and competing-owner fence (disposable JVM)

Date: 2026-11-20. Chat 1, draft PR #1. PR #2 remains untouched.

## R250 boundary remaining

The R250 native source recovery test proved that a quarantined native
manager must be stopped and a new owner must publish state with strictly
newer world generation and source sequence. **It did not establish
authenticity of a process or VM attachment**, which still needs an
independent OS/JVM-side identity check.

## R251 implementation and strict scope

R251 adds **test-only**, first-party, Java 8 `Attach` components:

- `R251LeaseTarget`: separately launched disposable Java 8 process
  with a JVM runtime start-time receipt, own PID, rotating 128-bit
  SecureRandom challenge, incrementing local session generation, and
  a pinned JVM main-thread owner for cooperative release.
- `R251LeaseAgent`: a different-process `agentmain` loaded into
  that exact disposable JVM. It checks JVM-local PID/start,
  currently issued challenge and generation, and exact system-loaded
  target class identity before an exclusive lease can be admitted.
  A duplicate owner cannot take an active lease. An uncertain owner
  permanently locks the old JVM's admission gate. R251.1 uses an
  immutable, volatile epoch/challenge pair so the Attach agent can
  inspect a consistent target proof without taking the target monitor
  while holding the agent monitor; this removes a potential
  Attach-versus-owner-rotation lock-order inversion.
- `R251LeaseLauncher`: invokes Java 8's real
  `com.sun.tools.attach.VirtualMachine.attach(targetPid)` and
  `loadAgent`, **only** against an orchestrator-owned target PID.
- `r251_disposable_jvm_lease_attach.py`: starts and exclusively
  supervises two test JVMs, checks failed PID/start/challenge proofs,
  a duplicate/competing claim, foreign-thread release refusal,
  valid owner-thread quiescence, stale proof/replay rejection,
  new generation admission, permanently poisoned old JVM state,
  different JVM reincarnation, and original-process-clean exit.

The fixture has a dedicated `slinkywinky_r251_test_agent.jar`
with `Can-Retransform-Classes: false`. It never adds game
transformers, accesses Minecraft classes or writes game state.
The normal production `slinkywinky_instrument.jar` is checked
to contain **no** R251 fixture class; the R251 jar is not included
in the published standalone preview artifact.

The existing `Injectable Windows Java 8 agent` GitHub workflow
runs the real external Attach test via CTest. Prior R241–R243
real-Attach/restore tests continue unchanged.

## Security and integration limitation

This is **not cryptographic authentication of a commercial Minecraft
client**. A fixture-issued nonce visible in the test controller's
stdout is not a production secret; PID plus Java's JVM start time
does not independently attest a trusted OS process. There is no
R251-to-R250 JNI bridge or live native module owner connected here.

Before a real-client attachment can be admitted, a production
implementation must establish the trusted process/window/JVM
incarnation, bind that to R250's native owner generation and
request/ack state, reject stale workers after unload, and prove
input restoration without activating unverified game hooks.

## Exact-head verification

Do not infer success: add the GitHub CI outcome after completion.
