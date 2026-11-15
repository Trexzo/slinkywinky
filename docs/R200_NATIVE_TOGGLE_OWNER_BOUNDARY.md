# R200 — Owner-thread native toggle admission boundary

Date: 2026-11-14. Repo: Trexzo/slinkywinky, PR #2, draft.
Branch: integration/openslinky-fd9c592-reference.

## Scope and source ownership

R199 established two defects in the separately compiled public OpenSlinky
66-entry module manager: duplicate pending toggles generate extra notifications
and cleanup calls, and reentrant event-5 requests can be lost. R199's first-party
guard was only a harness-local prototype.

R200 introduces a reusable first-party component at
reference_compat/include/slinkywinky/native_toggle_owner_gate.hpp.
Its executable synthetic suite is
reference_compat/tests/test_native_toggle_owner_gate.cpp.

The standalone compiled-manager harness now uses a thin R200 adapter rather
than reimplementing queue admission locally. The adapter binds the actual
corrected native manager's 66 registered pointers to the first-party gate and
uses queue_module_toggle, apply_module_toggles and native enabled flags via
explicit callbacks.

PR #2 is not the live Minecraft runtime branch. We have not changed its
loader, JNI/JVMTI code, injected DLL, GUI, native third-party source,
or the concurrently edited PR #1.

## Owner-thread and session contracts

The new NativeToggleOwnerGate contract is:

1. Owner captured at construction; non-owner submit, apply and shutdown are
   refused. There is no implicit off-thread message delivery.
2. Nonzero session generation, unique non-null module identities, and an
   explicit capacity in the range 1 through 66.
3. Null, foreign and expired-generation requests are rejected before any
   native callback.
4. Same-state duplicate pending requests are idempotent, conflicting
   pending transitions are rejected, and both native-stage and deferred
   queues are individually bounded.
5. Any callback-origin request during native queue admission or native
   apply is deferred, never allowed to modify the active native queue.
6. Deferred requests are submitted after native apply, and executed no
   earlier than the next owner-thread apply.
7. Missing world/player readiness preserves the pending queue.
8. Exceptions and failed native apply force irreversible fail-closed
   shutdown. A rejected native queue request is not spuriously admitted.
9. Shutdown is irreversible: native manager cleanup and native callback
   retirement are the caller's responsibility.
10. This design assumes all native toggle entry points route through one
    gate. Any bypass invalidates deduplication and reentrancy guarantees.

The component uses only first-party C++17 headers, with no game API access
and no proprietary source, executable native callbacks or native image data.

## Executable synthetic tests

The new CTest r200_native_toggle_owner_thread_and_lifecycle exercises:
duplicate/contradictory requests; membership and capacity; stale generations;
world/player readiness; event-origin and native-admission-origin reentrancy;
wrong-thread submit/apply/shutdown; native veto; native apply exception;
shutdown inside callbacks; re-creation under a new session; and constructor
argument validation. Its test modules and native queue are invented fixtures.

The tests are registered in reference_compat/CMakeLists.txt, which
GitHub Actions builds and runs in Debug and Release on Windows and Linux.
The Windows source-only compiled-manager harness target also includes the
new reusable header without third-party object linkage.

## Verification status and remaining gates

At authoring time, GitHub CI for R200 has not been confirmed successful.
Desktop Commander was offline on November 14, so the exact COFF-linked
R199/R200 harness acceptance was NOT rerun on the user's Windows PC.
R199's earlier actual COFF results remain verified independently but do not
establish acceptance of the revised R200 adapter.

The next native acceptance when the PC is connected is to update only
PR #2's checkout, build the harness against the hash-pinned corrected manager,
rerun all R195-R200 CTests in Debug/Release, and confirm original 67-entry
negative controls still fail as expected.

R200 does not establish cross-thread dispatch, JNI/JVMTI hook installation,
real Minecraft frame callbacks, clean native unload, GUI visual fidelity,
or authentic commercial original equivalence. No additional gameplay
module is promoted to working status: coverage remains 66 original descriptor
identities / 171 subscriptions / 13 offline policies / 15 bound callbacks.

Keep PR #2 draft and unmerged. PR #1 is untouched.
