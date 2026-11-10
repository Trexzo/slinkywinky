# R193 — late keyboard/mouse release quarantine after native ClickGUI close

**Target:** `Trexzo/slinkywinky`, branch `reconstruction/r1-core-fidelity`, open draft PR #1. Separate OpenSlinky reference reconstruction PR #2 is untouched.

## Motivating defect

R190's paired synthetic input ledger and R192's native WGL mouse+keyboard bridge could close an overlay while a key or left-button remained held. R191's `fixture_end_keyboard` set the native keyboard router to `Draining`, canceled the retained mouse gesture, and set the paired ledger to `Draining`. However the native model had no **post-window-close** path for the held mouse release: the regular `fixture_pointer_event` refused once the ledger entered Draining, and the retained preview's `close()` removed its WGL session.

The existing R190 `restore(...)` also trusted an externally asserted `all_controls_released=true` even during ordinary Draining with a **known model-held key/button**. That was too permissive for a deterministic test-fixture ledger. In a genuinely revoked session, a missing release cannot necessarily be recovered from the model, so a separate independently attested original dispatcher restoration and all-controls-release claim remains necessary.

## R193 code

- A native `fixture_drain_release(ledger,router,device,code)` accepts **only a release of a previously tracked synthetic key/button**, under the same owner thread, same R190 epoch and same R191 `MenuNavigation` instance, and only while the ledger is in `Draining`. It does not need the WGL preview to remain active, the viewport to be bound, or foreground focus. It never calls the normal pointer handler, hit-testing, native profile mutation, JNI, or original game keyboard/mouse state. The underlying synthetic ledger always returns `Quarantined`, including for valid releases; duplicate/untracked releases are rejected.
- A paired `fixture_finish_restore` requires an explicit `menu_closed=true`, `all_controls_released=true`, and `original_dispatch_restored=true` assertion, plus the same router/ledger epoch and source menu. Ordinary Draining now **also** refuses restoration if the ledger internally still tracks any held key/button, even when all three host booleans were passed. The keyboard router is returned to Idle first; the ledger only returns to Idle after the same attestation. This is a **synthetic fixture state reset**, not real Minecraft original-input restoration.
- Revoked sessions (e.g. a focus loss with an unpaired held key) can only recover with the full external attestation. They must not pretend a missing synthetic Up was observed; the host attestation is a separate condition, not evidence of real physical hardware.
- `fixture_idle()` is a read-only keyboard-model state query. All additions keep the binary layout of `WglRetainedMenuPreview` unchanged; the router and ledger remain caller-owned.
- The existing R119/R160 read-only safety test now explicitly forbids the R193 fixture APIs from appearing in the actual `jni_bridge.cpp` or Java 8 original game agent sources. **There is still no original game input hook, no vanilla event suppression, no event replay and no live click delivery.**

## Regression coverage

In `tests/test_windows_retained_pointer.cpp`, a full synthetic keyboard Down and modal close are followed by a **real WGL context detach** (`wglMakeCurrent(nullptr,nullptr)`). A foreign-thread release, untracked release and duplicate release are rejected; the authentic tracked synthetic Up is quarantined while GL is detached. Required external restoration flags are independently tested and, after successful ledger/router recovery, the same WGL context is rebound without changing the saved GL state.

A second case holds a source-verified native toggle mouse press without an Up, closes the GUI model, then drains the late mouse release. It proves the native toggle remains disabled and the late Up cannot commit the already-canceled gesture, despite the matching original coordinates. A third case loses focus with a held key, enters Revoked, rejects a drain-only release, and recovers only with separately attested full restoration.

The portable R190 ledger test now rejects `restore(true,true,true)` when a known held key/button remains during ordinary Draining, even with a host claim to the contrary. No **original Minecraft** hardware click/release occurs in these tests.

## Outstanding release gates

**Do not interpret the R193 fixture results as real input exclusivity or actual Minecraft restore.** The existing exact-source R171 audit finds 146 direct input API references in vanilla Minecraft 1.8.9, 96 outside the main `ave.s()V` input method. Original in-game foreground input ownership, full transitive consumer exclusion, physical release checks, real class and key-state restoration, a 1:1 original Slinky ClickGUI screenshot comparison and all gameplay modules remain unverified/incomplete. The native click and original input modification paths stay disabled; PR #1 remains draft/unmerged.

## Verified Windows/original-client results

The Windows x64 Release build passed and the full native / Java 8 / JNI / agent-attach CTest suite passed **93/93 tests**, including R190's modified restore-negative control and the R193 closed-WGL release test. The separate isolated original vanilla Minecraft 1.8.9 run exited successfully with **six** restoration/admission stages, **64** retained GL11 frames, **60** source-visible slider draws and **63** passive input observations. Its original-JVM exact OS-focus receipt was `foreground=0 background=63 refused=0 losses=0 regains=0 post_hits=0`, with **zero** source-hover hits and `native_click_delivery=disabled`.

All actual-game observations remained in the background. These results **do not establish physical foreground release handling or live original input ownership**; the new post-close release tests are synthetic and the source/JNI production guard remains intact.
