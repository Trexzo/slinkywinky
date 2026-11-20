# R244 — owner-bound two-phase Java 8 / JNI Clutch prior ray (read-only)

Date: 2026-11-20. SlinkyWinky PR #2, building on R243. PR #1 remains untouched.

## Gap

The R239/R241 native Clutch evaluation deliberately rejected previously
selected blocks because the original algorithm needs a world ray AFTER
its source-randomized aim, angle and strict 4.49-block reach checks.
It would be incorrect to treat a missing ray as a verified null result or
to sample it before the native source aim is known.

## Transaction protocol

The Java 8 world sampler now retains the exact prior-block coordinates in
its immutable `Snapshot`, independently of caller-provided `Request`
objects. `nativeMatchesWithPrior` rejects any changed generation, tick,
geometry, source distance range, min-height setting or prior coordinates.

On one owner thread:

1. Java captures world identity, player identity, source ordered block IDs
   and a previously selected block ID using `ClutchWorldSampler.capture`.
2. `ClutchNativePreview.nativePreparePrior` consumes that scope under the
   native generation/tick/owner gate. The C++ preflight evaluates original
   source walk samples on a **copy** of the RNG and returns the exact ray
   endpoints only when source angle and reach criteria require a ray.
3. Java calls `ClutchWorldSampler.priorRay` on the real game-thread `Port`.
   The sampler verifies the original snapshot, one-ray-per-tick restriction,
   world/player identity on both sides of the ray, strict 4.49 reach, and
   a bounded block result. A null ray is explicitly distinguished from
   missing ray evidence.
4. `nativeFinishPrior` accepts exactly the pending tick and expected
   evidence shape. It returns the final native planner's 15-value
   read-only proposal (including preserved-prior and aiming values),
   advancing its source RNG **only on accepted completion**.
5. Any Java-visible transaction exception closes the Java native owner;
   native replay, inconsistent evidence and wrong-owner failures quarantine
   its gate. There is no gameplay, camera, key, packet or inventory mutation.

The existing no-prior JNI method is preserved and separately tested.

## Automated regression scope

The Java 8 native smoke tests exercise:
- The source native prior preserved after one real null ray observation.
- Obstruction at another block revoking previous-block preservation.
- Unreachable previous aim proceeding without a world ray.
- Caller-substituted previous coordinates, stale tick replay, and a
  ray-transport exception refusing or closing the native owner.

C++ tests separately verify that the preflight cannot consume the
shared source RNG and that the phase-two aim uses the same coordinates.
The CTest wrapper requires BOTH `R239_JAVA8_NATIVE_CLUTCH_SOURCE_PLANNER_PASS`
and `R244_JAVA8_NATIVE_CLUTCH_PRIOR_RAY_PASS`.

## Provenance and limitations

The supplied private decrypted archive's distance-prefix and 167-entry
Clutch offset table were checked separately in R242/R243. No original
binary, proprietary source blob, copied machine instructions or assets
are committed to the GitHub branch.

This bridge uses the synthetic Java 8 `Port` in CI. The existing
`ForgeClutchWorldPort` offers real Minecraft 1.8.9 block/ray reads but
the opt-in Forge preview still needs to carry persisted prior candidates
through its live tick loop. The actual original vanilla Minecraft 1.8.9
injectable runtime, source-equivalent movement/rotation/click/placement,
recovery, and all other Slinky module behaviors remain outstanding.

CI verification is determined by the workflow runs for the exact final
commit; writing this document does not itself establish a passing build.
Keep PRs #1/#2 draft and unmerged.
