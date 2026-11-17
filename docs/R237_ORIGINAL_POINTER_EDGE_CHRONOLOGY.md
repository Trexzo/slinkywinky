# R237 — Original Minecraft read-only source-pointer edge chronology

Date: 2026-11-17. Chat 1, `reconstruction/r1-core-fidelity` / draft PR #1.

## Why the original runtime needs this

R188/R189 verified exact WGL/HWND foreground samples and cursor hover, with pressed buttons blocked from certifying hover. They did not distinguish whether the *original game's render thread* sampled a left press followed by a left release on the identical, already-painted Auto Block > Main > Range slider. That missing chronology is useful for supervised original-client acceptance before attempting any reversible input interception.

## Native implementation

- Add a first-party `OriginalPointerEdgeTrace` which accepts **only** the original renderer's R188 before/after focus-reconciled observations, current Win32 button state sampled without removing messages, and the R179 exact painted slider target.
- Require an exact-source, clean all-button-released foreground observation before accepting a subsequent left-down sample, and require a left-up sample over the same source target before counting one pair. A button that was already down on entry is never a valid new press.
- Leaving the source target or chording cancels an unfinished press; losing focus during a held press **revokes that observation session** so a later foreground release cannot be counted. No sampled button edge changes the GUI, profile or Minecraft.
- The sample chronology is held beside the JNI original-render retained-preview owner, **not** inside `MenuState` or `WglRetainedMenuPreview`; original native/JNI object layouts remain unchanged.
- Expose only five integer aggregate counters to Java (samples, foreground, left-down, matched releases, cancelled presses) under the existing exact WGL owner admission. At 64 retained frames, require 63 samples, same exact-foreground total as the R183 independent window trace, and conservation of completed/cancelled presses.
- Ensure the trace is destroyed when the retained or operator session closes. Keep `nativeClickDeliveryEnabled() == false`; no physical click, window subclassing, LWJGL event interception or module activation is enabled.

## Verification boundary

The new Windows native regression simulates original-state samples and verifies clean arming, held-on-entry rejection, complete pairs, leave-target/chord cancellation, focus-loss revocation, foreign-thread rejection and non-delivery. Existing Windows JNI, Java8 Attach and original renderer build tests must remain passing. **A sampled Win32 GetKeyState edge is not proof of an LWJGL `Mouse.next()` event or hardware origin.** No physical-input acceptance can be claimed while the authorized desktop device is offline.

No modification to Chat 2 PR #2. Draft/unmerged until actual original-client input ownership and reversible restoration are independently certified.

## CI

Pending exact-commit verification.
