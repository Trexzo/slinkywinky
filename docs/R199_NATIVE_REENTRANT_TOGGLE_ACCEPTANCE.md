# R199 — Native duplicate-toggle and reentrant-callback hazard acceptance

Date: 2026-11-13. Repository: Trexzo/slinkywinky.
Integration branch: integration/openslinky-fd9c592-reference (PR #2, draft).
The other reconstruction PR #1 remains untouched.

## Scope and provenance

This analysis executes **the actual corrected 66-entry public OpenSlinky
module_manager.cpp.obj** in a standalone first-party Windows AMD64 harness.
Public OpenSlinky is pinned to upstream commit
fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4.
The compiled manager SHA-256 is
632d706ec121d4809a744876f4006c0e4ece19d7af0ad4994def704a77e516ae.

The unmodified upstream 67-entry manager (used only as a negative control)
has SHA-256
40573226782654580a89504ecbb7ccb8b0217f832cb4d1fa47b771f277c78f80.

No Minecraft instance, original commercial Slinky executable, full
public OpenSlinky DLL, JNI/JVMTI runtime, injector, network, or
original gameplay callbacks were executed. The manager's external
Java and native dependencies are first-party stubs. Native DLL/COFF
files remain on the connected authorized Windows PC and are not
uploaded to GitHub.

## 1. Duplicate queue admission — confirmed compiled behavior

Source inspection found that queue_module_toggle checks whether the
module's already-applied enabled state matches the request and tests
the queue's **66-item capacity**, but does not test whether an
identical transition is already pending.

The standalone harness confirmed the behavior of the **compiled** code:

1. A synthetic module with an event-5 callback receives three
   consecutive enable requests before the native queue is applied.
2. Native manager en_count becomes 3 (not 1).
3. Applying the queue delivers **three** event-5 enable notifications.
4. Three consecutive disable requests for that same enabled module
   yield native dis_count=3 (not 1).
5. Applying those disables delivers three event-5 disable notifications
   and invokes the stubbed release_owner cleanup **three times**.

The final observed totals are **six event-5 notifications and three
release_owner calls** for a single module whose state only transitioned
enabled once and disabled once.

In the harness release_owner is a harmless counter, not an actual
resource-freeing function. The observation shows duplicate cleanup
invocations but does not independently demonstrate an in-game double
free or crash.

## 2. Reentrant event-5 toggle — confirmed lost request

A synthetic event-5 handler requested an enable for a second module
during the first module's enable-notification callback.

In the compiled manager, the initial loop sets enabled flags before
notification delivery. While sending notifications, the manager loops
using the mutable en_count, and subsequently resets en_count to zero.

The bounded original native callback test observed:

- First module: enabled.
- Second module: remained **disabled**.
- Event-5 callback: delivered a second notification for the second
  module despite its state remaining disabled.
- Pending native queue: empty after the processing call.

The second enable request was therefore **lost**, rather than
deferred for a later frame. Only a bounded, single extra request was
injected; no recursive apply call or unbounded queueing was attempted.

## 3. First-party guarded queue prototype

The new independently authored test harness contains an explicit,
single-threaded GuardedToggleQueue adapter. It does not patch the
original native manager or touch upstream files.

Prototype rules:

- Verify module membership against the compiled manager's compacted
  66-pointer list; reject the excluded original source index 17.
- Collapse identical pending enable/disable requests into one.
- Reject contradictory pending transitions instead of silently
  applying an unexpected order.
- During native apply/notification processing, capture callback-origin
  requests in a bounded first-party deferred queue rather than mutate
  native en_count/dis_count mid-dispatch.
- Deduplicate reentrant requests by module and desired state.
- After the native apply returns, enqueue deferred requests to be
  applied on the *next* processing cycle, not in the current event-5
  notification loop.

The actual corrected native manager, exercised through that guard,
delivered **one enable and one disable notification**, called
release_owner once, and successfully enabled the second module on
the subsequent processing cycle. The membership and contradictory
pending-request rejection cases also passed.

Both direct hazard reproduction and guarded-policy acceptance
balanced native manager allocation/free counts at **76 / 76**.

These results establish a **harness-only prototype**, not a
production fix integrated with all real queue call sites, JNI/JVMTI
event dispatch, multiplayer concurrency, or authenticated original
client behavior.

## 4. Test results, negative controls and reproduction

Added two named native CTest cases:

- r199_native_duplicate_reentrant_hazard_observation
- r199_first_party_guarded_toggle_acceptance

Both passed in Debug and Release. The full local native CTest matrix
(R195, R196, R197, R198, R199) passed **7/7 in Debug and 7/7 in Release**.

The separate, hash-guarded
tools/run_r199_native_toggle_acceptance.ps1
recompiled the corrected and unmodified objects into isolated
standalone test executables. It passed both corrected R199 CTest
cases in Debug and Release. It then required the unmodified
67-entry manager to reject each mode with exit code 2 and precise
diagnostics: four of four original negative controls PASS.

Final local output:
R199_COMPILED_DUPLICATE_REENTRANT_TOGGLE_ACCEPTANCE_PASS
Runner exit code 0.

Reproduction, from the integration branch on the authorized
Windows PC with existing native COFF outputs:

    $root = Join-Path $env:TEMP 'slinkywinky-r193-llvm-portable'
    $rel = 'CMakeFiles/slinky.dir/src/engine/jni/module_manager.cpp.obj'
    $corrected = Join-Path $root ('build66/' + $rel)
    $original = Join-Path $root ('build/' + $rel)
    ./tools/run_r199_native_toggle_acceptance.ps1 -CorrectedObject $corrected -OriginalObject $original

The runner refuses to overwrite previous local evidence and
validates the exact source-object SHA-256 for each variant.
All test executables and real compiled objects remain local;
public GitHub CI compiles first-party harness source only.

## 5. Acceptance gaps and next work

The duplicate and reentrant behavior are verified **in the pinned
compiled public OpenSlinky native manager**, not shown to occur
in the commercial original Slinky release or the separate
in-development reconstructed game client.

The first-party guard is not installed in a live runtime:
all call sites, game-thread affinity, concurrent producers, reentrant
nested apply, cross-frame transactions, unload ordering, ownership
semantics and original parity still require review and tests.

Before using this approach outside the harness, integrate the guard
at a single owner-controlled queue-admission boundary, validate that
all callers use it, and ensure the deferred-work state has an
explicit thread and process lifetime. Do not blindly alter
upstream code or distribute the third-party compiled object.

Existing reconstruction coverage remains 66 descriptor identities,
171 native event subscriptions, 13 independent offline module
policies, 15 bound original callback decisions and 156
unimplemented native subscriptions. This test does not promote
new gameplay modules. PR #2 remains draft/unmerged.
