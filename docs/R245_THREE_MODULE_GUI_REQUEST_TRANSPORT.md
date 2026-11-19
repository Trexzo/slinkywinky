# R245 — three-source-module GUI request transport

2026-11-19 | PR #1 `reconstruction/r1-core-fidelity`.

## Change

The source-backed PR #2 R211 Java8/JNI native manager exposes actual
request/owner-state control for `No Jump Delay`, `No Slow` and `Sprint`.
PR #1 previously admitted only Sprint in its GUI-to-runtime *request*
handoff. R245 admits the other two **names** with the same owner-thread,
source-sequence, world-generation, acknowledgment and feed requirements.

The exact focused source catalog item must be one of these three
names and pass R244's verified 66-entry native-manager identity fence.
The request captures the focused name, not hard-coded Sprint.
Neither a GUI preview enabled bit nor a requested state makes the
module active. A snapshot can confirm a requested module as inactive
when outside a world. Other registered names (e.g. Strafe) remain
ineligible until their native control path is implemented and verified.

## Regression

The existing `r210_gui_module_control_handoff` native test now
exercises No Jump Delay enable/disable with active-in-world acknowledgment
and No Slow requested/disable while out-of-world, using the source
catalog indices 21 and 22. It verifies preview-only flags cannot make
an active row, request-name continuity, and unsupported Strafe refusal.
Existing Sprint positive and negative regressions remain in place.

## Scope

No PR #2 source is copied or edited. The real PR #2 native manager is
not yet connected to this PR #1 runtime. This is a value-only GUI
request/ack model, not original Minecraft keyboard/mouse input,
gameplay effects, production injection or original commercial parity.
Do not count these as three fully working Slinky modules. PR #1
stays draft and unmerged.

## Exact-head CI results

Record GitHub Actions results only when they actually pass.
