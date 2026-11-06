# R104 — source-admitted foreground eligibility fence (no game input authority)

**State:** implemented in draft PR #1, not merged. This step implements and tests a first-party native **foreground/focus-loss lifetime contract** against an isolated WGL test HWND. **No Minecraft input hook, no direct keyboard or mouse capture, no real-world foreground forcing, no combat/movement features.**

## Why this is separate from R103

R103 established that the exact original Minecraft 1.8.9 source-pinned render owner’s WGL HGLRC/HDC resolved to the same unique 854×480 HWND observed from the external Windows process, while safely restoring original renderer bytecode after four fresh admission stages. But the actual original Minecraft window was **background across all eight native samples** and both eight-sample external brackets. A matching render HWND is necessary, not sufficient, for physical input ownership. R103 **did not** demonstrate that a key consumed by a ClickGUI would be blocked from Minecraft’s ordinary LWJGL/Java gameplay input processing.

R104 introduces `include/slinkywinky/windows_menu_focus_fence.hpp` with a `MenuFocusFence` that accepts **only the already verified** `GameWindowFocus` statuses (from the R101/R103 exact WGL/HWND test). Its purpose is to establish **eligibility** and a strictly latching loss of focus, **not** to intercept or consume any event.

## Implemented native policy

- Every fence is bound to its constructing C++ thread. A foreign thread can receive only **Refused**, without mutating the original owner’s policy state.
- `begin(epoch, released_keys_and_buttons)` requires a positive, strictly increasing owner-session epoch **and explicit evidence that keys/buttons are released**. A stale/reused epoch or a held-input condition cannot rearm the fence.
- Background is explicitly **not foreground**. Three consecutive successful foreground observations are required before `ForegroundCandidate` becomes true; one or two foreground observations are never sufficient.
- A **background transition after foreground candidacy** permanently latches `FocusLost` for the current epoch. An invalid/ambiguous original-window WGL status permanently latches `Refused` for that epoch, including after the window becomes visible/unique again.
- New foreground observations alone never clear either latch; an explicit *new epoch* and released keys/buttons are mandatory to start a new candidate lifecycle.
- `can_consume_game_input()` returns **false under all circumstances** in this R104 version. This is deliberate: there is no demonstrated native-to-Minecraft modal input route, no guarantee of avoiding input pass-through, and no user-originated Insert or mouse certification. An operator/synthetic code path must not treat the `ForegroundCandidate` classification as permission to activate a game module or swallow Minecraft gameplay events.

The first-party C++ type is header-only and does not add a new JNI exported input method, window-procedure subclass, LWJGL hook, GetAsyncKeyState poll, SetForegroundWindow, SendInput or keyboard/mouse dispatch to the original client.

## Evidence

1. New `tests/test_windows_menu_focus_fence.cpp` runs entirely on a disposable first-party C++ object. It covers absent/invalid epoch, held keys, two/three foreground sample separation, background stability, loss-of-focus lockout, wrong thread refusal without corrupting the owner state, non-reused epochs, invalid/ambiguous window refusal and reset, and unconditional zero gameplay input authority.
2. Existing `tests/test_windows_menu_gl11.cpp` independently connects the fence to **real** `audit_game_render_window` Win32/WGL observations from an isolated scratch OpenGL context. It checks three actual OS foreground/background observations without assuming what the hosted runner’s foreground window is. A duplicate same-title root HWND produces a native refusal and latches the fence; restoring uniqueness does **not** rearm until a new epoch and verified released-input condition. The GL state restoration checks remain unchanged.
3. CMake registers the new `native_menu_focus_loss_rearm_no_input` CTest independently of `isolated_wgl_menu_state_restore`. Both compiled and passed on the authorized local Windows MSVC build.

**Scope limits:** These are **isolated WGL + policy tests**, not fresh R104 foreground samples from the real Mojang JVM. The last independently verified original-client foreground sample remains R103's **0/8 foreground, 8/8 background** result. No actual user input was sent/consumed in the original game.

## Next requirements (not yet satisfied)

Before any in-game keyboard/mouse toggle or ClickGUI modal event handler can be enabled: verify exact original user-owned foreground while running; install a clearly attributed UI-owned input route; independently prove the same key/click **cannot** be delivered to normal Minecraft gameplay input while the overlay is visible; enforce key-release/rearm, focus loss, resize, root HWND recreation, native context changes, unload, button down/up matching, and no stale window focus. A real visible on-screen menu screenshot and tested end-to-end navigation are separate admission requirements.

This preflight is deliberately conservative. R100's operator-controlled inert in-game menu remains distinct from any physical input route. Original Slinky gameplay features are not reconstructed or certified by R104. PR #1 must stay draft/unmerged.
