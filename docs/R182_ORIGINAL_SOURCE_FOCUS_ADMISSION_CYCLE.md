# R182 — ordered foreground-admission loss/recovery telemetry

**Branch:** `reconstruction/r1-core-fidelity` (draft PR #1), separate
from the OpenSlinky C++ integration PR #2.

## Problem

R181 requires at least two original-JVM foreground samples hitting
the genuine last-painted Auto Block > Main > Range slider. But aggregate
R176/R179 counters cannot tell if these hits happened before or after
loss of foreground admission, and therefore do not establish restoration
of a visible hover after a focus interruption.

## Source and runtime change

The first-party C++ `WglRetainedMenuPreview` now records a strictly ordered
passive observation sequence on its pinned WGL render thread. Three
additional counters are retained per native session:

1. **losses**: a transition from an admitted foreground pointer sample
   to a sample without foreground admission.
2. **regains**: a subsequent transition back to foreground admission
   after a real prior foreground admission and intervening loss.
3. **post_hits**: source-exact Auto Block/Range hover observations that
   occurred **after** at least one regain, while foreground admission
   remained current.

An initial foreground hit does not count as recovery. Returning focus
alone does not count as an exact source hover. An out-of-client hover
is never an exact source hit. Loss of foreground admission continues
to clear displayed hover state; existing R180 last-painted geometry
fences still apply.

`source_focus_cycle_evidence` is owned by the same C++ thread, native
WGL context, DC and viewport as the original R179 evidence reader. Its
production JNI entry point returns exactly the three bounded integer
counters. Java 8 validates them against R176 and R179 before reporting:

`R182_ORIGINAL_NATIVE_FOCUS_CYCLE_PASS frames=64 losses=N regains=N post_hits=N native_click_delivery=disabled`

The R99 original-client acceptance reader now requires this additional
marker. **Zero cycles are valid passive evidence**, not a pass for recovery.

## Explicit manual acceptance

Add `--require-focus-cycle-source-hit` to the existing R181 manual
`--run --render-retained-acceptance --require-foreground-source-hits`
acceptance invocation, preserving `--workspace`, `--jdk8`, exact
original 1.8.9 installation/sha checks, and the other existing options.
The new flag refuses noninteractive mode and other render modes.

The operator needs to bring the original isolated game into foreground,
move over the visible source slider, then leave and regain foreground
and hit the slider again before the 64-frame probe ends. Source
geometry and counters are verified only after the original render
harness has independently checked loaded client identity, bounded
WGL session and restoration.

**Important limitation:** This is an observed **foreground-admission
loss/regain**, not independently authenticated OS focus transitions.
The underlying R172 passive pointer reader returns no observation
both when the window is genuinely backgrounded and when a pointer
admission check otherwise fails. Until an independent exact-window
tri-state focus trace is integrated and verified, an R182 post-regain
receipt must not be described as proof of actual OS focus restoration,
physical user input or deliberate user movement. The 64-frame window
also makes a completed manual focus cycle nontrivial; no live cycle
has been certified.

## Isolated tests

- A Java 8/JNI test drives synthetic foreground transitions through
  the **real native WGL retained renderer**. It verifies first hit,
  background clearing, reacquisition without hit, exact hit after
  reacquisition, repeated transitions, source counter agreement,
  foreign-thread rejection, wrong viewport, profile immutability and
  WGL teardown.
- A Java 8-only production receipt smoke test rejects malformed or
  contradictory count combinations.
- An original-process marker parser rejects duplicate, mismatched,
  background-only-as-recovery, non-source and click-enabled claims.
- Registered in CTest; the original Minecraft 1.8.9 executable is not
  distributed or stored in Git.

## Validation status

The connected Windows computer was offline at authoring time.
The new code/tests are source changes awaiting independent Windows
build, Java 8/CTest execution, CI and real original-client acceptance.
Do **not** mark runtime foreground restoration as PASS on the basis
of the synthetic tests alone.

Minecraft LWJGL input interception, mouse clicks, game packets and
combat-module execution remain disabled. PR #1 stays draft/unmerged.
