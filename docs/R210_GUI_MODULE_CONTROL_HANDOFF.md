# R210 — Chat 1 source-focused GUI to module-owner request/acknowledgement handoff

**Branch:** `reconstruction/r1-core-fidelity`, draft PR #1. Chat 2 / PR #2 remains independently owned and unchanged.

## Motivation

R209 connects real native ArrayList painting to a JNI copy-only runtime-state feed, but the existing `MenuState::preview_enabled` and `ToggleFocusedPreview` were intentionally only demonstration controls. Chat 2's R210 `ModuleControlRequest` and `ModuleControlState` contract already separates requested from active states and has a source-backed Sprint implementation. A concrete first-party GUI-side request transport is needed without secretly turning a demo flag into gameplay.

## Added

- `GuiModuleControlHandoff` (`include/slinkywinky/gui_module_control_handoff.hpp`, `src/gui_module_control_handoff.cpp`) maintains exactly one pending copy-only request and separately observed, dispatched, acknowledged, denied and retired phases.
- `request_focused_toggle` requires a source catalog row currently visible and focused in the **same** owner-bound `MenuState`/`MenuNavigation`, a real `ModuleArrayListFeed` snapshot, and no modal/unload dialog. Currently only the exact canonical name `Sprint` is accepted. Its desired state is the inverse of the **confirmed owner snapshot's requested bit**, never `menu.preview_enabled`.
- `take_on_module_owner` provides the request as plain values, pinning a single module-owner thread on first dequeue. The caller must independently verify it is the actual Minecraft/module owner; the queue does not attach or discover one.
- `acknowledge_on_module_owner` verifies the matching request ID, a later source sequence, nonregressing world generation, complete valid snapshot and exact acknowledged requested bit. Only then does it publish that state to the existing R208/R209 ArrayList feed. `active_in_world` can be false even when requested true (world unavailable); this is not an error or active HUD row.
- `deny_on_module_owner` leaves the published state unchanged. Invalid or replayed acknowledgements quarantine the handoff and retire the published feed to prevent stale active rows.
- `WglRetainedMenuPreview::queue_focused_runtime_toggle` is an opt-in owner-WGL/last-painted-focus adapter, without adding fields to the retained preview or changing existing JNI object layouts. An authenticated exclusive GUI gesture must be implemented **separately** before it is called in original Minecraft.
- `MenuNavigation::owns_menu` prevents an otherwise valid focused row from being paired with a different menu object.

## Windows verification

`r210_gui_module_request_ack_owner_lifecycle` tests real focused source row, independent preview flag, distinct module-thread owner, active Sprint acceptance, requested-but-inactive worldless acceptance, disable, denials, stale sequence and wrong-thread rejection. The existing `isolated_wgl_menu_state_restore` tests a genuinely painted retained GL11 menu issuing a pending request, rejecting absent context and avoiding changes to preview state.

The initial expanded Windows WGL test fixture crashed with Windows code `0xC00000FD` (stack overflow) because a second large `WglRetainedMenuPreview` was stack-allocated in an already large test scope. This was corrected by heap-allocating only the additional disposable test instance. Both targeted R210 tests passed after correction.

## Still unverified / owned by next integration

This is **not** evidence that physical original Minecraft mouse clicks/keyboard events reach this queue. Production code does not autonomously call `queue_focused_runtime_toggle`, and the actual Chat 2 owner manager has not yet been connected to `take_on_module_owner`. The tests use independently supplied admitted-like snapshots, not Chat 2's JNI manager. Do not claim interactive original ClickGUI, in-game module activation, all 66 modules, or original commercial Slinky equivalence. Once an input owner is verified, connect one authenticated Sprint request to Chat 2's legitimate owner callback and publish its real acknowledgement; retain explicit deny, world-loss, unload and input-restoration tests.

## Verified final test results — November 14, 2026

- Windows x64 Release build: exit code 0, JNI and Attach configured ON.
- Targeted `r210_gui_module_request_ack_owner_lifecycle`: PASS.
- Targeted `isolated_wgl_menu_state_restore`: PASS following the fixture's stack allocation repair.
- Complete configured Windows Release suite: **100/100 CTest PASS**, 23.84 seconds, exit code 0.
- Original Minecraft input/gameplay acceptance **not run** for R210; offline WGL and portable owner-model evidence cannot establish it.
