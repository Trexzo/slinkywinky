# R239 — Original LWJGL2 pointer exit/chord cancellation and safe paired restoration

Date: 2026-11-17. Chat 1, draft `reconstruction/r1-core-fidelity` PR #1. Chat 2 PR #2 untouched.

## Found failure mode

The first-party Java 8 `MouseQueueOwner` was built for a future original vanilla Minecraft `ave.s()V` mouse-queue handoff. While in MODAL it treated **every** LWJGL2 mouse sample with an X/Y outside the client rectangle as a corrupt source: the queue entered REFUSED. But leaving the window while dragging is a normal mouse sequence, and a legitimate left-button release still needs to be quarantined and drained. There was also no explicit CANCEL when the left mouse was being dragged and a right/other button was pressed; the final in-client left UP could then incorrectly commit the original native menu's pending gesture.

## Corrected first-party Java input-owner behavior

- Introduce a CANCEL=3 phase, matching the existing native `RetainedPointerPhase::Cancel`. It sends only fixed `(-1,-1)` coordinates; no off-window pointer position is forwarded.
- Track `pointerActive` separately from real held-button state. Only a fresh in-client left DOWN with no other button held creates an actionable local GUI gesture. Outside left DOWN remains quarantined, even after the pointer reenters while held.
- Moving outside, a wheel event during an active pointer gesture, or pressing a different button sends a **single CANCEL** and permanently prevents a later move/up from reviving that gesture. A fresh valid left DOWN after all releases can create a new gesture.
- A left UP outside cancels rather than committing. In DRAINING, outside mouse releases still update the held buttons and let the owner return to VANILLA when the complete queue is empty. Only malformed button codes (-2, 8+) or source failures cause hard REFUSED.
- On focus loss, close and unbind, clear the pending pointer action. No original Minecraft mouse/keyboard method has been redefined; actual vanilla input dispatch/suppression remains disabled.

## Regression

Expand the existing Java 8 `MouseQueueOwnerSmoke` with outside press/reentry, held drag leaving window, return without replay, right-button chord cancellation, legitimate post-cancel click, outside drain, malformed button refusal, callback exception and bounded flood. Add an `R165ModalInputSessionSmoke` paired mouse/keyboard source-owner cycle that consumes DOWN/CANCEL, then drains and restores both queues. These reuse the same Java8 first-party `MouseQueueOwner` that would be used by a future source-pinned original-client input handoff.

The build and CTest harness already registers both standalone fixtures; no new mock-only classes are packaged into the production agent. The permanent actual original-game input ownership gate remains disabled until live bytecode-restoration, alternate input-consumer coverage and foreground hardware acceptance can be verified.

## Exact-commit CI verification

R239 corrected code/test commit `fade8dea49fbe28af484046b052ae72e006dd645`: **8/8 GitHub checks passed**:

- Core native C++ Windows Debug/Release, Ubuntu Debug/Release, Linux sanitizers and JNI smoke: **6/6 success** — https://github.com/Trexzo/slinkywinky/actions/runs/44071847385
- Windows native JNI bridge: **success** — https://github.com/Trexzo/slinkywinky/actions/runs/44071847580
- Windows standalone Java 8 native loader and actual Attach API regression suite: **success** — https://github.com/Trexzo/slinkywinky/actions/runs/44071847589

The first implementation commit `893b2955296dee151779823f7dd23acb5efcba79` had one expected older-fixture failure: its R160 "normal drag" test still inserted a wheel/right-button press *before* left Up, which now correctly cancels the gesture. Corrected follow-up `fade8de` moved those unrelated events *after* valid Up; the new dedicated outside/chord cancellation tests remain intact and the full CI passed. Production `MouseQueueOwner` logic did not change between the two commits.

No original Minecraft live physical input or double-dispatch test is claimed. This improves the future handoff's source-owner cancellation and restoration semantics but does not activate an original `ave.s()V` input hook, grant exclusive input ownership, or establish restored game handlers. PR #1 remains draft/unmerged; PR #2 untouched.
