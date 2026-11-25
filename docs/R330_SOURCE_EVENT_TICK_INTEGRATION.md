# R330 — source-informed tick controllers bound to the existing event replay

Previous R329 added partial offline policy controllers for the remaining four original-table modules. R330 **does not add another manager**: it installs four typed owner-bound tick callbacks inside existing `ModuleExecutionBridge` / `EventReplay`, including module-state gating, source descriptor-name/category/complete slot-set admission, value-only tick proposals and explicit disable/world/stop resets.

| Module | Original candidate slot | R330 bound callback |
| --- | --- | --- |
| Backtrack | +0x168 = event 18 | target eligibility/cooldown tick |
| Lag Range | +0x168 = event 18 | delay eligibility/sprint/splash tick |
| Timer Range | +0x48 = event 0 | burst eligibility tick |
| Inv Walk | +0x1d8 = event 25 | inventory/screen key decision tick |

Original slot sets checked for each registered module: Backtrack 2/6/18; Lag Range 3/6/15/18; Timer Range 0/1/3/5; Inv Walk 5/17/23/25. Subscription **priorities are not yet authenticated** and are supplied by the tested catalog. Only the tick callback is implemented per module; other registered source callbacks are intentionally unbound. Valid same-event Backtrack+Lag Range frame dispatch is supported; mixed-event typed frames are rejected.

R330 copies candidate module state before dispatch, restoring it if a later module rejects the frame or a callback throws. The existing native source RNG branch also invokes this rollback. The lifecycle reset paths cover disable, detached world and bridge stop. This is offline C++ event replay; no JVM hook, real packet buffer, OpenGL menu, physical keyboard write, production injector or original commercial 1:1 claim.

CTest `r330_four_source_tick_event_bindings` checks descriptor admission, callback priority ordering, combined event18, wrong slot rejection, disable/reset, and inter-module rollback. PR #1 not changed; PR #2 remains draft/unmerged. Full native injectable parity still requires verifying the other callback subscriptions, exact priority metadata and game-thread ownership.
