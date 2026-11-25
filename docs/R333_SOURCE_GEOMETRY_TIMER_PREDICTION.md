# R333 — original-source geometry, motion and predictive ray simulation

R333 addresses two source fidelity gaps that R332 still delegated to externally supplied Boolean guesses:

1. **Timer Range forward prediction:** the pure C++ candidate logic now reconstructs yaw/pitch look vector, three-block segment versus 0.8×2.0×0.8 target AABB, distance to nearest AABB point, Minecraft movement push, 0.546 post-push drag, source freeze-before target walking, and server-position interpolation. A copy-only `TimerRangeSourceObservation` can be supplied to the existing e3 frame callback, which derives its own evidence steps before asking `TimerRangeController` for a burst decision. Giving both guessed ray steps and a source snapshot is rejected. Zero-horizontal target walk and invalid/nonfinite snapshots are fail-closed rather than copying source's undefined division. Simulation capped at 128 ticks.
2. **Backtrack / Lag Range render geometry:** source box 0.6×2.0, interpolation, camera translation, y+1 center, optional Backtrack yaw rotation, filled alpha 0.4, adjustable line width/color, no GL calls. Both use existing original e6 callback and can share one event6 frame only if camera and partial tick match.
3. **Lag Range attack phase:** source e15 callback uses original phase-one hit-this-tick latch for the next eligibility tick.

Source archive: user-owned candidate `c++_possibly_OpenSlinky-main.zip` inspected locally. Original decrypted binary callback registration priorities were pinned in R331. This is **source-shaped offline behavior**; no real client I/O, Java references, packet flush, GL context use or injection. The prediction is an approximation of floating-point/JVM native behavior: full 1:1 equivalence requires differential replay of native original and actual Minecraft 1.8.9 frames, including game event ordering, identical item/system state and original RNG.

R333 includes standalone C++17 regression tests for hitbox rays, prediction rejection, exact render geometry, source event6 dual render admission, event15 attack, snapshot-derived timer burst and untrusted mixed evidence rejection. Only PR #2 modified.
