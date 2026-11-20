# R238 — native Clutch source world receipt admission (offline)

Date: 2026-11-20. SlinkyWinky draft PR #2.

R237 added first-party Java 8 sampling of the real Minecraft 1.8.9
`WorldClient` block IDs in *pinned 167-source-offset order*.
The raw decrypted candidate ZIP is not available in the current session,
so we reuse the already-audited pinned table and reference algorithm.
No commercial source or bytecode is copied into this change.

## New independent native boundary

`clutch_observation_bridge.hpp` accepts a typed, bounded
`ClutchObservedFrame` representing one R237 Java block-ID snapshot.
It validates **before** building a `ClutchCandidateWorld`:
- Nonzero opaque world/player identity, exact generation, strictly
  increasing tick, and the creating C++ owner's thread;
- finite and bounded player AABB/eye, and the exact base coordinate
  formula derived from the R235 original;
- the *exact prefix length* for the configured original range index;
- every sampled block ID between 0 and 4095 (0 means air), missing
  evidence or incorrect ground-depth ordering;
- the early ground-solid short circuit before source candidate reads;
- prior candidate's conditional block-solid observation, with no
  fake `ray_known` or successful ray-hit evidence.

A failure quarantines the owner permanently; the caller must create an
entirely new, verified world/session binding to retry. The adapted
`ClutchCandidateWorld` contains **value-only** evidence and never
retains any Java object or raw JVM pointer.

## Source planner regression

The R238 native test drives the existing actual R235
`clutch_plan_original()` from an admitted six-block R237-like snapshot.
It verifies selection of block (1,63,0) at source index 5 and checks:
- replayed tick, wrong world/player/generation, wrong native thread;
- malformed base, prefix, block ID and ground observation order;
- early ground observation rejects candidate selection without RNG writes;
- prior-block certainty never implies verified raycast; the native
  planner refuses an eligible un-rayed previous block without advancing
  RNG, rather than granting a fabricated placement;
- a genuinely observed air prior can be bypassed for a source-ordered
  fallback candidate.

Targeted cross-platform CMake test:
`r238_native_clutch_observation_planner_gate` (Clang/GCC Debug/Release).

## Important missing piece

This test uses **synthetic typed receipts**. It is NOT yet a C++ JNI
endpoint and does not establish actual in-game Clutch execution. R239
must implement a strictly owner-checked Java/JNI snapshot transport
on the actual Minecraft client tick, including source-timed prior
raycast evidence and the shared R234 native RNG transaction. The
R237 Forge adapter and R238 native gate currently exist as separate
verified source components.

The game-owning Windows desktop was offline when this change was
authored. There is no fresh real Forge/vanilla acceptance, source-
faithful click/rotation/placement, completed client or authenticated
decrypted DLL comparison. Keep both PRs draft, independent and unmerged.
