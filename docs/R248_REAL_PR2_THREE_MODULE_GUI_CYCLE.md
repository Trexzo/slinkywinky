# R248 — actual PR2 three-module end-to-end offline GUI control

Date: 2026-11-20. PR #1 `reconstruction/r1-core-fidelity`, draft/unmerged.
PR #2 `integration/openslinky-fd9c592-reference` remains untouched.
Source pin: `7f457376058ad42e0a0e0323ed874d7e5b2ae03a`.

## Gap after R247

The cross-repo real-native test R247 proved No Slow enable/disable/
re-enable through PR1's source-driven GUI request and PR2's
`SprintObservationPreview`. It did not prove independent controls
for the other two R211 modules on the same real native manager, nor
a GUI command while the real manager has no active Minecraft world.

## R248 test, same real source, no separate mock

The optional R247 CMake PR2 source checkout now exposes another
standalone first-party executable `slinkywinky_r248_real_three_module_test`.
The dedicated GitHub workflow builds BOTH R247 and R248 against the
exact verified PR2 commit and runs both CTests.

R248 exercises the original native R211 three-module manager with
the PR1's actual `GuiModuleControlHandoff`,
`NativeModuleSnapshotAdapter` and `ModuleArrayListFeed`.

On a separate GUI owner thread, it drives source-catalog focused
Sprint, No Jump Delay and No Slow controls while the native owner
thread invokes the REAL native `request_module` and
`control_states` API. Following each acknowledgement, it checks
native decision differences from the actual engine:
- Sprint enabled -> proposed key event; disabled -> no proposal;
- No Jump Delay enabled -> proposed jump ticks 0; disabled -> none;
- No Slow bow multiplier 0.8 -> source-backed forward/strafe input
  proposals; disabled -> no source proposal.

It then proves native world-loss retires the three active states,
allows a source-owned requested No Slow toggle *out of world* without
inventing an active callback, and re-enters the world on a new generation
to recreate only the requested native subscribers. Additional toggles
after re-entry establish independence and correct ArrayList rows.
Foreign-thread requests and clean owner retirement are refused or
handled by the same verified PR1/PR2 contract.

This is **real native-source integration running invented observation
frames**. It is NOT the original Minecraft JVM, JNI attach, physical
input owner, actual player keyboard, packet transport, gameplay
write, or full Slinky module fidelity. No native source bytes from
PR2 are vendored into PR1; the root CMake build remains independent.
Nothing in PR2 is modified. The other unimplemented modules/callbacks
remain unverified.

## Exact-head CI outcome

To be recorded from the actual GitHub workflow, never presumed.
