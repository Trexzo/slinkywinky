# R253 — pinned Windows process identity + real PR2 native recovery

Date 2026-11-20. PR #1 draft; PR #2 unchanged and exact-pinned to
`7f457376058ad42e0a0e0323ed874d7e5b2ae03a`.

## Verified OS facts, not user-submitted identity alone

`WindowsJvmKernelPin::open` takes the source `JvmLeaseIdentity`
issued by the active R251 disposable Java 8 Attach agent. It reads
the actual Windows process using `OpenProcess` with only
`PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE`, obtains the
authoritative process creation FILETIME using `GetProcessTimes`,
and pins the HANDLE against PID reuse. It also verifies the process
is still running, its executable basename is `java.exe` or
`javaw.exe`, its user SID equals the caller's Windows user SID,
and its JVM start millisecond value is close to the kernel birth
time. Unknown, inaccessible or mismatched process state fails closed.

`WindowsJvmKernelExitWitness` only permits a **distinct** Java
process replacement once the OLD PINNED HANDLE has become signaled
(exited) and the new pinned process is demonstrably live. The
witness rechecks these same kernel handles after the native
recovery callback. A revoked/terminated new JVM after native
publication causes R252 to quarantine the published view. It is
single use.

This **does not authenticate the JVM bytecode, Java publisher
signature, original Minecraft class loader, game account,
first-party game thread, or intent/consent**. A local same-user
adversary could be running `java.exe`. The fixture nonce is
public test correlation data, not a cryptographic capability.
Same-JVM lease handoff is deliberately refused at this stage.

## Actual disposable Windows Java8 and PR2 test

The dedicated `real-windows-java8-kernel-recovery` workflow job:
- compiles a real PR2 `SprintObservationPreview` native owner
  and R253 kernel-pinned witness in a separate PR #1 target;
- compiles R251 test-only Java8 agent, target, and the genuine
  Java8 `com.sun.tools.attach.VirtualMachine` launcher;
- opens an owned disposable old Java8 JVM, makes an actual Attach
  request and obtains its active R252 `ATTEST` receipt;
- launches R253 native verifier while that JVM still exists, so
  the **old kernel process HANDLE is captured before exit**;
- cleanly exits the old JVM; starts and actually attaches to a
  separate new Java8 JVM with a distinct kernel object;
- passes both signed-off fixture receipts into a strict
  OS-side process-birth/HANDLE witness for the full R252→R250
  recovery call using the *actual* PR2 module manager;
- tests forged birth and PID, replay and a recovered real No Slow
  native decision after admissible source publication.

No original Minecraft client is launched or modified. Nothing
is added to the release/production agent JAR; this workflow uses
only separate disposable R251 test JARs. Normal C++ and Java8
workflow builds remain independent.

## Remaining

Process identity is kernel-pinned **within this isolated
fixture**, not a connected commercial Minecraft authorization
system. Production would need verified class-loader/game-thread
identity, consent, attach/stop semantics, same-JVM renewal,
and a secure transport from JVM-owner attestations to the
native owner. R253 cannot enable real gameplay callbacks.

## Exact-head CI
To be recorded only after the actual commit checks pass.
