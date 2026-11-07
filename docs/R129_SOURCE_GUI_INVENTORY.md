# R129 — source-pinned catalog coverage guard

This is a **read-only provenance/coverage test**, not module reconstruction or gameplay proof.

## Candidate reference

User-owned `decrypted files.zip`, archive SHA-256:
`326efa346905370ed1815aba2247d35666008346dd6858262c38070d0f42aea4`

Its `re/modules_real.json` provides 67 recovered module names/categories/descriptors: Combat 17, Move 10, Visual 15, Block 7, Utility 18. Six descriptors have no native GUI callback and are treated separately as engine-only: No Jump Delay, Ping Fix, No Hurt Cam, Instant Stop, Item Use Fix, Null Move.

Its R120-native registration audit identified 61 GUI callbacks and 424 W_* callsites. The current `src/recovered_settings.cpp` exposes 385 typed *fixture* widgets across exactly those 61 callback-bearing names. This distinction must remain explicit: a count-match of module names does **not** prove widgets/options/defaults/visibility match the original, and 385 is not the native widget-call count.

## New regression

`tests/test_gui_source_inventory.py` is portable and uses only the repository's source files, not the user-owned DLL. It checks that:

1. All 67 module name-category pairs match an independent, source-pinned descriptor fixture, with exactly the five expected category totals.
2. Every one of the 61 callback-bearing module names is represented in the typed GUI fixture, and none of the six engine-only descriptors is spuriously treated as having native GUI widgets.
3. There are still exactly 385 typed fixture widget records; unexpected changes must be investigated rather than silently passed.

CTest registers `native_source_gui_inventory_coverage` when a Python interpreter is available. It is a lightweight source-only guard that can run on Windows and other supported development platforms.

## Observed current HEAD

A direct GitHub source inspection confirmed **67 catalog descriptors** in the five expected categories and **385 typed widget entries** across **61 GUI module names**, with zero missing or extra module names. The full new CTest command and all Windows runtime/graphics tests remain **unverified** until actually executed.

## Remaining gates before GUI completion

- Resolve per-callsite registration coverage against the 424 native source calls, including conditionally displayed settings and exact choice lists.
- Implement remaining checkbox, numeric slider/range, multiselect, text, color picker, keybind and modal interactions with safe focus/gesture handling.
- Reconstruct source-grounded typography and animations; validate actual OpenGL screenshots at standard and wide/compact sizes.
- Implement profile listing/save/load/delete atomically with validation and without leaking actions into Minecraft.
- Demonstrate complete original vanilla 1.8.9 keyboard/mouse input ownership and restoration on the correct client thread.

**Only after GUI acceptance:** implement and test each of the 67 modules' actual behavior. Initial GUI preview bindings for seven modules are not gameplay equivalence. Keep PR draft/unmerged.
