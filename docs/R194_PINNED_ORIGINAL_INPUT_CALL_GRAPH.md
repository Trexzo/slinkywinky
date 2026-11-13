# R194 — exact-declared-target reverse input call graph of original Minecraft 1.8.9

**Project:** `Trexzo/slinkywinky` | `reconstruction/r1-core-fidelity` | open draft PR #1. The separate OpenSlinky C++ comparison PR #2 is not edited.

## Why R194 matters

R171 proved that a proposed early return from `ave.s()V` would be an unsafe shortcut. It enumerated **146** direct LWJGL2 Mouse/Keyboard and original `avb` key-binding invocations from the exact Mojang 1.8.9 JAR, of which **96 are outside `ave.s()V`**. Direct-site enumeration still does not say which *callers* can reach these input-consumer methods.

R194 implements an independent read-only Java 8/ASM 5.2 caller-to-declared-callee graph. It scans every method invocation in each original class, selects the **exact method ID** `owner.name(descriptor)` as an edge, then performs a reverse breadth-first traversal from methods containing R171-matched direct input API references. This is a static, *declared-target-only* reachability model: it is not a control-flow-sensitive taint model, a whole Java runtime dependency graph, a live observer, or any evidence of an actual original Minecraft keyboard/mouse event.

## Verified exact-source metrics

The source was the **SHA-1 pinned 8,461,484-byte official Minecraft 1.8.9 client JAR**, previously audited in R171. R194 invokes the same pinned-source admission and independently reproduces its class and direct-callsite counts. The original JAR was verified byte-identical before/after the analysis.

| Measure | Verified value |
|---|---:|
| Classes analyzed | **2,507** |
| Methods identified | **19,766** |
| Direct input API invocation sites | **146** |
| Distinct methods containing these sites | **56** |
| Additional 1-hop upstream calling methods | **46** |
| Additional 2+-hop upstream calling methods | **135** |
| Total methods reachable in reverse traversal (including direct) | **237** |
| Longest found exact-declared-target reverse path | **6** |
| Unique edges targeting methods inside the source JAR | **37,813** |
| Unique edges with targets not declared in that JAR | **21,806** |
| `ave.s()V` distance to an input site | **0** (it directly calls input) |

Deterministic graph-reachability SHA-256 (source/order independently verified):

`f0b1cb8aee451b56302a0035ec92665829c774a251718b4e00f61852d9e3bdfd`

These values describe **exact declared call edges**, not the actual number of input actions, actual paths taken at runtime, or an upper bound on possible consumers. A direct API call can, for example, query input labels or cursor state without initiating gameplay.

## Negative controls

The separate, disposable Java ASM smoke test constructs a chain `Grand.run → Parent.run → Leaf.run → Mouse.isButtonDown`. The expected reverse distances are 2/1/0. Reordering source classes preserves the graph fingerprint. Mutating the leaf's direct Mouse call to an unrelated class removes all three reachable methods and changes the fingerprint. An unresolved external call target does not invent a nonexistent method body.

A **deliberately unresolved virtual dispatch** fixture is also tested: the call is declared on `Base.run` while the Mouse-reading override lives in `Sub.run`. The declared-target traversal does *not* claim `Call.run` is reachable, demonstrating an important **under-approximation**. Similar blind spots include interface dispatch, invokedynamic, reflection, native methods, external libraries, control flow and OS-level input paths. Thus `proofOfExclusiveInput()` **always returns false**.

## Packaging and release safety

R194 source and smoke classes are compiled exclusively into `slinkywinky_render_audit.jar` / disposable Java test outputs, never the injectable `slinkywinky_instrument.jar`. `tests/attach_smoke.py` checks this separation. The CTest `r194_java8_original_input_call_reachability_fixture` verifies all synthetic negative controls, and `tests/official_189_render_audit.py` now checks the pinned-source graph fingerprint in the official offline original-client audit pipeline.

**No Minecraft input hooks, generated clicks, game actions, keyboard suppression, game process modifications or runtime event ownership were introduced.** Native live click delivery stays disabled. The original Mojang JAR is not bundled or committed.

## Verified local acceptance

The full Windows x64 Release build succeeded, and the complete CTest suite passed **94/94 tests**, including the new R194 Java 8 graph fixture, packaging isolation and existing JVM attach/WGL regressions. The rebuilt `slinkywinky_render_audit.jar` independently reproduced the pinned **237-method** original call graph and the graph SHA-256 above. Both the synthetic virtual-dispatch negative control and pinned-source byte-identical-before/after test passed. The modified official-original-audit Python script passed syntax compilation; its full CI download workflow was not claimed as run.

A separate read-only **original vanilla Minecraft 1.8.9** launch passed six restoration/admission stages with **64** retained GL11 frames, **60** source-visible slider draws and **63** passive observations. Original Win32 focus evidence remained `foreground=0 background=63 refused=0 losses=0 regains=0 post_hits=0`; exact source-hover hits remained zero and `native_click_delivery=disabled`. This is **background-only renderer acceptance**, not an input-interaction test.

## What this still does not prove

The strict original client foreground cursor/focus-cycle test has not passed with the game genuinely foreground and operator-driven hardware input. Native WGL ClickGUI, R190–R193 fixture mouse/keyboard transactions and their restoration tests remain distinct from real exclusive ownership of the Mojang input graph.

Before enabling interactive native ClickGUI, the project must either resolve all relevant runtime consumer paths (including polymorphism) with authenticated **original** event ownership and precise bytecode/class restoration or explicitly keep unsafe paths out of scope. This R194 analysis is an evidence-gathering stage, **not authorization to suppress vanilla input**. The commercial Slinky GUI 1:1 visual fidelity and gameplay-module reconstruction are still incomplete. Keep PR #1 draft/unmerged.
