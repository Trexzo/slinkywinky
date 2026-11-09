# R185 — fail-closed WGL render refusal and focus epoch fencing

**Branch:** `reconstruction/r1-core-fidelity` (draft PR #1; PR #2 unchanged).

## Why this matters

R180/R184 restricted passive hover and inert pointer gestures to the last *successfully painted* native WGL frame. However, `WglRetainedMenuPreview::draw` returned immediately on malformed owner parameters (null context/DC, source viewport mismatch, context mismatch, etc.) before invalidating that last frame. A subsequent call on the restored pinned WGL context could therefore still reach the earlier hit geometry without another successful GL11 paint. The same problem existed for a held preview press, which could survive a rejected redraw.

R185 makes a failed draw on the **correct** owner thread revoke the retained painted frame and pending gesture. It clears local hover hints and breaks both R182 foreground-admission chronology and R183 independently sampled OS focus chronology. A rejected paint does **not** increment any OS-focus observation counter: a failed draw is not a witnessed foreground/background/refused OS state. A new successful draw is required before another hit target or local model gesture is usable.

A draw call from a *foreign* thread is rejected **before** any mutation, so a non-owner cannot revoke an otherwise valid GUI owner's painted frame or held gesture.

The successful WGL draw route and source-specific slider hit-test stay intact. This is **not** a Minecraft input interception, OS focus automation, or permission to dispatch live clicks.

## Verification

- Disposable Windows native WGL integration: foreign-thread rejected draw leaves current source hit target intact; an invalid supplied viewport revokes it, and passive hover yields empty instead of an old numeric/source hit. The source target returns only after a successful redraw.
- Held native slider press: null-DC render refusal cancels the pending transaction; the following Up cannot modify settings. A redraw restores hover and source geometry without reverting the prior source-validated native profile.
- Real GL viewport mismatch: `glViewport(0,0,650,610)` with an admitted 676×630 preview produces a failed draw; restoring GL viewport alone does not revive the old clickable frame. Only another successful draw restores hit geometry.
- Paired **synthetic disposable** native WGL and pure C++ R183 tri-state tests assert that a focus loss followed by a failed paint and foreground observation is **not** counted as a regain or post-regain source hit. Real sample totals are preserved; no fabricated refused OS sample is emitted.
- All other Java8/JNI and original-client marker regressions remain part of the full Windows CTest suite.

This iteration does not establish original Minecraft 1.8.9 foreground interaction, an exclusive LWJGL event owner, hardware input, or 1:1 original Slinky modules. Keep live input delivery disabled and PR #1 draft and unmerged.
