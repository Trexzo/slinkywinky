# R247 — real pinned PR #2 native-module owner through PR #1 GUI adapter

Date: 2026-11-19. Chat 1 PR #1, draft and unmerged. PR #2 untouched.

## What R246 could not prove

R246 implemented and tested a C++17 template adapter against a mock
with the same method names as PR #2's native module manager. It did not
compile PR #2's real `SprintObservationPreview` alongside PR #1.

## R247 actual cross-source test

A *separate, opt-in* CMake target `slinkywinky_r247_real_native_module_test`
requires `SLINKYWINKY_R247_PR2_ROOT` to point at a local checkout
of exact public PR #2 commit `7f457376058ad42e0a0e0323ed874d7e5b2ae03a`.
CMake checks `git rev-parse HEAD` and the existence of both source
headers before enabling the optional target. Ordinary builds do not
depend on PR #2, do not copy it, and do not run the integration target.

The dedicated GitHub Actions workflow
`.github/workflows/r247-real-pr2-integration.yml`
checks out PR #1 and exact pinned PR #2 into *separate directories*,
verifies the source commit, and **compiles the actual native
`reference_compat::SprintObservationPreview` header** together with
PR #1's `NativeModuleSnapshotAdapter`, `GuiModuleControlHandoff`,
and ArrayList implementations. It does not mock the native manager.

The test exercises:
- actual native initial in-world Sprint activation / No Slow disabled;
- source-focused No Slow GUI request on a separate GUI-owning thread;
- real PR #2 `request_module` enable, native `control_states` response,
  PR #1 owner acknowledgement and active ArrayList publication;
- actual source native No Slow `observe` decision with typed
  bow-using movement and configured multipliers;
- independent disable and re-enable acknowledgement and native decision;
- original native world-loss generation transition and revocation of
  active HUD rows while retaining requested No Slow selection;
- original native world re-entry and replacement of event subscriptions;
- cross-thread rejection and clean owner retirement/unload of
  the view feed.

This is a **real first-party PR1↔PR2 source integration compile and
offline native policy run**, not live Minecraft or an injected DLL.
Input is a synthetic read-only observation fixture; no Java/JNI
Minecraft process, original-client mouse/keyboard interception,
packet manipulation, game player write, or external server is used.

Remaining major work: bind this owner-side adapter to a real,
authoritative game-thread session, complete original GUI physical
input owner/restoration verification, implement additional native
module controls and the remaining callback subscriptions, and
validate each gameplay effect in an isolated real client.
No original commercial Slinky 1:1 parity claim is warranted.

## Exact-head GitHub result

Record verified workflow outcome after it completes.
