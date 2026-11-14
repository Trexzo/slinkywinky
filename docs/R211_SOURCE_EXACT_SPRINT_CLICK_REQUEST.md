# R211 — Source-exact native Sprint toggle gesture to module request

**Owner:** Chat 1, draft PR #1 (`reconstruction/r1-core-fidelity`), on top of R210. Chat 2's PR #2 remains untouched.

## Changes

- Adds `GuiModuleClickTransaction` (`include/slinkywinky/windows_gui_module_click_fixture.hpp`, `src/windows_gui_module_click_fixture.cpp`), a separately owned **synthetic fixture-only** gesture-to-request gate. It does not install hooks, suppress Minecraft input, obtain exclusivity from Win32, control a module or write a player field.
- Requires the first-party retained WGL menu's actual last-painted GL11 frame, current context/DC and viewport, focused canonical `Sprint` catalog entry and a matching `MenuHitKind::Toggle` at the physical pixel tested. An offscreen but keyboard-focused Sprint row is **not** a valid hit.
- Requires the existing R190/R191 synthetic fixture ledger to report a specific active exclusive-menu epoch, tracked all-up to start, paired mouse down then up, same owner/router and no held keyboard key. The gesture's `MenuMouseGesture` independently certifies the same painted target at both ends.
- Requires identical `drawn_frames`, source sequence, world generation, WGL owner/context and input epoch between mouse down and mouse up; no accepted request if any changes. A stale held gesture aborts the synthetic fixture ledger and must be independently restored.
- Commits only `GuiModuleControlHandoff::request_focused_toggle` at mouse-up. It cannot mutate `preview_enabled`, game memory, network, input queues or ArrayList active status. Only later acknowledged source-active state can paint ArrayList.
- Adds optional `expected_source_sequence` to the existing R210 GUI request and retained-menu adapter. This bounds the request to the clicked source snapshot and rejects mismatched requests without modifying already-published state.

## Verification and regression

The real disposable Windows WGL retained GUI test `native_retained_menu_source_mouse_transactions` now also runs `R211_SOURCE_EXACT_WGL_FIXTURE_MOUSE_TO_PENDING_SPRINT_PASS`. A compact 676x630 panel required scrolling the Sprint row into physically visible top rows: keyboard focus alone did not make its toggle hittable. Its actual hit is found through the existing `hit_test_menu`, never a guessed coordinate. Test exercises orphan mouse-up, unrelated click, off-thread delivery, complete Down/Up, unchanged preview and inactive HUD, source-sequence drift during a pending second click, fixture revoke/restoration, and permanent denial of original-game input suppression claims.

The existing R210 portable and WGL tests additionally cover expected-sequence mismatch and positive matched-sequence publication. Java8/JNI tests continue to run in the full Windows Release suite.

## Still required for a live game UI

This fixture **does not intercept or suppress physical input in original Minecraft**. There is no evidence of a production exclusive mouse/keyboard owner; do not call this from passive original-client hover observations. Chat 2 already has a module-control owner API but no production GUI handoff consumer. Before end-to-end in-game Sprint claims, implement verified original client input-queue ownership and restoration; execute exactly one owner-bound module request, obtain an authoritative acknowledgement and use the R209 ArrayList publication slot; test world loss and unload. A source-sequence receipt alone is not proof of module-owner freshness: the consuming owner must reject a stale observed sequence before applying a request.
