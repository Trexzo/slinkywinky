# R79 — renderer admission-before-JNI and deterministic callback drain

**Purpose:** Fix a concrete R78 ordering hazard: the optional WGL JNI query could run before the R75 `RenderFrameGate` had validated the callback's render-owner Java thread. In a race or foreign-thread callback, this allowed a read-only GL query outside the admitted render thread.

## Changes

- `RenderCallbackFence` is a **pure Java 8**, synchronized guard binding one existing `RenderFrameGate` to one optional observation function. For each callback:
  1. Validate the exact `ave.av → bfk.a(FJ)V` stack, partial-tick arguments, monotonic-nonnegative frame-time requirement and Java owner thread through `RenderFrameGate.sample()`.
  2. Only if that specific invocation **increased** the accepted count and the gate did not invalidate may it issue the optional R78 native WGL/viewport snapshot.
  3. Catch observer exceptions without throwing into original game rendering. Record an observer failure and permanently stop the gate.
- The callback's `stopAndDrain()` uses the **same Java monitor** as admission and observation. When cleanup wakes after eight callbacks (or the finite 12-second timeout), it disables future callbacks and **joins any in-flight JNI observation** before initiating original `bfk` bytecode retransformation. Removed the old 100 ms sleep, which was neither a safe join nor a reliable restore boundary.
- Installation failure and worker startup failure also disable and drain callback reads before attempting restoration. After a failure, the owner gate is released **only** if exact pinned renderer restoration is independently proven, preserving R75's fail-closed behavior.
- The original R75 Java-only callback mode has no native observer and continues through the same fence. It is still bounded to eight frames with no GL calls, and both R75 and R78 pass markers refuse any callback-fence observer failure.

## Tests

`native_r79_admission_before_wgl_and_drain` (Windows Java 8 CTest) uses **no JNI or Minecraft**. A fake observer counts calls and asserts:
- Exactly eight validated samples, no ninth observation.
- Wrong Java owner, wrong `ave.av` stack, invalid partial ticks and negative frame time are rejected **before** any observer call.
- A synthetic native observer exception stops future callback admission and invalidates successful completion.
- An intentionally blocked observer causes cleanup to **wait for drain** even after `RenderFrameGate.await()` has reported completion; after drain, a stale callback cannot observe again.

R79 does not enable native menu drawing in the real game or change the pinned original-class fingerprints. It makes the already opt-in R78 read-only context diagnostic safer. CI evidence must be taken from the current commit and a real in-game test is still outstanding while the desktop is offline.
