# R165 — paired Java 8 GUI input session, still test-only

**Target:** `Trexzo/slinkywinky`, branch `reconstruction/r1-core-fidelity`, draft PR #1. This milestone is an **isolated original-Java-8-compatible fixture**. No real Minecraft input hook, key interception, packet mutation or gameplay module is activated.

## Why coordinated input ownership matters

The independently SHA-pinned original Minecraft 1.8.9 method `ave.s()V` uses 58 input-related callsites beyond a single mouse/keyboard queue, including direct keyboard state and held-keybind polling. R164 validated an **offline-only** whole-method conditional early-return gate. R110 and R160 independently model a Java 8 LWJGL2 keyboard and mouse queue owner. Independently passing tests did not establish safe behavior when one of those owners fails while the other is still modal.

R165 adds `R165ModalInputSession` and `R165ModalInputSessionSmoke` **under `tests/java` only**. It coordinates the two existing queue owner models with R164's artificial `R164OfflineModalGate`, and R161's synthetic 16-bit coverage ledger, as an ordered session:

1. Bind both mock LWJGL2 classes on the same calling thread; reject missing classloader/source gate.
2. Require source pin, increasing epoch, complete synthetic coverage, visible UI, simulated foreground, released physical input and a whole-method-entry gate.
3. Switch the test gate to **quarantined before** arming either queue. Arm mouse and then keyboard. If keyboard admission fails, close and drain the mouse before relaxing the gate; otherwise enter lockout.
4. During a modal sample, check foreground and both owners, drain both mock queues and forward only their accepted test callbacks.
5. Closing the overlay enters **DRAINING**, leaving the test gate quarantined until both owners have returned to VANILLA, all input is independently marked released, the source queues are empty and the fixture coverage ledger closes.
6. Focus loss or one callback failure revokes **both** queue owners and enters LOCKOUT. The gate stays quarantined through any stale releases. Restoration requires separate, independently supplied original-byte/physical-release/menu-closed flags for both sources, plus empty queues.

## Verified scenarios

The Java 8 regression tests ordinary VANILLA → MODAL → VANILLA execution, left-mouse coordinates and movement, keyboard edge capture, rejection of a second modal session, cross-thread refusal, partial keyboard admission with compensating mouse drain, unreleased input at close, an independently withheld empty-queue receipt, foreground loss, stale queued mouse/keyboard releases, and a failing mouse callback that formerly left the keyboard owner modal. The latter was corrected by revoking both owners in the common failure path.

The regression also verifies that a reissued source epoch is rejected and that a higher epoch can be admitted after a compensated partial arm. All changes are source-only Java tests plus CTest wiring and a production-input source guard. The attach-package checker requires that neither the coordinator nor its smoke test is included in the instrument JAR **or** the separate original-source audit JAR.

## Important limitations

The booleans `wholeMethodCutInstalled`, `foregroundVerified`, `allPhysicalReleased` and the R161 coverage bits are **provided by a disposable test**, not an authenticated original-game runtime. `R164OfflineModalGate` is a Java boolean that is not installed in the Mojang class. Releasing it cannot substitute for verified restoration of live loaded bytecode. Likewise `MouseQueueOwner` and `KeyboardQueueOwner` use mocked LWJGL sources in this test. This work **does not prove a genuinely atomic multi-queue transaction**; it implements fail-closed ordering and compensation, which must still be evaluated under actual client event scheduling.

R119's CI source guard explicitly forbids production input-owner calls and references to the R164/R165 synthetic gate. No injector/client behavior is changed. Before a live GUI can receive Minecraft input, independent original-client acceptance must establish source-pinned whole-method isolation, correct game/render thread handoff, foreground and physical-key proof, queue exhaustion across close, safe loss-of-focus handling, and actual loaded-class restoration. The remaining source-catalog modules are not complete.
