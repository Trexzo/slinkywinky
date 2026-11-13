# R195 — possible virtual/interface input dispatch in pinned Minecraft 1.8.9

**Repo:** `Trexzo/slinkywinky` | `reconstruction/r1-core-fidelity` | draft PR #1. The separate OpenSlinky C++ PR #2 is untouched.

## Why R194 was insufficient

R194 inspected the entire SHA-1-pinned original Minecraft 1.8.9 JAR and found 146 direct LWJGL2 Mouse/Keyboard/keybinding invocation sites in 56 distinct methods. Following exact *declared* method-call targets backwards from these methods reached 237 caller methods. This missed cases in which `Base.run()` or `Interface.run()` is the invoked target while the active receiver's implementation is a different subclass/interface implementer.

R195 adds a separate, **offline-only**, Java 8 / ASM 5.2 possible-dispatch graph. For a virtual/interface callsite it finds classes declared in the exact original JAR that are concrete and extend/implement the declared receiver type, resolves the nearest eligible instance method along each known class hierarchy, and conservatively considers possible interface defaults. The result is combined with the R194-style exact declared edges before reverse traversal.

This is a *class-hierarchy over-approximation* across all known concrete classes, **not** a runtime-execution trace or a set of observed active consumers. In particular, the graph does not know which receivers are actually constructed, which branches run, whether the first-party GUI is open, or whether an input API access initiates gameplay. The method count is not a count of gameplay input consumers.

## Reproduced pinned-source results

Official original Minecraft 1.8.9 input JAR: **8,461,484 bytes**, SHA-1 `3870888a6c3d349d3771a3e9d16c9bf5e076b908`. R195 independently reuses the R171 SHA-1, class count, direct callsite count and full direct-input fingerprint, with a prior R194 graph SHA-256 check.

| Metric | R194 declared-target graph | R195 possible-dispatch graph |
|---|---:|---:|
| Original classes | 2,507 | 2,507 |
| Methods in original classes | 19,766 | 19,766 |
| Direct input callsites | 146 | 146 |
| Direct methods with these calls | 56 | 56 |
| Reverse-reachable methods | **237** | **6,655** |
| 1-hop reverse callers | 46 | 53 |
| 2+-hop reverse callers | 135 | 6,546 |
| Maximum reverse distance | 6 | 21 |

R195's possible-dispatch candidate expansion added **39,705** distinct extra `caller→possible-target` edges beyond the exact targets in its graph. **11,540** virtual/interface callsite evaluations added at least one new candidate target. These numbers are intentionally broad. Diagnostics also find calls with no known concrete receiver candidate and calls whose target/implementation is missing in the scanned JAR. Those **gaps mean the result is neither a complete upper bound nor proof of exclusive event ownership**.

Pinned R195 possible graph SHA-256:

`3c8399abdc065c43d933e14355b6835675898e2386851054940fa006ef60fbfe`

The R194 declared-target digest remains:

`f0b1cb8aee451b56302a0035ec92665829c774a251718b4e00f61852d9e3bdfd`

## Verified negative controls

`R195PossibleInputDispatchSmoke` generates isolated Java bytecode, **not** a loaded Minecraft JVM:

- A caller invoking `Base.run()` is linked to `Override.run()`, whose body reads `Mouse.isButtonDown`, despite the base's own method not accessing input
- An `INVOKEINTERFACE` caller is linked to the known concrete implementer's Mouse-reading method
- A different subclass's non-reading override is **not** mislabeled as a direct input root
- A class order permutation leaves the graph fingerprint unchanged, and replacing the Mouse-reading override eliminates the synthetic reachable input path
- A call targeting a missing external receiver is recorded as unresolved, rather than fabricating a usable implementation
- The pinned original source is required to reproduce R171, R194 and R195 exact hashes; analysis refuses changed source and does not write the original JAR

These are model properties, not original game runtime input tests.

## Deployment isolation and remaining gates

`R195PossibleInputDispatch` and its smoke test are compiled only into the separately distributed `slinkywinky_render_audit.jar` and disposable test build outputs. `tests/attach_smoke.py` verifies they are **absent** from `slinkywinky_instrument.jar`. The R195 synthetic test is registered in CTest, and `tests/official_189_render_audit.py` now adds the exact pinned JAR plus graph SHA acceptance.

**Important limitations:** Java virtual/interface dispatch via classes not present in the JAR, reflection, native libraries, invokedynamic bootstrap behavior, OS event routing, real Minecraft input focus and dynamically loaded classes remain unresolved. Conversely, considering every known concrete subclass can drastically overcount reachable methods: it does not mean they all execute while the GUI is visible.

## Verified Windows and original-client acceptance

The complete Windows x64 Release build passed and **95/95 local CTest tests passed**, including the new R195 synthetic virtual/interface dispatch fixture, Java 8/JNI, native WGL ClickGUI and agent-attach regressions. The rebuilt offline audit JAR separately reproduced the **6,655** possible-method fingerprint and verified the exact Mojang JAR remained byte-identical. Python syntax checks of the official source-audit script and package-contents test passed. The official source-audit *download workflow* was not run separately.

The isolated original vanilla Minecraft 1.8.9 JVM read-only acceptance exited 0: **six** restoration/admission stages, **64** retained GL11 frames, **60** source-visible slider draw frames, and **63** passive pointer observations. Original Win32 focus evidence remained `foreground=0 background=63 refused=0 losses=0 regains=0 post_hits=0`, source exact-hover hits **0** and `native_click_delivery=disabled`. This is *background-only* rendering acceptance and does **not** certify real hardware cursor, keyboard, double-dispatch suppression, physical release or Java class restoration.

**No live Minecraft input hook was enabled; no events are injected, consumed, replayed, or suppressed.** R190–R193 native WGL paired input remains a synthetic fixture. Commercial Slinky ClickGUI visual parity and all gameplay modules remain unfinished. Keep PR #1 open as draft and do not modify PR #2.
