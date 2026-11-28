# R261 — current PR2 native GUI integration, exact-head verification

**Date:** 2026-11-28. **PR #1** `reconstruction/r1-core-fidelity` (draft, unmerged).
**PR #2** `integration/openslinky-fd9c592-reference` (draft, unmerged).

## Problem

The existing R247–R257 cross-branch integration suite was intentionally
pinned to PR #2 commit `7f457376058ad42e0a0e0323ed874d7e5b2ae03a`.
The actual current PR #2 head had advanced to R371:
`898a0e0bdd9d6bb3aea90121d89c9dfb4da2a9a0`.
A green old-pin CI run was no proof that PR #1 GUI handoffs still compiled
or behaved correctly with the *current* native module manager.

## Changes

- `CMakeLists.txt` preserves the known-good R247 default commit pin.
  The optional, explicit `SLINKYWINKY_R247_PR2_EXPECTED_SHA` allows a
  **separately source-verified checkout** to be tested without replacing
  or weakening the baseline historical pin.
- `.github/workflows/r261-current-pr2-compatibility.yml` checks out
  current PR #2 on its dedicated branch, records the actual 40-character
  commit hash, passes that exact hash into CMake and runs the seven
  existing independent native owner/GUI integration tests.
- A negative preflight configures the same source with an intentionally
  incorrect SHA and requires the exact source-mismatch rejection.
- The existing historical-pinned R247 workflow remains unchanged.
  Neither branch is merged; no native callback, packet, game-input or
  Minecraft render hooking behavior has changed.

## Verified CI result

**[R261 current-head compatibility — SUCCESS](https://github.com/Trexzo/slinkywinky/actions/runs/46161686180)**

- PR #2 checkout: `898a0e0bdd9d6bb3aea90121d89c9dfb4da2a9a0`.
- Negative source pin: `R261_BAD_PIN_REFUSED_PASS`.
- CMake current-source configuration and C++ build: **PASS**.
- **7/7 CTests passed**:
  `r247_real_pr2_native_gui_owner_runtime_roundtrip`,
  `r248_real_pr2_three_module_gui_world_lifecycle`,
  `r249_real_pr2_native_uncertain_acknowledgement_fence`,
  `r250_real_pr2_quarantined_owner_recovery`,
  `r252_real_pr2_jvm_witnessed_native_recovery`,
  `r254_real_pr2_three_module_policy_parity`,
  `r257_real_pr2_reach_event8_140_frame_parity`.
- Terminal source receipt:
  `R261_CURRENT_PR2_COMPATIBILITY_PASS 898a0e0bdd9d6bb3aea90121d89c9dfb4da2a9a0`.

## Correct evidence boundary

This establishes that the existing **offline** PR #1 GUI/native-control
handoff, three source-managed module controls and retained lifecycle
regressions still compile and pass with PR #2 R371. The test only runs
source-owned models and disposable fixtures; it does **not** link the
two draft branches into one production binary or prove that a
user-operated native menu can activate all 66 Minecraft modules.

The active native module roster remains 66 (60 partial offline
controllers plus six experimental combat models). Full reference
parity, original-client mouse ownership, all module callbacks,
complete settings, production standalone injection/GUI integration,
and authoritative packet/gameplay actions remain unfinished.

**Next integration gate:** replace the isolated demonstration with a
versioned, consolidated runtime contract and test that contract inside
the actual standalone original vanilla 1.8.9 owner, while preserving
owner-thread, process-birth and rollback guarantees.
