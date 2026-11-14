# R201 — Timer reference comparison and independent offline reconstruction

Date: 2026-11-14. PR #2 remains draft; PR #1 is untouched.

## Three-source reference evidence (read-only)

The user has supplied or previously stored several candidate Slinky reconstructions.
Their code and executables are treated as **unverified third-party reference
material**, not official source or redistribution grants.

The following archives were inspected offline as ZIP entries only; no
packaged JAR, DLL, EXE, injector or Minecraft client was executed:

- Java archive `OpenSlinky-main(2).zip`: SHA-256
  `364846830fcc3820b64eafe3ebd0639f71e9dafcf06d6f9fad20170f278afd6c`.
  This archive includes 270 Java files.
- C++ archive `c++_possibly_OpenSlinky-main.zip`: SHA-256
  `c06b4c6033e2d19de86bad8f19127b0a3a68cfc496130233ebc9ff29f4540915`.
  This is the same ZIP hash recorded in the older forensic audit.
- Older C++ archive `slinky-main (1)(1).zip`: SHA-256
  `073b77d2bb77ce5cccf9eff57aa940eb558d425083cae4acd744a837d9e228b1`.

The **Timer C++ source file** in both C++ archive versions is exactly
2,665 bytes and has matching SHA-256
`038600db8fd8683d76d24c39089ddfe7da2eafc1fccb5ec6a880cf950aefb723`.
The pinned public GitHub source for the current integration uses the
same general Timer contract, but source hash identity alone is not
evidence of original commercial-client authenticity.

The Java `TimerModule` independently describes frame-event 1 behavior:
it selects the configured speed when requirements on mouse press,
downward movement and damage are satisfied, otherwise returns normal
speed 1.0. It also explicitly resets timer speed on disable/world unload.

The pinned public C++ source has TWO native callback slots recorded in
its linked data: a frame callback at event slot 1, priority 200; and a
toggle notification at event slot 5, priority 0. The C++ implementation
selects configured speed when those same three conditions pass, and
resets speed to 1.0 when the notification names Timer and switches it off.
Its default speed is 1.0.

Important differences:
- Java listens only to event 1 and expresses reset via its own disable
  lifecycle, while C++ registers the reset as event-5 callback. This
  does **not** prove identical ordering or lifecycle in a real JVM.
- The C++ source's selected speed is not globally gated on in-game
  state unless the damage requirement is active. The new first-party
  R201 controller intentionally fails closed to speed 1.0 when world
  or player is absent, or speed is invalid. Those are documented
  defensive divergences, not exact native parity.
- The R201 standalone bridge simulates event-5 reset input. The current
  offline EventReplay dispatch does NOT automatically guarantee the
  Timer callback executes after a real native disable; the in-game
  lifecycle and original callback ownership remain unverified.

## Independent first-party implementation (no third-party code copied)

Added:
- `reference_compat/include/slinkywinky/timer_controller.hpp`
  for typed frame and toggle inputs and data-only output proposals.
- `reference_compat/tests/test_timer_bridge.cpp` to check threshold
  conditions, damage/ground/mouse gating, 1.0 speed reset,
  non-finite settings, wrong event/payload rejection, metadata
  category/priority validation and callback activation rules.
- `reference_compat/tests/fixtures/seventeen_modules_with_fourteen_decisions.tsv`
  with synthetic Timer event 1/200 and event 5/0.
- Three synthetic negative fixtures for wrong category, priority
  and missing toggle subscription.

Updated the independent `ModuleExecutionBridge` to register both
Timer callbacks atomically only when candidate metadata matches
the pinned event/priority/category. The coverage-audit tool now
requires the Timer pair in authenticated 66-module metadata and
reconciles the expected **14 module offline policies, 17 executable
callback bindings and 154 unimplemented subscriptions out of 171**.

These figures refer only to **offline first-party policies**.
They are not a declaration that Timer now works inside Minecraft,
that all 14 policies are original-algorithm exact, or that 14 complete
modules can be used in a live client.

## Verification status and remaining work

The new r201_timer_frame_and_toggle_offline test and
r201_fourteen_decision_coverage synthetic fixture are registered in
the cross-platform Debug/Release reference CTest suite. All three
negative controls are marked WILL_FAIL and must reject invalid
catalog definitions.

At authoring time, final-head GitHub CI remains to be confirmed.
The authorized Windows machine was offline, so there has been
no live Minecraft test or independent original process receipt for
Timer. If CI passes, it proves the isolated offline implementation
compiles and passes declared inputs, not complete commercial parity.

Next gates: actual world/player lifecycle, automatic native event-5
disable reset, owner-thread sandbox callback semantics, settings
persistence, original GUI behavior and protected unload/reinject.
Source licensing, original-binary provenance and game-server rules
remain independent constraints. PR #2 stays draft/unmerged.
