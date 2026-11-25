# R329 — remaining four native-registration module policies

**Candidate source inspected directly:** the user-owned `c++_possibly_OpenSlinky-main.zip`, paths `src/logic/combat/backtrack.cpp`, `lag_range.cpp`, `timer_range.cpp`, and `src/logic/move/inv_walk.cpp`, including their metadata headers. The larger decrypted binary archive was recovered, but this update does not assert instruction-level equivalence or provenance of the candidate C++ reconstruction.

Partial value-only C++17 controllers (no Java references, network buffers, GL, or input writes):
- **Backtrack:** candidate 20-tick attack eligibility, hurt/range filtering, final cooldown tick, target change and closer-target release, feedback passthrough, teleport/self-velocity stop, inclusive delay and once-per-sequence movement book-keeping.
- **Lag Range:** candidate 10–15 block sampled range, weapon gating, 15-tick splash-potion pause, sprint-release and attack-release flags, relative distance assessment from outgoing movement samples, strict budget, optional third-person render flag.
- **Timer Range:** candidate two modes (freeze before / after), ground/mouse/weapon/item-use/forward eligibility, bounded supplied ray-step reach plans, frozen packet gate, timer/use-key restoration, cooldown, tick-burst click proposal. The actual `horizontal_motion`/target interpolation/AABB simulation is not yet ported; the host provides ray-step evidence instead.
- **Inv Walk:** candidate silent inventory screen states, client-status/click/close packet admission, bounded 32-entry count-only queue, screen/physical-key restores, replay-gated flush and inventory-movement state. The model never retains Java global refs or sends a packet.

These are limited, testable **offline policy reconstructions**, not functional injected Minecraft modules. Settings, event subscriptions, original RNG, networking, re-entrant packet ordering, real resource ownership, GL, or GUI wiring are not completed. Inv Walk deliberately refuses queue overflow rather than silently losing an item; the candidate silently drops excess. Cross-thread safety requires integrating the existing owner/packet queue correctly before any hook can be enabled.

R329 adds CTest, standalone header compilation via R316, coverage-evidence registration and matrix reconciliation. This makes all **66 original pointer-table modules** present in one of the two inventory categories (partial controller or experimental model). **It does NOT make 66 modules playable or prove binary fidelity.** The 67th unique descriptor, Chest Stealer, is outside the original active pointer table. PR #1 unchanged; PR #2 remains draft/unmerged.

Acceptance gates: GitHub Windows and Ubuntu Debug/Release and sanitizers, source audit, then exact-event integration and real vanilla Minecraft 1.8.9 client-thread lifecycle verification.
