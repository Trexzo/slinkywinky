# R220 — Original Slinky ClickGUI range-slider crossing fidelity

Date: 2026-11-15. Chat 1 source branch `reconstruction/r1-core-fidelity`, draft PR #1.

## Fidelity gap from the pinned C++ GUI reference

In the independently reviewed pinned OpenSlinky C++ reference
(`Skid-Technology/OpenSlinky@fd9c592`), the range-row native handler
in `src/engine/gui/gui_controls.cpp` maintains two knobs. Dragging the
lower knob beyond the current upper endpoint **pushes the upper endpoint
to the newly dragged lower value**. Conversely, dragging the upper knob
below the current lower endpoint **pushes the lower endpoint downward**.

Before R220, our own `MenuState::set_recovered_range_preview` in
`src/menu.cpp` **clamped the dragged endpoint to the other knob**.
That made a real, concrete ClickGUI interaction behave unlike the
reference, even though visual paints and existing bounded hit-tests
were otherwise correct.

R220 independently implements the verified two-way endpoint push in
the first-party native C++ client preview model. It preserves:
- source-indexed range widgets, exact expanded-page visibility and
  matched catalog/name admission;
- existing quantized source step, min/max bounds and finite-value
  rejection before any state change;
- the exact original handle chosen at mouse DOWN through the existing
  `MenuMouseGesture` pinned-frame release check;
- the invariant `minimum <= lower <= upper <= maximum`;
- independent local GUI preview only, no runtime gameplay, persistence,
  OS input hooks or commercial source code copied into this repository.

The focused C++ test now verifies lower-crossing pushes upper, upper
crossing pushes lower, step/bounds, wrong control identity, original
press-handle ownership despite crossing, and unchanged module-toggle
or profile state.

The original retained GL11 source-mouse fixture still asserted the
old R133 clamping semantics, so the first full CTest pass detected a
real, expected incompatibility. That assertion was revised to require
the native-reference pushed endpoint in **both** crossing directions,
without weakening input/frame or GL-state validation.

**Final isolated C++ GUI verification: 22/22 PASS**, including range
preview, numeric drag, real first-party retained GL11 source-mouse
transactions, WGL state restoration, color, dropdowns and profile GUI
fixtures. The complete native Release build succeeded; no client/game
launch was performed. Prior full CTest included one outdated assertion,
so the full 107-suite is not claimed green after this revision; the
affected C++ GUI subset was rerun and is fully green.

**No Minecraft process was launched** for R220. All checks are
source-level or standalone first-party native test binaries. This is
an interaction-parity improvement, **not a claim that the entire
original Slinky ClickGUI or module system is 1:1 reconstructed**.

Chat 2's separate draft PR #2 remains untouched and unmerged.
