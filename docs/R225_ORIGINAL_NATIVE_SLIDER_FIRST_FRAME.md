# R225 — Original native ClickGUI first-frame slider press and repeated-drag fidelity

Date: 2026-11-15. Chat 1 / reconstruction/r1-core-fidelity / draft PR #1.

## Original C++ reference behavior

The pinned OpenSlinky gui_controls.cpp draw_slider_track checks
clicked_inside_r_frame, sets its active flag and, in that same draw
call, evaluates the pointer position to update the slider value.
The R224 retained WGL bridge only initialized the preview on Down,
then updated its value on Move. An OS frame with Down but no Move
therefore displayed the stale thumb position until the next event.

## First-party repair

R225 shares the same source-validated numeric pointer-to-value
mapping for initial Down and subsequent Move. The exact pressed
widget, range handle, current WGL context/viewport and model-side
admission remain required. Numeric movement is still temporary
render state; the profile setter is called only after a valid Up.

The WGL regression now presses away from the initial source knob
then redraws without any Move and verifies the new knob position.
It then performs two separately repainted moves, including a
two-handle crossing, proving that the source-selected handle remains
owned and the opposite handle is pushed visually. At each phase the
original profile is byte-identical and the temporary preview is not
persisted to the range override. Only the final Up commits the
source-pinned value and clears the drag preview.

Scope: native first-party C++/WGL behavior, not live Minecraft
keyboard/mouse ownership, an original injected gameplay session,
or reconstructed combat modules. Chat 2's PR #2 is untouched.

Validation results are recorded only after actually running CTest.