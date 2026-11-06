# R105 — owner-thread-only focus-fence diagnostic reads

**State:** tested locally on Windows, draft PR #1 only. No Minecraft input hook, physical key consumption, or gameplay feature is enabled.

## Change

R104 already required the constructing thread for mutating focus state, state(), and foreground_candidate(). Four diagnostic getters (epoch(), foreground_samples(), background_samples(), denied_samples()) returned mutable fields without checking thread ownership. Concurrent calls from a different thread could create a C++ data race.

R105 applies the same owner-thread guard to every diagnostic getter. Calls on any non-owner thread return an inert zero without reading mutable session state. Existing owner-thread values are unchanged. The input-authority method remains unconditionally false.

## Regression

The native_menu_focus_loss_rearm_no_input test now exercises all four getters from a foreign thread after a nonzero epoch and foreground samples are present, while preserving the original session on the owner thread. The isolated WGL/GL11 regression is also re-run.

Local Windows MSVC Release: 20/20 native CTest PASS (including both targeted focus-fence and isolated WGL tests). This is a narrow data-race prevention improvement, not a foreground or keyboard-ownership test in original Minecraft.

## Remaining

An in-game user-originated Insert/GUI input route must still prove foreground ownership, matched press/release, loss-of-focus latch, and that the same key cannot pass through to normal Minecraft controls. Until separately certified, physical input consumption and combat/movement functionality remain disabled. Keep PR #1 draft and unmerged.
