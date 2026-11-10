# R189 — held mouse buttons cannot certify original source hover

**Scope:** SlinkyWinky native ClickGUI reconstruction, `reconstruction/r1-core-fidelity` and draft [PR #1](https://github.com/Trexzo/slinkywinky/pull/1). PR #2 / OpenSlinky reference C++ work is untouched.

## Defect

R188 brackets the original Win32 cursor read with exact original-client WGL/HWND foreground observations and conservatively classifies disagreements as Refused. However, when the original-game cursor is physically over the source-matched Auto Block / Main / Range slider **while any mouse button is held**, that passive sample could increment numeric and exact-source hover counters. These counters provide readiness evidence, not event ownership, but the stricter manually supervised R181/R182/R183 hover gate should not accept a source hit during a possible vanilla mouse press or held input.

The original read-only pointer receipt already includes the five-button `GetKeyState` mask for left/right/middle/X1/X2 buttons. This is not the LWJGL event queue and is based on the render thread's last dequeued Win32 message state.

## Implementation

- `original_pointer_released_for_hover` is a **pure read-only predicate**, true only for button mask zero. Any of the 31 nonzero five-button combinations, including chords, blocks certification of a source hover.
- `WglRetainedMenuPreview::observe_hover` accepts an explicit `buttons_released` flag. The actual original-render JNI callback **always supplies** the sampled value. A foreground cursor with any held button still counts as a real admitted foreground observation, but cannot set a visual numeric/bind hover hint, numeric-hit counter, or source-exact slider hit.
- A later no-buttons observation can restore a legitimate source hover in the same valid painted WGL frame; an intervening press clears an already-active visual hover and does not increase source hits. This is not a mouse release event and does not change a native profile.
- R183's independently audited exact OS foreground state remains separate. A held mouse button does not fabricate a focus loss, invalidate the window identity, or introduce a refusal by itself.
- The disposable R175 Java 8/JNI test fixture explicitly passes `buttons_released=true` for its synthetic hover calls; production no longer relies on the fixture default. **No original Minecraft click queue is intercepted, blocked or injected.**

## Verification

The disposable native WGL test calls the real retained ClickGUI model on source-verified painted slider geometry, checks 31 foreground samples with simulated held-state flags, and requires zero numeric/bind/exact-source hits. It checks the R183 focus counters remain foreground, then verifies a button-released sample produces exactly one numeric/exact-source hover, followed by a held sample that clears that visual hover while retaining the previous exact-hit total. The GL11 test checks all 32 possible five-button masks, including release.

The Windows x64 build passed. Full Java 8/native/JNI CTest passed **92/92 tests** after R189, including JNI/agent attach and the new button-mask/hover fixtures. A separately launched, bounded **original vanilla Minecraft 1.8.9** acceptance also exited successfully with six original-JVM restoration/admission stages, 64 retained frames, 60 source-visible draws, and 63 passive samples. Its exact original Win32 focus receipt was **foreground=0 background=63 refused=0 losses=0 regains=0 post_hits=0**, and exact source hits were **0**, with `native_click_delivery=disabled`. This is a background-only acceptance; it **does not prove physical foreground hover**, any original-game held-button event, or exclusive input. Even a successful operator-supervised foreground gate would establish read-only observed cursor/focus evidence, not hardware event origin, vanilla-event suppression, or native ClickGUI click ownership.

Live original Minecraft input dispatch stays disabled; the PR must remain draft/unmerged.
